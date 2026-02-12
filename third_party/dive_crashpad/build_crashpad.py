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
import os
import platform
import shutil
import subprocess
import sys
import glob
import shlex

def run_command(cmd, cwd=None, env=None, shell=False):
    """Runs a shell command and raises an exception on failure."""
    if isinstance(cmd, str) and not shell:
        cmd = shlex.split(cmd)
        
    print(f"Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    try:
        subprocess.check_call(cmd, cwd=cwd, env=env, shell=shell)
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        sys.exit(1)

def check_cache(args, current_stamp):
    """Checks if the build can be skipped based on metadata and existing files."""
    if not os.path.exists(args.metadata_file):
        print("Crashpad metadata file not found.")
        return False

    with open(args.metadata_file, 'r') as f:
        cached_stamp = f.read().strip()

    if cached_stamp != current_stamp:
        print(f"Crashpad metadata mismatch.\n  Cached:  {cached_stamp}\n  Current: {current_stamp}")
        return False

    if not os.path.exists(args.cached_handler):
        print(f"Crashpad handler missing: {args.cached_handler}")
        return False

    libs = args.cached_libs.split()
    for lib in libs:
        if not os.path.exists(lib):
            print(f"Crashpad library missing: {lib}")
            return False

    print(f"All files exist and metadata matches ({current_stamp}). Skipping Crashpad build.")
    return True

def setup_depot_tools(root_dir):
    """Ensures depot_tools is installed and adds it to PATH."""
    depot_tools_dir = os.path.join(root_dir, "depot_tools")
    
    if not os.path.exists(depot_tools_dir):
        print("Cloning depot_tools...")
        run_command(["git", "clone", "https://chromium.googlesource.com/chromium/tools/depot_tools", "depot_tools"], cwd=root_dir)

    os.environ["PATH"] = depot_tools_dir + os.pathsep + os.environ["PATH"]
    
    if platform.system() == "Windows":
        run_command(["git", "config", "--global", "core.longpaths", "true"])
        os.environ["DEPOT_TOOLS_WIN_TOOLCHAIN"] = "0"

def fetch_and_sync_crashpad(root_dir, commit_hash):
    """Fetches crashpad source and syncs to the specific commit."""
    crashpad_dir = os.path.join(root_dir, "crashpad")

    if not os.path.exists(crashpad_dir):
        print("Fetching crashpad...")
        run_command(["fetch", "crashpad"], cwd=root_dir, shell=(platform.system() == "Windows"))

    print(f"Syncing Crashpad to {commit_hash}...")
    run_command(["git", "fetch", "origin"], cwd=crashpad_dir)
    run_command(["git", "checkout", commit_hash], cwd=crashpad_dir)
    run_command(["gclient", "sync"], cwd=crashpad_dir, shell=(platform.system() == "Windows"))

def build_crashpad(root_dir, config):
    """Generates build files with GN and builds with Ninja."""
    crashpad_dir = os.path.join(root_dir, "crashpad")
    
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
    print(f"Generating build files with args: {gn_args_str}")
    
    shell_cmd = (system == "Windows")
    
    run_command(["gn", "gen", "out/Default", f"--args={gn_args_str}"], cwd=crashpad_dir, shell=shell_cmd)

    print("Building with Ninja...")
    ninja_cmd = ["ninja", "-C", "out/Default"]
    if system == "Darwin":
        ninja_cmd.extend(["-j", "4"])
        
    run_command(ninja_cmd, cwd=crashpad_dir, shell=shell_cmd)

def handle_mac_artifacts(crashpad_dir):
    """Special handling for macOS to merge MIG objects into libutil."""
    if platform.system() != "Darwin":
        return

    out_dir = os.path.join(crashpad_dir, "out", "Default")
    
    patterns = ["mig_output*.o", "*child_portServer.o", "*child_portUser.o", "*notifyServer.o", "*notifyUser.o"]
    mig_objs = []
    
    for root, dirs, files in os.walk(out_dir):
        for file in files:
            if any(glob.fnmatch.fnmatch(file, p) for p in patterns):
                if "test" not in file and "gtest" not in file:
                    mig_objs.append(os.path.join(root, file))

    if not any("child_portServer" in f for f in mig_objs):
        print("Error: Could not locate critical MIG files (child_portServer) in out/Default")
        sys.exit(1)

    print(f"Merging {len(mig_objs)} MIG objects into libutil_complete.a...")
    
    libutil_path = os.path.join(out_dir, "obj", "util", "libutil.a")
    output_lib = os.path.join(out_dir, "libutil_complete.a")
    
    cmd = ["/usr/bin/libtool", "-static", "-o", output_lib, libutil_path] + mig_objs
    run_command(cmd)

def cache_artifacts(args, root_dir):
    """Copies built artifacts to the prebuilt directory."""
    print(f"Caching artifacts to {args.prebuilt_dir}...")
    if not os.path.exists(args.prebuilt_dir):
        os.makedirs(args.prebuilt_dir)

    crashpad_out = os.path.join(root_dir, "crashpad", "out", "Default")
    system = platform.system()

    artifacts = []
    if system == "Windows":
        artifacts = [
            ("crashpad_handler.exe", "crashpad_handler.exe"),
            (os.path.join("obj", "third_party", "mini_chromium", "mini_chromium", "base", "base.lib"), "base.lib"),
            (os.path.join("obj", "client", "common.lib"), "common.lib"),
            (os.path.join("obj", "client", "client.lib"), "client.lib"),
            (os.path.join("obj", "util", "util.lib"), "util.lib"),
        ]
    elif system == "Darwin":
        artifacts = [
            ("crashpad_handler", "crashpad_handler"),
            (os.path.join("obj", "third_party", "mini_chromium", "mini_chromium", "base", "libbase.a"), "libbase.a"),
            (os.path.join("obj", "client", "libcommon.a"), "libcommon.a"),
            (os.path.join("obj", "client", "libclient.a"), "libclient.a"),
            ("libutil_complete.a", "libutil.a"),
        ]
    else:
        artifacts = [
            ("crashpad_handler", "crashpad_handler"),
            (os.path.join("obj", "third_party", "mini_chromium", "mini_chromium", "base", "libbase.a"), "libbase.a"),
            (os.path.join("obj", "client", "libcommon.a"), "libcommon.a"),
            (os.path.join("obj", "client", "libclient.a"), "libclient.a"),
            (os.path.join("obj", "util", "libutil.a"), "libutil.a"),
        ]

    for src_rel, dest_name in artifacts:
        src = os.path.join(crashpad_out, src_rel)
        dst = os.path.join(args.prebuilt_dir, dest_name)
        if not os.path.exists(src):
            print(f"Error: Build artifact missing: {src}")
            sys.exit(1)
        shutil.copy2(src, dst)

def main():
    parser = argparse.ArgumentParser(description="Build Crashpad")
    parser.add_argument("commit_hash_file")
    parser.add_argument("prebuilt_dir")
    parser.add_argument("metadata_file")
    parser.add_argument("config")
    parser.add_argument("compiler_stamp")
    parser.add_argument("cached_libs")
    parser.add_argument("cached_handler")
    
    args, unknown = parser.parse_known_args()

    with open(args.commit_hash_file, 'r') as f:
        commit_hash = f.read().strip()

    current_stamp = f"{args.config}_{args.compiler_stamp}"
    if check_cache(args, current_stamp):
        return

    root_dir = os.getcwd() 
    setup_depot_tools(root_dir)

    fetch_and_sync_crashpad(root_dir, commit_hash)

    build_crashpad(root_dir, args.config)
    handle_mac_artifacts(os.path.join(root_dir, "crashpad"))

    cache_artifacts(args, root_dir)

    with open(args.metadata_file, 'w') as f:
        f.write(current_stamp)
    
    print("Crashpad build complete and metadata updated.")

if __name__ == "__main__":
    main()
