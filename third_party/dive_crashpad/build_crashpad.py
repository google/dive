#
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import enum
import os
import platform
import shutil
import subprocess
import sys
import shlex
from pathlib import Path

ROOT_DIR = Path.cwd()
DEPOT_TOOLS_DIR = ROOT_DIR / "depot_tools"
CRASHPAD_DIR = ROOT_DIR / "crashpad"
CRASHPAD_OUT_DIR = CRASHPAD_DIR / "out" / "Default"


class BuildType(enum.StrEnum):
    DEBUG = "Debug"
    REL_WITH_DEB_INFO = "RelWithDebInfo"
    RELEASE = "Release"


def run_command(cmd, cwd=None, env=None, shell=False):
    """Runs a shell command and raises an exception on failure."""
    if isinstance(cmd, str) and not shell:
        cmd = shlex.split(cmd)
        
    print(f"[Crash Report] Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    try:
        subprocess.check_call(cmd, cwd=cwd, env=env, shell=shell)
    except subprocess.CalledProcessError as e:
        print(f"[Crash Report] Error executing command: {e}")
        sys.exit(1)


def check_cache(args, current_stamp):
    """Checks if the build can be skipped based on metadata and existing files."""
    metadata_file = Path(args.metadata_file)

    if not metadata_file.exists():
        print("[Crash Report] Crashpad metadata file not found.")
        return False

    cached_stamp = metadata_file.read_text().strip()

    if cached_stamp != current_stamp:
        print(f"[Crash Report] Crashpad metadata mismatch.\n  Cached:  {cached_stamp}\n  Current: {current_stamp}")
        return False

    if not Path(args.cached_handler).exists():
        print(f"[Crash Report] Crashpad handler missing: {args.cached_handler}")
        return False

    libs = args.cached_libs.split()
    for lib in libs:
        if not Path(lib).exists():
            print(f"[Crash Report] Crashpad library missing: {lib}")
            return False

    print(f"[Crash Report] All Crashpad libraries exist and metadata matches ({current_stamp}). Skipping Crashpad build.")
    return True


def setup_depot_tools():
    """Ensures depot_tools is installed and adds it to PATH."""
    if not DEPOT_TOOLS_DIR.exists():
        print("[Crash Report] Cloning depot_tools...")
        run_command(["git", "clone", "https://chromium.googlesource.com/chromium/tools/depot_tools", DEPOT_TOOLS_DIR.name], cwd=ROOT_DIR)

    # Safely check if depot_tools is already in the PATH
    current_path = os.environ.get("PATH", "")
    depot_tools_str = str(DEPOT_TOOLS_DIR)

    # Split by pathsep to avoid partial string matches
    if depot_tools_str not in current_path.split(os.pathsep):
        os.environ["PATH"] = depot_tools_str + os.pathsep + current_path
    
    if platform.system() == "Windows":
        run_command(["git", "config", "--global", "core.longpaths", "true"])
        os.environ["DEPOT_TOOLS_WIN_TOOLCHAIN"] = "0"


def fetch_and_sync_crashpad(commit_hash):
    """Fetches crashpad source and syncs to the specific commit."""
    if not CRASHPAD_DIR.exists():
        print("[Crash Report] Fetching Crashpad...")
        run_command(["fetch", "crashpad"], cwd=ROOT_DIR, shell=(platform.system() == "Windows"))

    print(f"[Crash Report] Syncing Crashpad to {commit_hash}...")
    run_command(["git", "fetch", "origin"], cwd=CRASHPAD_DIR)
    run_command(["git", "checkout", commit_hash], cwd=CRASHPAD_DIR)
    run_command(["gclient", "sync"], cwd=CRASHPAD_DIR, shell=(platform.system() == "Windows"))


def build_crashpad(config):
    """Generates build files with GN and builds with Ninja."""
    is_debug = "true" if config == "Debug" else "false"
    target_cpu = "x64"
    extra_cflags = ""
    
    system = platform.system()
    
    if system == "Windows":
        extra_cflags = "/MDd" if config == "Debug" else "/MD"
    elif system == "Linux":
        extra_cflags = "-fPIC"
    elif system == "Darwin":
        machine = platform.machine()
        target_cpu = "arm64" if machine == "arm64" else "x64"

    gn_args_list = [f'is_debug={is_debug}', f'target_cpu="{target_cpu}"']
    if extra_cflags:
        gn_args_list.append(f'extra_cflags="{extra_cflags}"')
    
    gn_args_str = " ".join(gn_args_list)
    print(f"[Crash Report] Generating build files with args: {gn_args_str}")
    
    shell_cmd = (system == "Windows")
    
    run_command(["gn", "gen", "out/Default", f"--args={gn_args_str}"], cwd=CRASHPAD_DIR, shell=shell_cmd)

    print("[Crash Report] Building with Ninja...")
    ninja_cmd = ["ninja", "-C", "out/Default"]
    if system == "Darwin":
        ninja_cmd.extend(["-j", "4"])

    # Explicitly list only the targets we need to avoid building unnecessary binaries.
    if system == "Windows":
        targets_to_build = [
            "crashpad_handler.exe",
            "obj/third_party/mini_chromium/mini_chromium/base/base.lib",
            "obj/client/common.lib",
            "obj/client/client.lib",
            "obj/util/util.lib"
        ]
    else:
        targets_to_build = [
            "crashpad_handler",
            "obj/third_party/mini_chromium/mini_chromium/base/libbase.a",
            "obj/client/libcommon.a",
            "obj/client/libclient.a",
            "obj/util/libutil.a"
        ]
    ninja_cmd.extend(targets_to_build)

    run_command(ninja_cmd, cwd=CRASHPAD_DIR, shell=shell_cmd)


def handle_mac_artifacts():
    """Special handling for macOS to merge MIG objects into libutil."""
    if platform.system() != "Darwin":
        return

    patterns = ["mig_output*.o", "*child_portServer.o", "*child_portUser.o", "*notifyServer.o", "*notifyUser.o"]
    mig_objs = []
    
    for pattern in patterns:
        mig_objs.extend(list(CRASHPAD_OUT_DIR.rglob(pattern)))

    # Filter out test binaries
    mig_objs = [f for f in mig_objs if "test" not in f.name and "gtest" not in f.name]

    if not any("child_portServer" in f.name for f in mig_objs):
        print("[Crash Report] Error: Could not locate critical MIG files (child_portServer) in out/Default")
        sys.exit(1)

    print(f"[Crash Report] Merging {len(mig_objs)} MIG objects into libutil_complete.a...")
    
    libutil_path = CRASHPAD_OUT_DIR / "obj" / "util" / "libutil.a"
    output_lib = CRASHPAD_OUT_DIR / "libutil_complete.a"
    
    cmd = ["/usr/bin/libtool", "-static", "-o", str(output_lib), str(libutil_path)] + [str(p) for p in mig_objs]
    run_command(cmd)


def cache_artifacts(args):
    """Copies built artifacts to the prebuilt directory."""
    prebuilt_dir = Path(args.prebuilt_dir)
    print(f"[Crash Report] Caching artifacts to {prebuilt_dir}...")
    prebuilt_dir.mkdir(parents=True, exist_ok=True)

    system = platform.system()

    # Define relative paths using Path operator
    if system == "Windows":
        artifacts = [
            (Path("crashpad_handler.exe"), "crashpad_handler.exe"),
            (Path("obj") / "third_party" / "mini_chromium" / "mini_chromium" / "base" / "base.lib", "base.lib"),
            (Path("obj") / "client" / "common.lib", "common.lib"),
            (Path("obj") / "client" / "client.lib", "client.lib"),
            (Path("obj") / "util" / "util.lib", "util.lib"),
        ]
    elif system == "Darwin":
        artifacts = [
            (Path("crashpad_handler"), "crashpad_handler"),
            (Path("obj") / "third_party" / "mini_chromium" / "mini_chromium" / "base" / "libbase.a", "libbase.a"),
            (Path("obj") / "client" / "libcommon.a", "libcommon.a"),
            (Path("obj") / "client" / "libclient.a", "libclient.a"),
            (Path("libutil_complete.a"), "libutil.a"),
        ]
    else:
        artifacts = [
            (Path("crashpad_handler"), "crashpad_handler"),
            (Path("obj") / "third_party" / "mini_chromium" / "mini_chromium" / "base" / "libbase.a", "libbase.a"),
            (Path("obj") / "client" / "libcommon.a", "libcommon.a"),
            (Path("obj") / "client" / "libclient.a", "libclient.a"),
            (Path("obj") / "util" / "libutil.a", "libutil.a"),
        ]

    for src_rel, dest_name in artifacts:
        src = CRASHPAD_OUT_DIR / src_rel
        dst = prebuilt_dir / dest_name

        if not src.exists():
            print(f"[Crash Report] Error: Build artifact missing: {src}")
            sys.exit(1)

        shutil.copy2(src, dst)


def main():
    parser = argparse.ArgumentParser(
        description="Fetches, builds, and caches the Crashpad library.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument("--commit-hash-file", required=True,
                        help="Path to the text file containing the Crashpad Git commit hash to sync to.")
    parser.add_argument("--prebuilt-dir", required=True,
                        help="Directory where the final compiled artifacts should be copied and cached.")
    parser.add_argument("--metadata-file", required=True,
                        help="Path to the metadata file used to track the current build stamp.")
    parser.add_argument("--config", required=True,
                        choices=[str(build_type) for build_type in BuildType],
                        help="The build configuration type (Debug, RelWithDebInfo, Release).")
    parser.add_argument("--compiler-stamp", required=True,
                        help="A unique identifier representing the current compiler toolchain version.")
    parser.add_argument("--cached-libs", required=True,
                        help="Space-separated list of paths to the expected cached library files.")
    parser.add_argument("--cached-handler", required=True,
                        help="Path to the expected cached crashpad_handler executable.")
    
    args, unknown = parser.parse_known_args()

    if args.config != BuildType.REL_WITH_DEB_INFO:
        print(f"[Crash Report] Skipping Crashpad build for configuration: {args.config}. Only {BuildType.REL_WITH_DEB_INFO} is supported.")
        return

    commit_hash = Path(args.commit_hash_file).read_text().strip()
    current_stamp = f"{args.config}_{args.compiler_stamp}"

    if check_cache(args, current_stamp):
        return

    setup_depot_tools()
    fetch_and_sync_crashpad(commit_hash)
    build_crashpad(args.config)
    handle_mac_artifacts()
    cache_artifacts(args)

    Path(args.metadata_file).write_text(current_stamp)
    print("[Crash Report] Crashpad build complete and metadata updated.")


if __name__ == "__main__":
    main()
