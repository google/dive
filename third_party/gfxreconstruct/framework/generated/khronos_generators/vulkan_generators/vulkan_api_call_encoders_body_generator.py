#!/usr/bin/python3 -i
#
# Copyright (c) 2018-2019 Valve Corporation
# Copyright (c) 2018-2019 LunarG, Inc.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import sys
from vulkan_base_generator import VulkanBaseGenerator, VulkanBaseGeneratorOptions, write
from khronos_api_call_encoders_generator import KhronosApiCallEncodersGenerator

class VulkanApiCallEncodersBodyGeneratorOptions(VulkanBaseGeneratorOptions):
    """Options for generating C++ functions for Vulkan API parameter encoding."""

    def __init__(
        self,
        capture_overrides=None,  # Path to JSON file listing Vulkan API calls to override on capture.
        blacklists=None,  # Path to JSON file listing apicalls and structs to ignore.
        platform_types=None,  # Path to JSON file listing platform (WIN32, X11, etc.) defined types.
        filename=None,
        directory='.',
        prefix_text='',
        protect_file=False,
        protect_feature=True,
        extra_headers=[]
    ):
        VulkanBaseGeneratorOptions.__init__(
            self,
            blacklists,
            platform_types,
            filename,
            directory,
            prefix_text,
            protect_file,
            protect_feature,
            extra_headers=extra_headers
        )
        self.capture_overrides = capture_overrides

        begin_end = self.begin_end_file_data
        begin_end.specific_headers.extend((
            'generated/generated_vulkan_api_call_encoders.h',
            '',
            'encode/custom_vulkan_encoder_commands.h',
            'encode/custom_vulkan_array_size_2d.h',
            'encode/parameter_encoder.h',
            'encode/struct_pointer_encoder.h',
            'encode/vulkan_capture_manager.h',
            'encode/vulkan_handle_wrapper_util.h',
            'encode/vulkan_handle_wrappers.h',
            'format/api_call_id.h',
            'generated/generated_vulkan_command_buffer_util.h',
            'generated/generated_vulkan_struct_handle_wrappers.h',
            'util/defines.h',
        ))
        begin_end.namespaces.extend(('gfxrecon', 'encode'))


