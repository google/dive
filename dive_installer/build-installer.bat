:: Copyright 2023 Google LLC
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::      https://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

@echo off
pushd ..\build\ui\release
windeployqt .
popd
devenv dive_installer.sln /build Release
copy release-notes.txt dive_installer\Release
copy release-notes.txt ..\build\ui\release
copy ..\build\NOTICE ..\build\ui\release
copy ..\build\ui\release\vc_redist.x64.exe dive_installer\Release
del dive_installer\Release\setup.exe
powershell "compress-archive ..\build\ui\release\* dive_installer\Release\dive.zip -force"
