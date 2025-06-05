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

This repository is formatted with LLVM clang-format version 18.1.8.

#### Setup on Linux

Install `clang-format-18` with `sudo apt install clang-format-18` and verify that it is version 18.1.8.
Suggested usage is to run `./scripts/clangformat.sh` after committing changes, and all `.h` and `.cpp` files in the commit will be verified.

#### Setup on Windows

Install clang-format 18.1.8 using the appropriate Windows installer at: https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8
Suggested usage is using your preferred IDE, set it to format on save, and point to the installed `clang-format.exe`.

#### Pull requests
There is a lint github action that will run clang-format on all source code in the project.

### Code style

- `CamelCase` for class and function names. The exception is overriding methods, like Qt, where adopting their naming convention is required.
- Prefix class member variables with `m_`

### Code Reviews

All submissions, including submissions by project members, require review. We
use [GitHub pull requests](https://docs.github.com/articles/about-pull-requests)
for this purpose.

- Merging reviews approval from 2 Google reviewers.
- "Squash and merge" is the preferred option to merge a PR since we like a linear git history.
- "Update with rebase" only when the branch is out-of-date. This ensures a linear history in case "Rebase and merge" is used to submit a PR.
