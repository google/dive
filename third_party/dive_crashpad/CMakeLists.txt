cmake_minimum_required(VERSION 3.24)

project(dive_crashpad LANGUAGES CXX)

message(CHECK_START "Generate build files for third_party/dive_crashpad")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

# --- 1. Platform Configuration & Paths ---
# We define where the "Vendored" (Prebuilt) binaries should live in your source tree.
if(WIN32)
    set(PLATFORM "win32")
    set(LAUNCHER cmd /c)
    set(SCRIPT_NAME "build_crashpad_win32.bat")
    set(HANDLER_NAME "crashpad_handler.exe")
    set(LIB_BASE "base.lib")
    set(LIB_COMMON "common.lib")
    set(LIB_CLIENT "client.lib")
    set(LIB_UTIL "util.lib")
    set(PLATFORM_UTILS "crashpad_utils_win32.cpp")
elseif(APPLE)
    set(PLATFORM "mac")
    set(LAUNCHER bash)
    set(SCRIPT_NAME "build_crashpad_mac.sh")
    set(HANDLER_NAME "crashpad_handler")
    set(LIB_BASE "libbase.a")
    set(LIB_COMMON "libcommon.a")
    set(LIB_CLIENT "libclient.a")
    set(LIB_UTIL "libutil.a")
    set(PLATFORM_UTILS "crashpad_utils_macos.cpp")
else()
    set(PLATFORM "linux")
    set(LAUNCHER bash)
    set(SCRIPT_NAME "build_crashpad_linux.sh")
    set(HANDLER_NAME "crashpad_handler")
    set(LIB_BASE "libbase.a")
    set(LIB_COMMON "libcommon.a")
    set(LIB_CLIENT "libclient.a")
    set(LIB_UTIL "libutil.a")
    set(PLATFORM_UTILS "crashpad_utils_linux.cpp")
endif()

# The persistent cache directory in your source tree
set(PREBUILT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt/${PLATFORM}")

# The specific files we expect to exist
set(CACHED_HANDLER "${PREBUILT_DIR}/${HANDLER_NAME}")
set(CACHED_LIBS
    "${PREBUILT_DIR}/${LIB_CLIENT}"
    "${PREBUILT_DIR}/${LIB_COMMON}"
    "${PREBUILT_DIR}/${LIB_UTIL}"
    "${PREBUILT_DIR}/${LIB_BASE}"
)

# --- 2. Smart Build Rule ---
# This command tells CMake: "I need these files. If they exist, do nothing.
# If they are missing (or crashpad_commit.txt changed), run the script to create them."
add_custom_command(
    OUTPUT ${CACHED_HANDLER} ${CACHED_LIBS}
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/crashpad_commit.txt"
        "${CMAKE_CURRENT_LIST_DIR}/${SCRIPT_NAME}"
    COMMAND
        ${LAUNCHER} "${CMAKE_CURRENT_LIST_DIR}/${SCRIPT_NAME}"
        "${CMAKE_CURRENT_LIST_DIR}/crashpad_commit.txt"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building Crashpad (${PLATFORM}) and caching binaries..."
    VERBATIM
)

# A target to trigger the command above
add_custom_target(
    dive_crashpad_ensure_binaries
    DEPENDS ${CACHED_HANDLER} ${CACHED_LIBS}
)

# --- 3. Dive Wrapper Library ---
add_library(
    dive_crashpad
    STATIC
    dive_crashpad/crashpad_client.cpp
    dive_crashpad/crashpad_client.h
    dive_crashpad/crashpad_utils.h
    dive_crashpad/${PLATFORM_UTILS}
)

# Ensure binaries exist before compiling our wrapper
add_dependencies(dive_crashpad dive_crashpad_ensure_binaries)

# Link against the CACHED libraries directly
# NOTE: Order matters for static linking (Dependent -> Dependency)
target_link_libraries(
    dive_crashpad
    PUBLIC absl::base absl::status absl::strings
    PRIVATE ${CACHED_LIBS}
)

if(APPLE)
    target_link_libraries(
        dive_crashpad
        PRIVATE
            "-framework AppKit"
            "-framework CoreFoundation"
            "-framework Security"
            "bsm"
    )
endif()

target_include_directories(dive_crashpad PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_include_directories(
    dive_crashpad
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/crashpad"
        "${CMAKE_CURRENT_BINARY_DIR}/crashpad/third_party/mini_chromium/mini_chromium"
)
target_include_directories(dive_crashpad PUBLIC "${CMAKE_SOURCE_DIR}/src")

# --- 4. Expose Handler Executable ---
add_executable(crashpad_handler_exe IMPORTED GLOBAL)
set_property(
    TARGET crashpad_handler_exe
    PROPERTY IMPORTED_LOCATION "${CACHED_HANDLER}"
)
add_dependencies(crashpad_handler_exe dive_crashpad_ensure_binaries)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