class VulkanApiCallEncodersBodyGenerator(VulkanBaseGenerator, KhronosApiCallEncodersGenerator):
    """VulkanApiCallEncodersBodyGenerator - subclass of VulkanBaseGenerator.
    Generates C++ functions responsible for encoding Vulkan API call
    parameter data.
    Generate C++ functions for Vulkan API parameter encoding.
    """

    # Map of Vulkan function names to override function names.  Calls to Vulkan functions in the map
    # will be replaced by the override value.
    CAPTURE_OVERRIDES = {}

    # Functions that can activate trimming from a pre call command.
    PRECALL_TRIM_TRIGGERS = ['vkQueueSubmit', 'vkQueueSubmit2', 'vkQueueSubmit2KHR']

    # Functions that can activate trimming from a post call command.
    POSTCALL_TRIM_TRIGGERS = ['vkQueueSubmit', 'vkQueueSubmit2', 'vkQueueSubmit2KHR', 'vkQueuePresentKHR', 'vkFrameBoundaryANDROID']

    CHECK_WRITE = ['vkWaitForPresentKHR']

    def __init__(
        self, err_file=sys.stderr, warn_file=sys.stderr, diag_file=sys.stdout
    ):
        VulkanBaseGenerator.__init__(
            self,
            err_file=err_file,
            warn_file=warn_file,
            diag_file=diag_file
        )
        KhronosApiCallEncodersGenerator.__init__(self, check_write=['vkWaitForPresentKHR'])


    def beginFile(self, gen_opts):
        """Method override."""
        VulkanBaseGenerator.beginFile(self, gen_opts)

        if gen_opts.capture_overrides:
            self.load_capture_overrides(gen_opts.capture_overrides)

    def endFile(self):
        """Method override."""
        self.write_api_call_encoders_contents(make_cmd_body=self.make_cmd_body )

        # Finish processing in superclass
        VulkanBaseGenerator.endFile(self)

    def use_instance_table(self, name, typename):
        """Check for dispatchable handle types associated with the instance dispatch table."""
        if typename in ['VkInstance', 'VkPhysicalDevice']:
            return True
        # vkSetDebugUtilsObjectNameEXT and vkSetDebugUtilsObjectTagEXT
        # need to be probed from GetInstanceProcAddress due to a loader issue.
        # https://github.com/KhronosGroup/Vulkan-Loader/issues/1109
        # TODO : When loader with fix for issue is widely available, remove this
        # special case.
        if name in ['vkSetDebugUtilsObjectNameEXT', 'vkSetDebugUtilsObjectTagEXT']:
            return True
        return False

    def make_layer_dispatch_call(self, name, values, arg_list):
        """Generate the layer dispatch call invocation."""
        call_setup_expr = []
        object_name = values[0].name
        wrapper_prefix = self.get_wrapper_prefix_from_command(name)
        if self.use_instance_table(name, values[0].base_type):
            dispatchfunc = '{}::GetInstanceTable'.format(wrapper_prefix)
            if values[0].base_type == 'VkDevice':
                object_name = 'physical_device'
                call_setup_expr.append("auto device_wrapper = {0}::GetWrapper<{0}::DeviceWrapper>({1});".format(wrapper_prefix, values[0].name))
                call_setup_expr.append("auto physical_device = device_wrapper->physical_device->handle;")
        else:
            dispatchfunc = '{}::GetDeviceTable'.format(wrapper_prefix)

        return [call_setup_expr, '{}({})->{}({})'.format(dispatchfunc, object_name, name[2:], arg_list)]

    def make_cmd_body(self, return_type, name, values):
        """Command definition."""
        indent = ' ' * self.INDENT_SIZE
        is_override = name in self.CAPTURE_OVERRIDES
        encode_after = False
        omit_output_param = None
        has_outputs = self.has_outputs(return_type, values)
        arg_list = self.make_arg_list(values)

        capture_manager = 'manager'
        if name == "vkCreateInstance":
            capture_manager = 'VulkanCaptureManager::Get()'
        body = ''
        if name != "vkCreateInstance":
            body += indent + 'VulkanCaptureManager* manager = VulkanCaptureManager::Get();\n'
            body += indent + 'GFXRECON_ASSERT(manager != nullptr);\n'
        if name == "vkCreateInstance":
            body += indent + 'auto api_call_lock = VulkanCaptureManager::AcquireExclusiveApiCallLock();\n'
        else:
            # GOOGLE: Leak fragment density maps to workaround a specific crash
            if name == "vkDestroyImage":
                body += indent + '// GOOGLE: Leak fragment density maps to workaround a specific crash\n'
                body += indent + 'auto wrapper = vulkan_wrappers::GetWrapper<vulkan_wrappers::ImageWrapper>(image);\n'
                body += indent + 'if (!wrapper) return;\n'
                body += indent + 'if (wrapper->is_fdm)\n'
                body += indent + '{\n'
                body += indent + '    GFXRECON_LOG_WARNING("vkDestroyImage: %p - refusing to destroy FDM given known unity bugs", image);\n'
                body += indent + '    return;\n'
                body += indent + '}\n'
            body += indent + 'auto force_command_serialization = manager->GetForceCommandSerialization();\n'
            body += indent + 'std::shared_lock<CommonCaptureManager::ApiCallMutexT> shared_api_call_lock;\n'
            body += indent + 'std::unique_lock<CommonCaptureManager::ApiCallMutexT> exclusive_api_call_lock;\n'
            body += indent + 'if (force_command_serialization)\n'
            body += indent + '{\n'
            body += indent + '    exclusive_api_call_lock = VulkanCaptureManager::AcquireExclusiveApiCallLock();\n'
            body += indent + '}\n'
            body += indent + 'else\n'
            body += indent + '{\n'
            body += indent + '    shared_api_call_lock = VulkanCaptureManager::AcquireSharedApiCallLock();\n'
            body += indent + '}\n'

        body += '\n'

        if has_outputs or (return_type and return_type != 'void'):
            encode_after = True

        if has_outputs and (return_type and return_type != 'void'):
            omit_output_param = 'omit_output_data'
            body += indent + 'bool omit_output_data = false;\n'
            body += '\n'

        if name in self.PRECALL_TRIM_TRIGGERS:
            body += indent + 'CustomEncoderPreCall<format::ApiCallId::ApiCall_{}>::Dispatch({}, shared_api_call_lock, {});\n'.format(
                name, capture_manager, arg_list
            )
        else:
            body += indent + 'CustomEncoderPreCall<format::ApiCallId::ApiCall_{}>::Dispatch({}, {});\n'.format(
                name, capture_manager, arg_list
            )

        if not encode_after:
            body += self.make_parameter_encoding(
                name, values, return_type, indent, omit_output_param
            )

        body += '\n'

        if is_override:
            # Capture overrides simply call the override function without handle unwrap/wrap
            # Construct the function call to dispatch to the next layer.
            call_expr = '{}({})'.format(
                self.CAPTURE_OVERRIDES[name], self.make_arg_list(values)
            )
            if return_type and return_type != 'void':
                body += indent + '{} result = {};\n'.format(
                    return_type, call_expr
                )
            else:
                body += indent + '{};\n'.format(call_expr)

            if has_outputs and (return_type and return_type != 'void'):
                body += indent + 'if (result < 0)\n'
                body += indent + '{\n'
                body += indent + '    omit_output_data = true;\n'
                body += indent + '}\n'
        else:
            # Check for handles that need unwrapping.
            unwrap_expr, unwrapped_arg_list, need_unwrap_memory = self.make_handle_unwrapping(
                name, values, indent
            )
            if unwrap_expr:
                if need_unwrap_memory:
                    body += indent + f'auto handle_unwrap_memory = {capture_manager}->GetHandleUnwrapMemory();\n'
                body += unwrap_expr
                body += '\n'

            if self.lock_for_destroy_handle_is_needed(name):
                body += indent + 'ScopedDestroyLock exclusive_scoped_lock;\n'

            # Construct the function call to dispatch to the next layer.
            (call_setup_expr, call_expr) = self.make_layer_dispatch_call(
                name, values, unwrapped_arg_list
            )
            if call_setup_expr:
                for e in call_setup_expr:
                    body += indent + e + '\n'
            if return_type and return_type != 'void':
                body += indent + '{} result = {};\n'.format(
                    return_type, call_expr
                )
            else:
                body += indent + '{};\n'.format(call_expr)

            # Wrap newly created handles.
            wrap_expr = self.make_handle_wrapping(values, indent)
            if wrap_expr:
                body += '\n'
                if return_type and return_type != 'void':
                    body += indent + 'if (result >= 0)\n'
                    body += indent + '{\n'
                    body += '    ' + wrap_expr
                    body += indent + '}\n'
                    if has_outputs:
                        body += indent + 'else\n'
                        body += indent + '{\n'
                        body += indent + '    omit_output_data = true;\n'
                        body += indent + '}\n'
                else:
                    body += wrap_expr
            elif has_outputs and (return_type and return_type != 'void'):
                body += indent + 'if (result < 0)\n'
                body += indent + '{\n'
                body += indent + '    omit_output_data = true;\n'
                body += indent + '}\n'

        if encode_after:
            body += self.make_parameter_encoding(
                name, values, return_type, indent, omit_output_param
            )

        body += '\n'
        if return_type and return_type != 'void':
            if name in self.POSTCALL_TRIM_TRIGGERS:
                body += '    CustomEncoderPostCall<format::ApiCallId::ApiCall_{}>::Dispatch({}, shared_api_call_lock, result, {});\n'.format(
                    name, capture_manager, arg_list
                )
            else:
                body += '    CustomEncoderPostCall<format::ApiCallId::ApiCall_{}>::Dispatch({}, result, {});\n'.format(
                    name, capture_manager, arg_list
                )
        else:
            if name in self.POSTCALL_TRIM_TRIGGERS:
                body += '    CustomEncoderPostCall<format::ApiCallId::ApiCall_{}>::Dispatch({}, shared_api_call_lock, {});\n'.format(
                    name, capture_manager, arg_list
                )
            else:
                body += '    CustomEncoderPostCall<format::ApiCallId::ApiCall_{}>::Dispatch({}, {});\n'.format(
                    name, capture_manager, arg_list
                )

        cleanup_expr = self.make_handle_cleanup(name, values, indent)
        if cleanup_expr:
            body += '\n'
            body += cleanup_expr

        if return_type and return_type != 'void':
            body += '\n'
            body += '    return result;\n'

        return body

    def make_begin_api_call(self, name, values):
        capture_manager = 'manager'
        if name == 'vkCreateInstance':
            capture_manager = 'VulkanCaptureManager::Get()'

        if name.startswith('vkCreate') or name.startswith(
            'vkAllocate'
        ) or name.startswith('vkDestroy') or name.startswith(
            'vkFree'
        ) or name.startswith('vkSetDebugUtilsObject'
        ) or self.retrieves_handles(values) or (
            values[0].base_type == 'VkCommandBuffer'
        ) or (name == 'vkReleasePerformanceConfigurationINTEL'):
            return 'auto encoder = {}->BeginTrackedApiCallCapture(format::ApiCallId::ApiCall_{});\n'.format(
                capture_manager, name
            )
        else:
            return 'auto encoder = {}->BeginApiCallCapture(format::ApiCallId::ApiCall_{});\n'.format(
                capture_manager, name
            )

    def make_end_api_call(self, name, values, return_type):
        decl = 'manager->'
        if name == 'vkCreateInstance':
            decl = 'VulkanCaptureManager::Get()->'

        wrapper_prefix = self.get_wrapper_prefix_from_command(name)

        if name.startswith('vkCreate') or name.startswith(
            'vkAllocate'
        ) or self.retrieves_handles(values):
            # The handle is the last parameter.
            handle = values[-1]
            parent_handle = values[0] if self.is_handle(
                values[0].base_type
            ) else None

            #  Search for the create info struct
            info_base_type = 'void'
            info_name = 'nullptr'
            for value in values:
                if (
                    ('CreateInfo' in value.base_type)
                    or ('AllocateInfo' in value.base_type)
                ):
                    info_base_type = value.base_type
                    info_name = value.name
                    # Confirm array counts match
                    if value.is_array and (
                        handle.array_length != value.array_length
                    ):
                        print(
                            'WARNING: {} has separate array counts for create info structures ({}) and handles ({})'
                            .format(name, value.array_length, count)
                        )

            return_value = 'VK_SUCCESS'
            if return_type == 'VkResult':
                return_value = 'result'

            if handle.is_array:
                length_name = self.make_array_length_expression(handle)

                if 'pAllocateInfo->' in length_name:
                    # This is a pool allocation call, which receives one allocate info structure that is shared by all object being allocated.
                    decl += 'EndPoolCreateApiCallCapture<{}, {}::{}Wrapper, {}>({}, {}, {}, {}, {})'.format(
                        parent_handle.base_type, wrapper_prefix, handle.base_type[2:],
                        info_base_type, return_value, parent_handle.name,
                        length_name, handle.name, info_name
                    )
                else:
                    # This is a multi-object creation call (e.g. pipeline creation, or swapchain image retrieval), which receives
                    # separate create info structures for each object being created. Many multi-object creation calls receive a
                    # handle as their second parameter, which is of interest to the state tracker (e.g. the VkPipelineCache handle
                    # from vkCreateGraphicsPipelines or the vkSwapchain handle from vkGetSwapchainImagesKHR). For api calls that do
                    # not receive a handle as the second parameter (e.g. vkEnumeratePhysicalDevices), the handle type is set to 'void*'.
                    if handle.base_type in self.struct_names:
                        # "handle" is actually a struct with embedded handles
                        unwrap_handle_def = 'nullptr'
                        member_handle_type, member_handle_name, member_array_length = self.get_struct_handle_member_info(
                            self.structs_with_handles[handle.base_type]
                        )

                        if not member_array_length:
                            unwrap_handle_def = '[]({}* handle_struct)->{wrapper_prefix}::{wrapper}Wrapper* {{ return vulkan_wrappers::GetWrapper<{wrapper_prefix}::{wrapper}Wrapper>(handle_struct->{}); }}'.format(
                                handle.base_type,
                                member_handle_name,
                                wrapper_prefix=wrapper_prefix,
                                wrapper=member_handle_type[2:]
                            )

                        decl += 'EndStructGroupCreateApiCallCapture<{}, {}::{}Wrapper, {}>({}, {}, {}, {}, {})'.format(
                            parent_handle.base_type,
                            wrapper_prefix,
                            member_handle_type[2:],
                            handle.base_type,
                            return_value,
                            parent_handle.name,
                            length_name,
                            handle.name,
                            unwrap_handle_def
                        )
                    elif self.is_handle(values[1].base_type):
                        second_handle = values[1]
                        decl += 'EndGroupCreateApiCallCapture<{}, {}, {}::{}Wrapper, {}>({}, {}, {}, {}, {}, {})'.format(
                            parent_handle.base_type,
                            second_handle.base_type,
                            wrapper_prefix,
                            handle.base_type[2:],
                            info_base_type, return_value,
                            parent_handle.name,
                            second_handle.name,
                            length_name,
                            handle.name,
                            info_name
                        )
                    else:
                        decl += 'EndGroupCreateApiCallCapture<{}, void*, {}::{}Wrapper, {}>({}, {}, nullptr, {}, {}, {})'.format(
                            parent_handle.base_type,
                            wrapper_prefix,
                            handle.base_type[2:],
                            info_base_type,
                            return_value,
                            parent_handle.name,
                            length_name,
                            handle.name,
                            info_name
                        )

            else:
                if handle.base_type in self.struct_names:
                    length_name = None
                    for mem in self.all_struct_members[handle.base_type]:
                        # Assuming only one member is_array
                        if mem.is_array:
                            length_name = '{}->{}'.format(handle.name, mem.array_length)
                    if length_name == None:
                        # No member of the structure was an array
                        # Shouldn't happen
                        raise NotImplementedError


                    # "handle" is actually a struct with embedded handles
                    unwrap_handle_def = 'nullptr'
                    member_handle_type, member_handle_name, member_array_length = self.get_struct_handle_member_info(
                        self.structs_with_handles[handle.base_type]
                    )

                    if not member_array_length:
                        unwrap_handle_def = '[]({}* handle_struct)->{wrapper_prefix}::{wrapper}Wrapper* {{ return vulkan_wrappers::GetWrapper<{wrapper_prefix}::{wrapper}Wrapper>(handle_struct->{}); }}'.format(
                            handle.base_type,
                            member_handle_name,
                            wrapper_prefix=wrapper_prefix,
                            wrapper=member_handle_type[2:]
                        )


                    decl += 'EndStructGroupCreateApiCallCapture<{}, {}::{}Wrapper, {}>({}, {}, {}, {}, {})'.format(
                        parent_handle.base_type,
                        wrapper_prefix,
                        member_handle_type[2:],
                        handle.base_type,
                        return_value,
                        parent_handle.name,
                        length_name,
                        handle.name,
                        unwrap_handle_def
                    )

                elif parent_handle:
                    decl += 'EndCreateApiCallCapture<{}, {}::{}Wrapper, {}>({}, {}, {}, {})'.format(
                        parent_handle.base_type,
                        wrapper_prefix,
                        handle.base_type[2:],
                        info_base_type,
                        return_value,
                        parent_handle.name,
                        handle.name,
                        info_name
                    )
                else:
                    # Instance creation does not have a parent handle; set the parent handle type to 'void*'.
                    decl += 'EndCreateApiCallCapture<const void*, {}::{}Wrapper, {}>({}, nullptr, {}, {})'.format(
                        wrapper_prefix,
                        handle.base_type[2:],
                        info_base_type,
                        return_value,
                        handle.name,
                        info_name
                    )

        elif name.startswith('vkDestroy') or name.startswith('vkFree') or (
            name == 'vkReleasePerformanceConfigurationINTEL'
        ):
            handle = None
            if name in ['vkDestroyInstance', 'vkDestroyDevice']:
                # Instance/device destroy calls are special case where the target handle is the first parameter
                handle = values[0]
            else:
                # The destroy target is the second parameter, except for pool based allocations where it is the last parameter.
                handle = values[1]
                if ("Pool" in handle.base_type) and name.startswith('vkFree'):
                    handle = values[3]

            if handle.is_array:
                decl += 'EndDestroyApiCallCapture<{}::{}Wrapper>({}, {})'.format(
                    wrapper_prefix, handle.base_type[2:], handle.array_length, handle.name
                )
            else:
                decl += 'EndDestroyApiCallCapture<{}::{}Wrapper>({})'.format(
                    wrapper_prefix, handle.base_type[2:], handle.name
                )

        elif values[0].base_type == 'VkCommandBuffer':
            get_handles_expr = self.make_get_command_handles_expr(
                name, values[1:]
            )
            if get_handles_expr:
                decl += 'EndCommandApiCallCapture({}, {})'.format(
                    values[0].name, get_handles_expr
                )
            else:
                decl += 'EndCommandApiCallCapture({})'.format(values[0].name)
        else:
            decl += 'EndApiCallCapture()'

        decl += ';\n'
        return decl

    def make_handle_wrapping(self, values, indent):
        expr = ''

        for value in values:
            wrapper_prefix = self.get_wrapper_prefix_from_type(value.base_type)
            if self.is_output_parameter(value) and (
                self.is_handle(value.base_type) or (
                    self.is_struct(value.base_type) and
                    (value.base_type in self.structs_with_handles)
                )
            ):
                # The VkInstance handle does not have parent, so the 'unused'
                # values will be provided to the wrapper creation function.
                parent_type = 'vulkan_wrappers::NoParentWrapper'
                parent_value = 'vulkan_wrappers::NoParentWrapper::kHandleValue'
                if self.is_handle(values[0].base_type):
                    parent_type = wrapper_prefix + '::' + values[0].base_type[2:] + 'Wrapper'
                    parent_value = values[0].name

                # Some handles have two parent handles, such as swapchain images and display modes,
                # or command buffers and descriptor sets allocated from pools.
                co_parent_type = 'vulkan_wrappers::NoParentWrapper'
                co_parent_value = 'vulkan_wrappers::NoParentWrapper::kHandleValue'
                if self.is_handle(values[1].base_type):
                    co_parent_type = wrapper_prefix + '::' + values[1].base_type[2:] + 'Wrapper'
                    co_parent_value = values[1].name
                elif values[1].base_type.endswith(
                    'AllocateInfo'
                ) and value.is_array and ('->' in value.array_length):
                    # An array of handles with length specified by an AllocateInfo structure (there is a -> in the length name) is a pool allocation.
                    # Extract the pool handle from the AllocateInfo structure, which is currently the first and only handle member.
                    members = self.structs_with_handles[values[1].base_type]
                    for member in members:
                        if self.is_handle(member.base_type):
                            co_parent_type = wrapper_prefix + '::' + member.base_type[2:] + 'Wrapper'
                            co_parent_value = values[
                                1].name + '->' + member.name
                            break

                if value.is_array:
                    length_name = value.array_length
                    for len in values:
                        if (len.name == length_name) and len.is_pointer:
                            length_name = '({name} != nullptr) ? (*{name}) : 0'.format(
                                name=length_name
                            )
                            break
                    if self.is_handle(value.base_type):
                        expr += indent + '{}::CreateWrappedHandles<{}, {}, {}::{}Wrapper>({}, {}, {}, {}, VulkanCaptureManager::GetUniqueId);\n'.format(
                            wrapper_prefix, parent_type, co_parent_type, wrapper_prefix, value.base_type[2:],
                            parent_value, co_parent_value, value.name,
                            length_name
                        )
                    elif self.is_struct(
                        value.base_type
                    ) and (value.base_type in self.structs_with_handles):
                        expr += indent + '{}::CreateWrappedStructArrayHandles<{}, {}, {}>({}, {}, {}, {}, VulkanCaptureManager::GetUniqueId);\n'.format(
                            wrapper_prefix, parent_type, co_parent_type, value.base_type,
                            parent_value, co_parent_value, value.name,
                            length_name
                        )
                else:
                    if self.is_handle(value.base_type):
                        expr += indent + '{}::CreateWrappedHandle<{}, {}, {}::{}Wrapper>({}, {}, {}, VulkanCaptureManager::GetUniqueId);\n'.format(
                            wrapper_prefix, parent_type, co_parent_type, wrapper_prefix, value.base_type[2:],
                            parent_value, co_parent_value, value.name
                        )
                    elif self.is_struct(
                        value.base_type
                    ) and (value.base_type in self.structs_with_handles):
                        expr += indent + '{}::CreateWrappedStructHandles<{}, {}>({}, {}, {}, VulkanCaptureManager::GetUniqueId);\n'.format(
                            wrapper_prefix, parent_type, co_parent_type, parent_value,
                            co_parent_value, value.name
                        )
        return expr

    def make_handle_unwrapping(self, name, values, indent):
        args = []
        expr = ''
        need_unwrap_memory = False
        for value in values:
            wrapper_prefix = self.get_wrapper_prefix_from_type(value.base_type)
            arg_name = value.name
            if value.is_pointer or value.is_array:
                if self.is_input_pointer(value):
                    if (value.base_type in self.structs_with_handles) or (
                        value.base_type in self.GENERIC_HANDLE_STRUCTS
                    ):
                        need_unwrap_memory = True
                        arg_name += '_unwrapped'
                        if value.is_array:
                            expr += indent + '{} {name}_unwrapped = {}::UnwrapStructArrayHandles({name}, {}, handle_unwrap_memory);\n'.format(
                                value.full_type,
                                wrapper_prefix, 
                                value.array_length,
                                name=value.name
                            )
                        else:
                            expr += indent + '{} {name}_unwrapped = {}::UnwrapStructPtrHandles({name}, handle_unwrap_memory);\n'.format(
                                value.full_type, wrapper_prefix, name=value.name
                            )
            args.append(arg_name)
        return expr, ', '.join(args), need_unwrap_memory

    def lock_for_destroy_handle_is_needed(self, name):
        if name.startswith('vkDestroy') or name.startswith('vkFree') or (
            name == 'vkReleasePerformanceConfigurationINTEL'
        ) or (name == 'vkResetDescriptorPool'):
            return True
        else:
            return False

    def make_handle_cleanup(self, name, values, indent):
        expr = ''
        if name.startswith('vkDestroy') or name.startswith('vkFree') or (
            name == 'vkReleasePerformanceConfigurationINTEL'
        ):
            handle = None
            if name in ['vkDestroyInstance', 'vkDestroyDevice']:
                # Instance/device destroy calls are special case where the target handle is the first parameter
                handle = values[0]
            else:
                # The destroy target is the second parameter, except for pool based allocations where it is the last parameter.
                handle = values[1]
                if ("Pool" in handle.base_type) and name.startswith('vkFree'):
                    handle = values[3]

            wrapper_prefix = self.get_wrapper_prefix_from_command(name)

            if handle.is_array:
                expr += indent + '{}::DestroyWrappedHandles<{}::{}Wrapper>({}, {});\n'.format(
                    wrapper_prefix, wrapper_prefix, handle.base_type[2:], handle.name, handle.array_length
                )
            else:
                expr += indent + '{}::DestroyWrappedHandle<{}::{}Wrapper>({});\n'.format(
                    wrapper_prefix, wrapper_prefix, handle.base_type[2:], handle.name
                )
        return expr

    def retrieves_handles(self, values):
        """Determine if an API call indirectly creates handles by retrieving them(e.g. vkEnumeratePhysicalDevices, vkGetRandROutputDisplayEXT)"""
        for value in values:
            if self.is_output_parameter(value) and (
                self.is_handle(value.base_type) or
                (value.base_type in self.structs_with_handles)
            ):
                return True
        return False

