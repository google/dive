#!/bin/bash

# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ==============================================================================
# SYNOPSIS
#    Uploads Crashpad .sym files to Google Symbol Collector using sym-upload-v2 via curl.
#
# USAGE
#    ./sym-upload-v2.sh <SymFilePath> <UploadUrl>
# ==============================================================================

# strict mode: exit on error, undefined vars, or pipe failures
set -euo pipefail

SYM_FILE_PATH="${1:-}"
UPLOAD_URL="${2:-}"

readonly API_KEY="${CRASHPAD_API_KEY:-}"
if [[ -z "$API_KEY" ]]; then
    echo "CRASHPAD_API_KEY environment variable is not set. Symbol upload will fail. Exiting."
    exit 1
fi

# Helper functions.

function write_step() {
    echo -e "\n[Upload debug symbols]: $1"
}

function die() {
    echo -e "\n[ERROR] Symbol upload failed!"
    echo "$1"
    exit 1
}

function get_json_value() {
    key=$1
    json=$2

    if command -v python3 &>/dev/null; then
        echo "$json" | python3 -c "import sys, json; print(json.load(sys.stdin).get('$key', ''))"
    elif command -v python &>/dev/null; then
        echo "$json" | python -c "import sys, json; print(json.load(sys.stdin).get('$key', ''))"
    else
        echo "$json" | grep -o "\"$key\": *\"[^\"]*\"" | sed "s/\"$key\": *\"//;s/\"//"
    fi
}

# Validation.

if [[ -z "$SYM_FILE_PATH" || -z "$UPLOAD_URL" ]]; then
    echo "Usage: $0 <SymFilePath> <UploadUrl>"
    exit 1
fi

if [[ ! -f "$SYM_FILE_PATH" ]]; then
    echo "CRITICAL: Symbol file not found at $SYM_FILE_PATH"
    exit 1
fi

# Metadata Extraction.

# Read first line.
FIRST_LINE=$(head -n 1 "$SYM_FILE_PATH")

# Split into array.
read -r -a PARTS <<< "$FIRST_LINE"

if [[ "${#PARTS[@]}" -lt 5 ]]; then
    echo "CRITICAL: Invalid .sym file format. Header missing required parts."
    exit 1
fi

# In standard sym format: MODULE OS CPU ID FILENAME
DEBUG_ID="${PARTS[3]}"
DEBUG_FILE="${PARTS[4]}"
# Trim trailing slash.
BASE_URL="${UPLOAD_URL%/}" 

write_step "Preparing upload for $DEBUG_FILE ($DEBUG_ID)"

# Handshake (Create Upload).
CREATE_URL="$BASE_URL/v1/uploads:create?key=$API_KEY"
write_step "Requesting upload credentials..."

# Capture response, handle potential curl errors via '|| die'.
RAW_RESPONSE=$(curl -s --fail --location --request POST "$CREATE_URL" \
    --header "Content-Length: 0" || die "Failed to connect to create endpoint.")

UPLOAD_URL_SIGNED=$(get_json_value "uploadUrl" "$RAW_RESPONSE")
UPLOAD_KEY=$(get_json_value "uploadKey" "$RAW_RESPONSE")

if [[ -z "$UPLOAD_URL_SIGNED" || -z "$UPLOAD_KEY" ]]; then
    die "Server response missing credentials. Raw output: $RAW_RESPONSE"
fi

# Binary Upload (PUT).
write_step "Sending file to storage..."

curl --fail --show-error --location --request PUT "$UPLOAD_URL_SIGNED" \
    --upload-file "$SYM_FILE_PATH" \
    --header "Content-Type:" || die "File transfer failed."

# Finalize (Complete).
write_step "Notifying collector of completion..."

COMPLETE_URL="$BASE_URL/v1/uploads/$UPLOAD_KEY:complete?key=$API_KEY"

# Construct JSON body safely.
COMPLETE_BODY=$(cat <<EOF
{
  "symbol_id": {
    "debug_file": "$DEBUG_FILE",
    "debug_id": "$DEBUG_ID"
  }
}
EOF
)

curl -s --fail --location --request POST "$COMPLETE_URL" \
    --header "Content-Type: application/json" \
    --data "$COMPLETE_BODY" || die "Finalization request failed."

echo -e "\n[SUCCESS] Symbol uploaded and finalized.\n"
