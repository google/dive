#!/bin/bash

set -eux

# TODO: Understand additions/subtractions between subtree pull commits
# Right now, this will never actually remove anything (only add). It's up to the reviewer to catch when things are removed
# Likely not catastrophic but leaves the repo less tidy

if [ $# -ne 3 ]
then
    echo "Usage: $0 BASE_INPUT_FILE OVERRIDE_INPUT_FILE OUTPUT_FILE"
    exit 1
fi

readonly BASE_INPUT_FILE="$1"
readonly OVERRIDE_INPUT_FILE="$2"
readonly OUTPUT_FILE="$3"

cp "${BASE_INPUT_FILE}" "${OUTPUT_FILE}"

for name in $(git config list -f "${OVERRIDE_INPUT_FILE}" --name-only)
do
    value="$(git config get -f "${OVERRIDE_INPUT_FILE}" ${name})"
    echo "${value}"
    # TODO this won't work with multi-valued configs
    git config set -f "${OUTPUT_FILE}" --all "${name}" "${value}"
done
