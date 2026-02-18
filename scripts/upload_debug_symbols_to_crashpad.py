#
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import sys
import json
import urllib.request
import urllib.error

def write_step(message):
    print(f"\n[Upload debug symbols]: {message}")

def main():
    parser = argparse.ArgumentParser(
        description="Upload Crashpad .sym files to Google Symbol Collector."
    )
    parser.add_argument("sym_file_path", help="Path to the .sym file")
    parser.add_argument("upload_url", help="Base URL for the symbol collector")
    
    args = parser.parse_args()

    api_key = os.environ.get("CRASHPAD_API_KEY")
    if not api_key:
        raise RuntimeError("CRASHPAD_API_KEY environment variable is not set. Symbol upload will fail.")

    if not os.path.exists(args.sym_file_path):
        raise FileNotFoundError(f"Symbol file not found at {args.sym_file_path}")

    with open(args.sym_file_path, 'r', encoding='utf-8', errors='ignore') as f:
        first_line = f.readline().strip()
    
    parts = first_line.split()
    if len(parts) < 5:
        raise ValueError("Invalid .sym file format. Header missing required parts.")

    # Standard sym format: MODULE OS CPU ID FILENAME
    debug_id = parts[3]
    debug_file = parts[4]

    base_url = args.upload_url.rstrip('/')
    write_step(f"Preparing upload for {debug_file} ({debug_id})")

    try:
        create_url = f"{base_url}/v1/uploads:create?key={api_key}"
        write_step("Requesting upload credentials...")

        req = urllib.request.Request(create_url, method='POST')
        req.add_header('Content-Length', '0')
        
        with urllib.request.urlopen(req) as response:
            if response.status != 200:
                raise RuntimeError(f"Create request failed with status {response.status}")
            
            data = json.load(response)
            upload_url_signed = data.get('uploadUrl')
            upload_key = data.get('uploadKey')

        if not upload_url_signed or not upload_key:
            raise ValueError(f"Server response missing credentials. Raw response: {data}")

        write_step("Sending file to storage...")

        with open(args.sym_file_path, 'rb') as f:
            file_data = f.read()

        req = urllib.request.Request(upload_url_signed, data=file_data, method='PUT')
        # The server expects NO Content-Type header (or empty).
        req.add_header('Content-Type', '') 
        
        with urllib.request.urlopen(req) as response:
            if response.status not in (200, 201):
                raise RuntimeError(f"File transfer failed with status {response.status}")

        write_step("Notifying collector of completion...")

        complete_url = f"{base_url}/v1/uploads/{upload_key}:complete?key={api_key}"
        
        payload = {
            "symbol_id": {
                "debug_file": debug_file,
                "debug_id": debug_id
            }
        }
        json_payload = json.dumps(payload).encode('utf-8')

        req = urllib.request.Request(complete_url, data=json_payload, method='POST')
        req.add_header('Content-Type', 'application/json')
        
        with urllib.request.urlopen(req) as response:
            if response.status != 200:
                raise RuntimeError(f"Finalization failed with status {response.status}")

        print("\n[SUCCESS] Symbol uploaded and finalized.\n")

    except urllib.error.HTTPError as e:
        try:
            print(f"Response context: {e.read().decode('utf-8')}")
        except Exception:
            pass
        raise

if __name__ == "__main__":
    main()
