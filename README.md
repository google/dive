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
git submodule update --init
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Building the driver in Linux (DEBUG)
```
cd <dive_path>
git submodule update --init
cd driver/xgl
mkdir -p build/Debug64
cd build/Debug64
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make -j$(nproc)
```

### Building the Windows installer

Build Dive in Release first.
You require the "Microsoft Visual Studio Installer Projects" extension to be installed for VS2017.
In an "x64 Native Tools Command Prompt for VS2017" window:
```
cd dive_installer
build-installer.bat
```
This will produce the following four files:
1. dive_installer\Release\release-notes.txt: The release notes.
2. dive_installer\Release\dive.msi: The main installer for Dive.  Requires the VC++ redistributable to already be installed.
3. dive_installer\Release\vc_redist.x64.exe: The VC++ redistributable required by dive.msi.
4. dive_installer\Release\dive.zip: A simple .zip archive to allow people to try Dive without installing it
   (they may have to install the included VC++14 redistributable).

### Steps to create an "official" build

1. Follow the instructions above to build the installer.
2. type ..\ui\version.h
3. git tag vM.m.R.B, where M.m.R.B are the major, minor, revision, and build numbers from version.h.
4. Edit ..\ui\version.h and increment the revision number.
5. Commit and push the version change.  Remember to use --tags to push the tag as well.

### Capture layer on Linux

To enable the capture feature on Linux, you need to install the capture layer on your Linux.
There're two ways to install the layer:

#### Install the layer using `make`

You can simply run `make install` to install the layer library and its configuration file.

This command will install:

- The layer library to `~/.local/lib/dive/`;
- The layer configuration file to `~/.local/share/vulkan/implicit_layer.d`;

#### Install the layer manually

- Copy `libVkLayer_dive_capture.so` to `~/.local/lib/dive/`;
- Copy `VkLayer_dive_capture.json` to `~/.local/share/vulkan/implicit_layer.d`;

### Generated structure-of-arrays code

The generated structure-of-array classes (dive_core/marker_types.h) are not automatically re-generated
by the cmake build. To re-generate the code, run:
```
$ python dive_core/generateSOAs.py dive_core/marker_types.json
```

#### Debugging generated structure-of-arrays code

The structure-of-arrays types are represented as a single allocations, with each array offset into
that allocation by some amount. This makes the values in the structure-of-arrays types difficult to
directly inspect in a debugger. If you need to inspect one of these types, you have two options:
- In a debug build, the generated type should also include `DBG_*` fields for each array, which
  points to the first element in the array.
- If you are using Visual Studio code, you can add the generated "*.natvis" file (e.g.
  "dive_core/marker_types.natvis") into the project. This should give a much nicer representation of the
  structure-of-arrays types (and their iterators).

## Jupyter notebooks and Dive Python bindings

Dive capture data is exposed through Python bindings, which enables exploration and visualization in Jupyter notebooks. See the [pydive README](pydive/README.md) for more information.

