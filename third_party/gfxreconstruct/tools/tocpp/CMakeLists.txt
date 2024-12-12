add_executable(gfxrecon-tocpp "")

if(MSVC)
    target_compile_options(gfxrecon-tocpp PRIVATE /wd4244)
    target_link_libraries(gfxrecon-tocpp psapi.lib)
    set_target_properties(gfxrecon-tocpp PROPERTIES FOLDER "gfxreconstruct")
endif()

target_sources(gfxrecon-tocpp
               PRIVATE
                   main.cpp
              )

target_include_directories(gfxrecon-tocpp PUBLIC ${CMAKE_BINARY_DIR})

target_link_libraries(gfxrecon-tocpp gfxrecon_decode gfxrecon_format gfxrecon_util platform_specific)

common_build_directives(gfxrecon-tocpp)

install(TARGETS gfxrecon-tocpp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
