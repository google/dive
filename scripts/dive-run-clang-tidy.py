"""Runs clang-tidy on the Dive codebase."""

import argparse
import enum
import pathlib
import subprocess
import shutil
import sys

import common_dive_utils as dive


class FilesChoice(enum.StrEnum):
    ALL = enum.auto()
    GIT_DIFF = enum.auto()
    SPECIFIC = enum.auto()


def find_program(candidate: str) -> pathlib.Path | None:
    which = shutil.which(candidate)
    if which:
        return pathlib.Path(which)

    candidate_path = pathlib.Path(candidate)
    if candidate_path.exists():
        return candidate_path

    raise argparse.ArgumentTypeError(f"Can't find program: {candidate}")


def git_diff(git: pathlib.Path) -> list[str]:
    return (
        subprocess.check_output(
            [
                str(git),
                "diff",
                "--diff-filter=ACMR",
                "--name-only",
                "origin/main..HEAD",
                "--",
            ],
            text=True,
        )
        .strip()
        .splitlines()
    )


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
        "--files",
        type=FilesChoice,
        choices=[x for x in FilesChoice],
        default=FilesChoice.GIT_DIFF,
        help="Which files to run clang-tidy on.",
    )
    parser.add_argument(
        "specific_files",
        nargs="*",
        help="With `--files specific`, these files will be analyzed with clang-tidy.",
    )
    return parser.parse_args()


def main(args: argparse.Namespace):
    dive_root = dive.get_dive_root()
    source_filter = "|".join([f"{str(dive_root)}/{dir}/.*" for dir in dive.SOURCE_DIRS])
    # NOTE: This script doesn't filter on arg.files since run-clang-tidy does that for us.

    def choose_files() -> list[str]:
        match args.files:
            case FilesChoice.ALL:
                return []
            case FilesChoice.GIT_DIFF:
                return git_diff(args.git)
            case FilesChoice.SPECIFIC:
                # TODO: raise error if specific_files is empty
                return args.specific_files
            case _:
                raise NotImplementedError(f"Unknown --files option: {args.files}")

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
            + choose_files()
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
