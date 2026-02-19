add_executable(gfxrecon-vk-optimize main.cc)
target_link_libraries(
    gfxrecon-vk-optimize
    PRIVATE
        gfxrecon_decode
        gfxrecon_util
        absl::status
        absl::strings
        absl::flags_parse
        absl::flags_usage
        absl::flags
        dive_core
)
