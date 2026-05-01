# How to Contribute

We would love to accept your patches and contributions to this project.

## Before you begin

### Sign our Contributor License Agreement

Contributions to this project must be accompanied by a
[Contributor License Agreement](https://cla.developers.google.com/about) (CLA).
You (or your employer) retain the copyright to your contribution; this simply
gives us permission to use and redistribute your contributions as part of the
project.

If you or your current employer have already signed the Google CLA (even if it
was for a different project), you probably don't need to do it again.

Visit <https://cla.developers.google.com/> to see your current agreements or to
sign a new one.

### Review our Community Guidelines

This project follows [Google's Open Source Community
Guidelines](https://opensource.google/conduct/).

## Contribution process

### Formatting
- The C++ code in this repository is formatted with LLVM clang-format version 18.1.8
- The cmake files are formatted with [gersemi](https://pypi.org/project/gersemi/) version 0.23.1

#### Setup clang-format on Linux

Install `clang-format-18` with `sudo apt install clang-format-18` and verify that it is version 18.1.8.
Suggested usage is to run `./scripts/clangformat.sh` after committing changes, and all `.h` and `.cpp` files in the commit will be verified.

#### Setup clang-format on Windows

Install clang-format 18.1.8 using the appropriate Windows installer at: https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8
Suggested usage is using your preferred IDE, set it to format on save, and point to the installed `clang-format.exe`.

#### Run clang-tidy (Recommended)

It is recommended to install clang-tidy (see instructions for clang-format above) and use it to tidy your C++ files before submitting a PR. This is not required but it's a recommended step, with the eventual goal of having the Dive codebase ready for a clang-tidy presubmit.

#### Setup gersemi

Follow instructions [here](https://pypi.org/project/gersemi/) to use pip to install the package. Make sure to install the version supported by Dive, it is recommended to use Python virtual environments. To run for this project, use provided `scripts/format_cmake.bat` or `scripts/format_cmake.sh` scripts.

#### Pull requests

There is a lint github action that will run clang-format on all source code in the project, and another one that runs the format_cmake scripts with gersemi.

### Code style
- `CamelCase` for class and function names. The exception is overriding methods, like Qt, where adopting their naming convention is required.
- `snake_case` for variable names.
- Prefix class member variables with `m_`

### Code Reviews

All submissions, including submissions by project members, require review. We
use [GitHub pull requests](https://docs.github.com/articles/about-pull-requests)
for this purpose.

- Merging reviews approval from 2 Google reviewers.
- "Squash and merge" is the preferred option to merge a PR since we like a linear git history.
- "Update with rebase" only when the branch is out-of-date. This ensures a linear history in case "Rebase and merge" is used to submit a PR.
- Add a comment to the PR describing what manual tests were performed by the contributor.

### Unit and regression tests
```sh
cd $DIVE_ROOT_PATH
ctest -C Debug --test-dir build/host
```

Make sure to have built everything according to BUILD.md (don't forget the install step) before running the tests.

## Updating Dive's gfxreconstruct subtree

1. Create a branch to contain the merge.
    ```sh
    git checkout -b subtree_pull
    ```
1. Run the pull command. Merge conflicts are expected:
    ```sh
    git subtree pull --prefix=third_party/gfxreconstruct https://github.com/LunarG/gfxreconstruct.git dev --squash
    ```
1. Run `//scripts/incorporate_gfxr_submodules.py` so that the required submodules are cloned.
1. Since the previous step can't remove submodules, remove any `third_party/gfxreconstruct/external/*` submodule entries in `//.gitmodules` that aren't in `//third_party/gfxreconstruct/.gitmodules`.
1. Update the submodules so that the merge commit includes the correct SHA:
    ```sh
    git submodule update --init --recursive
    ```
1. Regenerate GFXR Vulkan code:
    ```sh
    cd third_party/gfxreconstruct/framework/generated
    python generate_vulkan.py
    ```
1. Try to [build](BUILD.md). Fix any errors.
1. Resolve any conflicts that arise and ensure dive-specific changes are not removed. Files with dive-specific changes have comment lines: // GOOGLE: or # GOOGLE. If there are conflicts, don't forget to add them and commit:
    ```sh
    git add third_party/gfxreconstruct
    git commit -m "Merge third_party/gfxreconstruct updates"
    ```
1. Create a pull request for the updates.
1. Monitor PR builds; you might need to fix the GitHub workflows.
1. Ensure the commit is not squash merged so that git can find the subtree updates. This requires temporarily disabling the ["Require linear history" Branch Protection rule](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches#require-linear-history)

## Summary of repo structure

As a long-running project, Dive has significant amounts of legacy code and this is reflected in its directory structure.

### Top-level Directories

* **.github/**
    * Configuration files related to [Dive's Github Actions](https://github.com/google/dive/actions) for nightly and presubmit automated testing
* **capture_service/**
    * Libraries related to the Android device and communication via `adb`
    * Also contains the main CLI tool `dive_client_cli`
        * GFXR capture and replay
* **cli/**
    * CLI tool `divecli`
        * Parsing PM4 capture
* **cmake/**
    * Contains `.cmake` files used in the cmake building process
* **dive_core/**
    * Functionality to parse captures (GFXR, PM4) into in-memory representations (Command Hierarchies)
    * Also support for other data: GPU timing, metrics...
* **gfxr_dump_resources/**
    * CLI tool `gfxr_dump_resources`
        * Process a GFXR capture and produce a file suitable for use with GFXR `--dump-resources`
* **gfxr_ext/**
    * Library to extend the functionality of `third_party/gfxreconstruct` for use in Dive
    * Separated from the GFXR code to keep the process of updating the subtree simpler
* **gpu_time/**
    * Library for using Vulkan timestamps to time events on the GPU
* **host_cli/**
    * CLI tool `host_cli`
        * Block-level manipulation of GFXR captures
* **layer/**
    * `libVkLayer_Dive.so` and `libXrApiLayer_dive.so` are Vulkan layers 
        * Meant to be used on the device when running either GFXR replay APK or other Vulkan applications
        * To support features such as PM4 capture via freedreno's `libwrap.so`
* **lrz_validator/**
    * Lightweight tool `lrz_validator` to validate the LowResolution Z Buffer data inside a PM4 capture
* **network/**
    * Library for socket communication between the host and the Android device
* **plugins/**
    * Samples and placeholders related to Dive plugins (refer to the [README.md](https://github.com/google/dive/tree/main/plugins/README.md) for more details)
* **prebuild/**
    * Contains prebuilt libraries to speed up the build process for host tools
        * `libarchive`
        * `zlib`
* **runtime_layer/**
    * `libVkLayer_rt_dive.so` is a Vulkan layer
        * Mainly for use on the device, though can be useful for debugging on the host
        * Intercepts Vulkan commands to alter them and support Runtime What-Ifs functionality
* **scripts/**
    * All scripts, user-facing and the ones used automatically during the build process
* **src/**
    * Newer Dive source code
* **test/**
    * Golden files and other test data for the unit tests
* **third_party/**
    * Third-party submodules and subtrees used in Dive
* **trace_stats/**
    * Library that extracts statistics from a PM4 capture for displaying in the UI
* **ui/**
    * Dive UI, built using `Qt` libraries

### Planned Structural Improvements

These are some general ideas for structural improvements. Please keep them in mind during the addition of new code and the refactoring of the current code:

* Dive source code should live underneath `src/` rather than any other top-level directories in this repo
* Since Dive supports "GFXR captures" and "PM4 captures", naming related to traces/captures should be specific
* Consolidate CLI tools rather than further splintering
* More scripts should be written in Python for cross-platform support
