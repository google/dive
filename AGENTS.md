# Dive Project Development Guidelines for AI Agents

This document provides essential instructions and best practices for developing in the Dive codebase. Adhere to these guidelines to ensure consistency and quality.

## Overview

Dive is a graphics debugger and profiler. This repository contains the source code for the Dive host tools and libraries.

## Core Software Engineering Principles

Follow these principles when writing and modifying code:

-   **Avoid code duplication**: Before writing a new function, search the codebase for existing functions that provide similar functionality.
-   **Reuse and refactor**: If a suitable function exists, reuse it. If it's close but not an exact match, consider refactoring the existing function to accommodate the new use case instead of creating a copy.
-   **Consult if unsure**: If you are considering duplicating a function or a significant block of code, consult with the user first.

## Setup Dependencies

Before building the project, ensure you have the following dependencies installed.

### Common Dependencies

-   **CMake**: Version 3.22 or higher.
-   **Ninja**: Recommended build system generator.
-   **Python 3**: With `mako` package installed (`pip install mako`).
-   **Qt 5.15.2**: Required for the UI.

### Linux

Reference environment (Ubuntu 24.04):

```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build python3-mako
# Install Qt dependencies
sudo apt-get install -y qtbase5-dev libxcb-xinerama0 libxcb-xinerama0-dev libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxkbcommon-x11-0 libxcb1 libx11-xcb1 libxcb-glx0-dev libsystemd-dev libbsd-dev
# Install Clang 19
sudo apt-get install -y clang-19
```

### Windows

1.  **Visual Studio 2022**: Install with C++ desktop development workload.
2.  **Qt 5.15.2**: Install via the [Qt Online Installer](https://download.qt.io/archive/online_installers/). Select version 5.15.2 and `win64_msvc2019_64`.
3.  **Python Mako**: `pip install mako`

### macOS

1.  **AppleClang17**: Ensure you have the required compiler version.
2.  **Qt 5**: `brew install qt@5`
3.  **Mako**: `pip3 install mako`
4.  **CMake & Ninja**: `brew install cmake ninja`

### Android Development

-   **Android NDK**: Version r25 (e.g., 25.2.9519653).
-   **Java 17**: Required for Gradle.
-   **Environment Variables**:
    -   `ANDROID_NDK_HOME`: Path to NDK.
    -   `JAVA_HOME`: Path to Java 17 installation.

## Building C++ Code

The project uses CMake. The standard build directory name is `build`.

### Linux (Clang 19)

```bash
export CC=/usr/bin/clang-19
export CXX=/usr/bin/clang++-19
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -Bbuild .
ninja -C build
```

### Windows (Visual Studio 2022)

```cmd
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -Bbuild .
cmake --build build --config Debug
```

### macOS (AppleClang17)

```bash
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -Bbuild .
ninja -C build
```

## Android Build

To build the Android libraries, use the provided helper scripts. These scripts build into the `build_android` directory.

### Linux / macOS

```bash
./scripts/build_android.sh Debug
```

### Windows

```cmd
scripts\build_android.bat Debug
```

## Code Style

### C++

-   **Style Guide**: Google C++ Style Guide.
-   **Formatter**: `clang-format` version **18.1.8**.
-   **Naming Conventions**:
    -   `CamelCase` for class and function names. The exception is overriding methods, like Qt, where adopting their naming convention is required.
    -   `snake_case` for variable names.
    -   Prefix class member variables with `m_`.

### CMake

-   **Formatter**: `gersemi` version **0.23.1**.

### Verification

There are lint checks in CI. You can run `clang-format` locally to verify compliance.

## Running Tests

Tests are managed by CTest.

```bash
ctest --output-on-failure -C Debug --test-dir build
```

## Pull Requests & Commit Messages

-   **Commit Messages**: Standard convention (Short subject 50 chars max, blank line, detailed body).
-   **Review**: Requires approval from 2 Google reviewers.
-   **Merge Strategy**: Squash and merge is preferred for a linear history.
-   **Linting**: Ensure `clang-format` and `gersemi` checks pass.
