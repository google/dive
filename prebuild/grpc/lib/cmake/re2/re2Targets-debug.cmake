#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "re2::re2" for configuration "Debug"
set_property(TARGET re2::re2 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(re2::re2 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/re2d.lib"
  )

list(APPEND _cmake_import_check_targets re2::re2 )
list(APPEND _cmake_import_check_files_for_re2::re2 "${_IMPORT_PREFIX}/lib/re2d.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
