"""Invokes run-clang-tidy with `git diff` files as input.

All command-line flags are passed straight through to run-clang-tidy
"""

import shutil
import subprocess
import sys

EXCLUSION_PREFIX = ["prebuild", "third_party"]


class ProgramNotFoundError(Exception):
    pass


def find_program(program: str) -> str:
    which = shutil.which(program)
    if which is None:
        raise ProgramNotFoundError(program)
    return which


def should_analyze_file(filepath: str) -> bool:
    for prefix in EXCLUSION_PREFIX:
        if filepath.startswith(prefix):
            return False
    return True


def main():
    git = find_program("git")
    run_clang_tidy = find_program("run-clang-tidy")

    diff_files = (
        subprocess.check_output(
            [git, "diff", "origin/main", "--name-only", "--diff-filter=ACMR"], text=True
        )
        .strip()
        .splitlines()
    )
    diff_files = [file for file in diff_files if should_analyze_file(file)]

    sys.exit(
        subprocess.run([run_clang_tidy] + sys.argv[1:] + ["--"] + diff_files).returncode
    )


if __name__ == "__main__":
    main()
