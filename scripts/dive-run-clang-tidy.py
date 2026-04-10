"""Runs clang-tidy on the Dive codebase."""

import argparse
import shutil
import pathlib
import subprocess
import sys

import common_dive_utils as dive


def find_program(candidate: str) -> pathlib.Path | None:
    which = shutil.which(candidate)
    if which:
        return pathlib.Path(which)

    candidate_path = pathlib.Path(candidate)
    if candidate_path.exists():
        return candidate_path

    raise argparse.ArgumentTypeError(f"Can't find program: {candidate}")


def parse_args() -> argparse.Namespace:
    build_dir = dive.get_dive_root() / "build"
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--run_clang_tidy",
        type=find_program,
        default="run-clang-tidy",
        help="Path to run-clang-tidy or run-clang-tidy.py.",
    )
    parser.add_argument(
        "--git",
        type=find_program,
        default="git",
        help="Path to git.",
    )
    parser.add_argument(
        "--host_build_dir",
        type=pathlib.Path,
        default=build_dir / "host",
        help="Path to the host build directory.",
    )
    parser.add_argument(
        "--device_build_dir",
        type=pathlib.Path,
        default=build_dir / "device",
        help="Path to the device build directory.",
    )
    parser.add_argument(
        "--config_file",
        type=pathlib.Path,
        help="Path to the clang-tidy config file. Defaults to clang-tidy behavior.",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Automatically fix issues that are found. Same as clang-tidy -fix.",
    )
    parser.add_argument(
        "files",
        nargs="*",
        help="Optional list of files to analyze. Defaults to entire Dive codebase.",
    )
    return parser.parse_args()


def main(args: argparse.Namespace):
    print(args.run_clang_tidy)
    print(args.git)

    dive_root = dive.get_dive_root()
    source_filter = "|".join([f"{str(dive_root)}/{dir}/.*" for dir in dive.SOURCE_DIRS])
    # NOTE: This script doesn't filter on arg.files since run-clang-tidy does that for us.

    def execute_run_clang_tidy(build_dir: pathlib.Path) -> int:
        return subprocess.run(
            [
                sys.executable,
                str(args.run_clang_tidy),
                "-p",
                str(build_dir),
                "-source-filter",
                source_filter,
            ]
            + (["-fix"] if args.fix else [])
            + (["-config-file", str(args.config_file)] if args.config_file else [])
            + (args.files if args.files else [])
        ).returncode

    returncodes: list[int] = []
    if args.host_build_dir.exists():
        returncodes.append(execute_run_clang_tidy(args.host_build_dir))
    else:
        print(f"Skipping host build dir, doesn't exist: {str(args.host_build_dir)}")

    if args.device_build_dir.exists():
        returncodes.append(execute_run_clang_tidy(args.device_build_dir))
    else:
        print(f"Skipping device build dir, doesn't exist: {str(args.device_build_dir)}")

    if len(returncodes) == 0:
        print("clang-tidy was not run. No build directories exist.")
        sys.exit(1)

    for returncode in returncodes:
        if returncode != 0:
            sys.exit(returncode)


if __name__ == "__main__":
    main(parse_args())
