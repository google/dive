#!/bin/bash

set -eux

readonly PREFIX="third_party/gfxreconstruct"
readonly REPOSITORY="https://github.com/LunarG/gfxreconstruct.git"
readonly REF="dev"

# set up branch
git checkout main
git checkout -b "subtree_pull_$(date +%s)"

# do the pull; ignore the exit code (we'll figure out what to do later)
git subtree pull --prefix="${PREFIX}" "${REPOSITORY}" "${REF}" --squash || true
readonly squash_commit="$(cat .git/MERGE_HEAD)"

# TODO validate squash_commit
git rev-parse "${squash_commit}"

# Since this is a subtree, need to checkout any submodule updates before add
git subtree update --init --recursive

# TODO merge .gitmodules

# Commit the merge conflicts. This gives reviewers a good idea of what you changed.
git add .
git commit -m "Merge commit '${squash_commit}' as '${PREFIX}'"

# TODO what if no merge conflicts?

