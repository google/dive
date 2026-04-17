"""Runs clang-tidy on the Dive codebase."""

import argparse
import enum
import pathlib
import subprocess
import shutil
import sys

import common_dive_utils as dive


class Commands(enum.StrEnum):
    ALL = enum.auto()
    GIT_DIFF = enum.auto()
    FILES = enum.auto()


def find_program(candidate: str) -> pathlib.Path | None:
    which = shutil.which(candidate)
    if which:
        return pathlib.Path(which)

    candidate_path = pathlib.Path(candidate)
    if candidate_path.exists():
        return candidate_path

    return None


def git_diff(git: pathlib.Path, commit: str) -> list[str]:
    command = [
        str(git),
        "diff",
        "--diff-filter=ACMR",
        "--name-only",
        commit,
        "--",
    ]
    process = subprocess.run(command, text=True, capture_output=True)
    if process.returncode != 0:
        raise RuntimeError(
            f"Command failed: {command}.\nstdout: {process.stdout}\nstderr: {process.stderr}"
        )

    return process.stdout.strip().splitlines()


def parse_args() -> argparse.Namespace:
    def find_program_or_die(candidate: str) -> pathlib.Path:
        program = find_program(candidate)
        if program is None:
            raise argparse.ArgumentTypeError(f"Can't find program: {candidate}")
        return program

    def find_run_clang_tidy(candidate: str) -> pathlib.Path:
        program = find_program_or_die(candidate)
        process = subprocess.run([program, "--help"], capture_output=True, text=True)
        if "-source-filter" not in process.stdout:
            raise argparse.ArgumentTypeError(
                f"run-clang-tidy doesn't support -source-filter. Need at least run-clang-tidy-19. "
                f"Tried: {candidate}"
            )
        return program

    build_dir = dive.get_dive_root() / "build"
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    subparsers = parser.add_subparsers(dest="command")

    subparsers.add_parser(
        str(Commands.ALL),
        help="Run clang-tidy on the entire repository.",
    )

    files_subparser = subparsers.add_parser(
        str(Commands.FILES),
        help="Run clang-tidy on the provided files.",
    )
    files_subparser.add_argument(
        "specific_files",
        nargs="+",
        metavar="FILE",
        help="Files to be analyzed by clang-tidy.",
    )

    git_diff_subparser = subparsers.add_parser(
        str(Commands.GIT_DIFF),
        help="Run clang-tidy on changed files.",
    )
    git_diff_subparser.add_argument(
        "--git",
        type=find_program_or_die,
        default="git",
        help="Path to git.",
    )
    git_diff_subparser.add_argument(
        "commit",
        help="Which commit to diff against.",
        nargs="?",
        default="origin/main",
    )

    parser.add_argument(
        "--run_clang_tidy",
        type=find_run_clang_tidy,
        default="run-clang-tidy",
        help=(
            "Path to run-clang-tidy or run-clang-tidy.py. Must support -source-filter (at least "
            "run-clang-tidy-19)"
        ),
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
    args = parser.parse_args()

    if args.command is None:
        # Default to git_diff. In order to properly parse arguments on
        # subparsers (i.e. to populate args.git), reparse with the default
        # command explicitly provided.
        args = parser.parse_args([str(Commands.GIT_DIFF)] + sys.argv[1:])

    return args


def main(args: argparse.Namespace):
    dive_root = dive.get_dive_root()
    source_filter = "|".join([f"{str(dive_root)}/{dir}/.*" for dir in dive.SOURCE_DIRS])
    # NOTE: run-clang-tidy uses source_filter to filter chosen files so we don't have to

    print(args)

    def choose_files() -> list[str]:
        match args.command:
            case Commands.ALL:
                return []
            case Commands.GIT_DIFF:
                return git_diff(args.git, args.commit)
            case Commands.FILES:
                return args.specific_files
            case unknown:
                raise NotImplementedError(f"Unknown command: {unknown}")
        return []

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
