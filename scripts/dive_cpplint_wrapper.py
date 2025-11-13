"""cpplint wrapper for Dive."""

import argparse
import pathlib
import subprocess

DIVE_SOURCE_DIRS_CURRENT = (
    "cli", "common", "dive_core", "gfxr_dump_resources", "gfxr_ext", "gpu_time",
    "host_cli", "layer", "lrz_validator", "network", "plugins", "runtime_layer",
    "tests", "trace_stats", "ui", "utils",
)
# Note: this is part of source tree organization
DIVE_SOURCE_DIRS_FUTURE = ("include", "src")
DIVE_SOURCE_DIRS = DIVE_SOURCE_DIRS_CURRENT + DIVE_SOURCE_DIRS_FUTURE
DIVE_SOURCE_DIRS_SET = frozenset(DIVE_SOURCE_DIRS)
DIVE_CPPLINT_FILE_SUFFIX = (".cpp", ".cc", ".h")
DIVE_CPPLINT_FILTERS_COMMON = (
    # Dive repo specific
    "-build/c++17",  # <filesystem>
    "-readability/todo",
    "-readability/braces",  # macro parsing with {} on next line
    "-whitespace/braces",
    "-whitespace/indent",
    "-whitespace/newline",
    "-whitespace/blank_line",
    "-whitespace/parens",
    # Enable later
    "-build/include_what_you_use",
    "-build/include_subdir",
    "-build/include_order",
    "-runtime/explicit",
    "-readability/casting",
    "-whitespace/comments",
)

DIVE_CPPLINT_FILTERS = DIVE_CPPLINT_FILTERS_COMMON

DIVE_CPPLINT_FILTERS_CI = DIVE_CPPLINT_FILTERS_COMMON + (
    "-whitespace/line_length",
    "-readability/namespace",
    "-build/namespaces/source/namespace/nonliterals",
    "-whitespace/ending_newline",
    "-readability/inheritance",
)

DIVE_CPPLINT_FILTERS_G3 = DIVE_CPPLINT_FILTERS_COMMON + (
    "-whitespace/line_length",
    "-build/c++11",
    "-build/include",
    "-build/header_guard",
    "-runtime/deprecated_fn",
)

DIVE_CPPLINT_FLAGS = (
    "--linelength=100",
    "--filter="+",".join(DIVE_CPPLINT_FILTERS),
)

DIVE_CPPLINT_FLAGS_CI = (
    "--linelength=100",
    "--filter="+",".join(DIVE_CPPLINT_FILTERS_CI),
)

DIVE_CPPLINT_FLAGS_G3 = (
    "--filter="+",".join(DIVE_CPPLINT_FILTERS_G3),
)


def list_files_pathlib_rglob(top_dirs, dive_root):
    files = []
    for top_dir in top_dirs:
        for path in dive_root.joinpath(top_dir).rglob("*"):
            if not path.is_file():
                continue
            if path.suffix not in DIVE_CPPLINT_FILE_SUFFIX:
                continue
            files.append(path)
    return files


def require_lint(path, dive_root_resolved):
    """Check if current file need to run through cpplint."""
    if not path.is_file():
        return False
    if not path.exists():
        print("Skip non-exist file:", path)
        return False
    if not path.resolve().is_relative_to(dive_root_resolved):
        print("WARNING: Found out of tree file:", path)
        return False
    rel_path = path.resolve().relative_to(dive_root_resolved)
    if path.suffix not in DIVE_CPPLINT_FILE_SUFFIX:
        print("Skip non-C++ file:", path)
        return False
    if not rel_path.parts or rel_path.parts[0] not in DIVE_SOURCE_DIRS_SET:
        print("Skip non-Dive code:", path)
        return False
    return True, ""


def get_file_list(file_iter, dive_root):
    dive_root_resolved = dive_root.resolve()
    files = []
    for line in file_iter:
        line = line.strip()
        if not line:
            continue
        path = pathlib.Path(line)
        if not require_lint(path, dive_root_resolved):
            continue
        files.append(path)
    return files


def get_file_list_from_diff_file(diff_file, dive_root):
    with open(diff_file) as f:
        return get_file_list(f, dive_root)


def auto_file_list(base_commit, dive_root):
    result = subprocess.run(["git", "diff", base_commit, "--name-only"],
                            capture_output=True, text=True, check=True)
    return get_file_list(result.stdout.splitlines(), dive_root)


def main(args):
    files = []
    if args.all:
        files = list_files_pathlib_rglob(
            DIVE_SOURCE_DIRS, pathlib.Path(args.dive_root))
    elif args.git_diff_file:
        files = get_file_list_from_diff_file(pathlib.Path(
            args.git_diff_file), pathlib.Path(args.dive_root))
    else:
        files = auto_file_list(args.base_commit, pathlib.Path(args.dive_root))
    if not files:
        print("Nothing for cpplint")
        return
    files_strings = tuple(sorted(str(path) for path in files))
    if args.mode == "g3":
        command = (args.cpplint,) + DIVE_CPPLINT_FLAGS_G3 + files_strings
    elif args.mode == "ci":
        command = (args.cpplint,) + DIVE_CPPLINT_FLAGS_CI + files_strings
    else:
        command = (args.cpplint,) + DIVE_CPPLINT_FLAGS + files_strings
    print("Running cpplint:", " ".join(command))
    subprocess.run(command, check=True, text=True)


def parse_args():
    dive_root = pathlib.Path(__file__).parent.parent.resolve()
    if dive_root == pathlib.Path(".").resolve():
        dive_root = pathlib.Path(".")
    parser = argparse.ArgumentParser(prog="cpplint_wrapper")
    parser.add_argument("--git_diff_file")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--dive_root", default=str(dive_root))
    parser.add_argument("--cpplint", default="cpplint")
    parser.add_argument("--base_commit", default="origin/main")
    parser.add_argument("--mode", choices=["", "g3", "ci"])
    return parser.parse_args()


if __name__ == "__main__":
    main(parse_args())
