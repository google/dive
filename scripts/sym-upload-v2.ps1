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
#    ./sym-upload-v2.ps1 <SymFilePath> <UploadUrl>
# ==============================================================================


param (
    [string]$SymFilePath,
    [string]$UploadUrl
)

$ApiKey = $env:CRASHPAD_API_KEY
if ([string]::IsNullOrWhiteSpace($ApiKey)) {
    Write-Host "CRASHPAD_API_KEY environment variable is not set. Symbol upload will fail. Exiting." -ForegroundColor Red
    exit 1
}

$ErrorActionPreference = "Stop"

function Write-Step ([string]$Message) {
    Write-Host "`n[Upload debug symbols]: $Message" -ForegroundColor Cyan
}

# Validate File and Extract Metadata
if (-Not (Test-Path $SymFilePath)) {
    Write-Host "CRITICAL: Symbol file not found at $SymFilePath" -ForegroundColor Red
    exit 1
}

$FirstLine = Get-Content $SymFilePath -First 1
$Parts = $FirstLine -split '\s+'

if ($Parts.Count -lt 5) {
    Write-Host "CRITICAL: Invalid .sym file format. Header missing required parts." -ForegroundColor Red
    exit 1
}

$DebugId   = $Parts[3]
$DebugFile = $Parts[4]
$BaseUrl   = $UploadUrl.TrimEnd('/')

Write-Step "Preparing upload for $DebugFile ($DebugId)"

try {
    # Handshake (Create Upload)
    $CreateUrl = "$BaseUrl/v1/uploads:create?key=$ApiKey"
    Write-Step "Requesting upload credentials..."
    
    $RawResponse = curl.exe -s --location --request POST "$CreateUrl" --header "Content-Length: 0"
    $Response = $RawResponse | ConvertFrom-Json

    if (-not $Response.uploadUrl -or -not $Response.uploadKey) {
        throw "Server response missing credentials. Raw output: $RawResponse"
    }

    $UploadUrlSigned = $Response.uploadUrl
    $UploadKey       = $Response.uploadKey

    # Binary Upload (PUT)
    Write-Step "Sending file to storage..."
    
    # We use -T for raw file transfer and remove Content-Type to avoid signature mismatch
    curl.exe --location --request PUT "$UploadUrlSigned" `
             --upload-file "$SymFilePath" `
             --header "Content-Type:" `
             --fail --show-error

    if ($LASTEXITCODE -ne 0) { throw "File transfer failed with exit code $LASTEXITCODE" }

    # Finalize (Complete)
    Write-Step "Notifying collector of completion..."
    
    $CompleteUrl = "$BaseUrl/v1/uploads/$UploadKey:complete?key=$ApiKey"
    $CompleteBody = @{
        symbol_id = @{
            debug_file = $DebugFile
            debug_id   = $DebugId
        }
    } | ConvertTo-Json -Compress

    curl.exe -s --location --request POST "$CompleteUrl" `
             --header "Content-Type: application/json" `
             --data "$CompleteBody" `
             --fail

    Write-Host "`n[SUCCESS] Symbol uploaded and finalized.`n" -ForegroundColor Green

} catch {
    Write-Host "`n[ERROR] Symbol upload failed!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Yellow
    exit 1
}
