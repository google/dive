### Building Dive host tool in Windows
```
REM Assumes QTDIR is set to msvc directory (eg. C:\Qt\5.11.2\msvc2017_64)
set CMAKE_PREFIX_PATH=%QTDIR%
set PATH=%QTDIR%\bin;%PATH%
cd <dive_path>
git submodule update --init
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
```
Open `dive.sln` and build in Debug or Release
Or run:
cmake --build . --config Debug
cmake --build . --config Release

### Building Dive host tool in Linux (DEBUG)
```
REM Assumes QTDIR is set to gcc_64 directory (eg. ~/Qt/5.11.2/gcc_64)
export CMAKE_PREFIX_PATH=$QTDIR
export PATH=$QTDIR:$PATH
cd <dive_path>
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Building Android Libraries

- Download the Android NDK (e.g. 25.2.9519653)
- Set the environment variable `ANDROID_NDK_HOME` (e.g. export ANDROID_NDK_HOME=~/andriod_sdk/ndk/25.2.9519653)
Run the script 

```
./scripts/build_android.sh
```

It will build both debug and release version of the libraries.


