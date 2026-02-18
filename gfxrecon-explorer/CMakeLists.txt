add_executable(gfxrecon-explorer main.cc)
target_link_libraries(
    gfxrecon-explorer
    PRIVATE
        gfxrecon_decode
        gfxrecon_util
        absl::status
        absl::strings
        absl::flags_parse
        absl::flags_usage
        absl::flags
)
