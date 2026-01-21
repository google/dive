if(TARGET Vulkan::Headers)
    return()
endif()

if(TARGET vulkan_registry)
    add_library(Vulkan::Headers ALIAS vulkan_registry)
    return()
endif()

add_library(Vulkan::Headers INTERFACE IMPORTED)
set_target_properties(
    Vulkan::Headers
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_LIST_DIR}/../third_party/gfxreconstruct/external/Vulkan-Headers/include
)
