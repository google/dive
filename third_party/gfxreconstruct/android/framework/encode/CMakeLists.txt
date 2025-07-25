add_library(gfxrecon_encode STATIC "")

target_sources(gfxrecon_encode
               PRIVATE
                   ${GFXRECON_SOURCE_DIR}/framework/encode/api_capture_manager.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/api_capture_manager.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/capture_manager.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/capture_manager.cpp               
                   ${GFXRECON_SOURCE_DIR}/framework/encode/capture_settings.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/capture_settings.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/command_writer.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/command_writer.cpp
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_encoder_commands.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_api_call_encoders.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_api_call_encoders.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_struct_encoders.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_struct_encoders.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_struct_handle_wrappers.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/custom_openxr_struct_handle_wrappers.cpp>
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_encoder_commands.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_api_call_encoders.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_api_call_encoders.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_array_size_2d.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_command_buffer_util.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_struct_encoders.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_struct_encoders.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_struct_handle_wrappers.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/custom_vulkan_struct_handle_wrappers.cpp
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_capture_manager.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_capture_manager.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_handle_wrappers.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_handle_wrapper_util.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_handle_wrapper_util.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_info.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_table_base.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_tracker.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_tracker_initializers.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_tracker.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_writer.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/encode/openxr_state_writer.cpp>
                   ${GFXRECON_SOURCE_DIR}/framework/encode/descriptor_update_template_info.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/handle_unwrap_memory.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/parameter_buffer.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/parameter_encoder.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/scoped_destroy_lock.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/scoped_destroy_lock.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/struct_pointer_encoder.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_capture_manager.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_capture_manager.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_device_address_tracker.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_device_address_tracker.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_entry_base.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_entry_base.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_handle_wrappers.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_handle_wrapper_util.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_handle_wrapper_util.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_state_info.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_state_table_base.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_state_tracker.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_state_tracker.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_state_writer.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_state_writer.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_track_struct.h
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_capture_common.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/encode/vulkan_capture_common.h
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_api_call_encoders.h
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_api_call_encoders.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_command_buffer_util.h
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_command_buffer_util.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_pnext_struct_encoder.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_struct_encoders.h
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_struct_encoders.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_struct_handle_wrappers.h
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_struct_handle_wrappers.cpp
                   ${GFXRECON_SOURCE_DIR}/framework/generated/generated_vulkan_state_table.h
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_api_call_encoders.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_api_call_encoders.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_struct_encoders.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_struct_encoders.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_struct_handle_wrappers.h>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_struct_handle_wrappers.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_struct_next_encoder.cpp>
                   $<$<BOOL:${OPENXR_SUPPORT_ENABLED}>:${GFXRECON_SOURCE_DIR}/framework/generated/generated_openxr_state_table.h>
              )

if (OPENXR_SUPPORT_ENABLED)
    target_include_directories(gfxrecon_encode PUBLIC ${GFXRECON_SOURCE_DIR}/external/OpenXR-SDK/include)
    target_compile_definitions(gfxrecon_encode PUBLIC ENABLE_OPENXR_SUPPORT=1)
endif()

target_include_directories(gfxrecon_encode
                           PUBLIC
                               ${CMAKE_BINARY_DIR}
                               ${GFXRECON_SOURCE_DIR}/framework)

target_link_libraries(gfxrecon_encode gfxrecon_graphics gfxrecon_format gfxrecon_util vulkan_registry platform_specific android)
