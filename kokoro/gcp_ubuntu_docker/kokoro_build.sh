#!/bin/bash

# Fail on any error.
set -e

cd "${KOKORO_ARTIFACTS_DIR}/github/dive"
./scripts/kokoro/linux/build.sh
