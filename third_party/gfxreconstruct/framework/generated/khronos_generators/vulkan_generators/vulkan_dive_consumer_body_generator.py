#!/usr/bin/python3 -i
#
# Copyright 2025 Google LLC
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
from base_generator import BaseGenerator
from vulkan_base_generator import VulkanBaseGenerator, VulkanBaseGeneratorOptions, write
from khronos_dive_consumer_body_generator import KhronosExportDiveConsumerBodyGenerator
from reformat_code import format_cpp_code, indent_cpp_code, remove_trailing_newlines


class VulkanExportDiveConsumerBodyGeneratorOptions(VulkanBaseGeneratorOptions, KhronosExportDiveConsumerBodyGenerator):
    """Options for generating a C++ class for a GFXR Vulkan capture file to dive."""

    def __init__(
        self,
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

        self.begin_end_file_data.specific_headers.extend((
            'util/defines.h',
            'generated/generated_vulkan_dive_consumer.h',
            'decode/custom_vulkan_struct_to_json.h',
        ))
        self.begin_end_file_data.namespaces.extend(('gfxrecon', 'decode'))


class VulkanExportDiveConsumerBodyGenerator(VulkanBaseGenerator, KhronosExportDiveConsumerBodyGenerator, BaseGenerator):
    """VulkanExportDiveConsumerBodyGenerator - subclass of VulkanBaseGenerator.
    Generates C++ member definitions for the VulkanExportDiveConsumer class responsible for
    generating a textfile containing decoded Vulkan API call parameter data.
    Generate a C++ class for Vulkan capture file to Dive generation.
    """

    def __init__(
        self, err_file=sys.stderr, warn_file=sys.stderr, diag_file=sys.stdout
    ):
        VulkanBaseGenerator.__init__(
            self,
            err_file=err_file,
            warn_file=warn_file,
            diag_file=diag_file
        )

        self.customImplementationRequired = {
            'vkCmdBuildAccelerationStructuresIndirectKHR',
            'vkCmdPushConstants',
            'vkCreatePipelineCache',
            'vkCreateShaderModule',
            'vkGetPipelineCacheData',
        }

        self.formatAsHex = {
            'VkDeviceAddress',
        }

        # Parameters using this name should be output as handles even though they are uint64_t
        self.formatAsHandle = {
            'objectHandle',
        }

        self.queueSubmit = {
            "vkQueueSubmit",
            "vkQueueSubmit2",
            "vkQueueSubmit2KHR",
            }
    def endFile(self):
        """Method override."""
        # TODO: Each code generator is passed a blacklist like framework\generated\vulkan_generators\blacklists.json
        # of functions and structures not to generate code for. Once the feature is implemented, the following can be
        # replaced with adding vkCreateRayTracingPipelinesKHR in corresponding blacklist.
        if 'vkCreateRayTracingPipelinesKHR' in self.APICALL_BLACKLIST:
            self.APICALL_BLACKLIST.remove('vkCreateRayTracingPipelinesKHR')

        KhronosExportDiveConsumerBodyGenerator.generate_content(self)

        # Finish processing in superclass
        VulkanBaseGenerator.endFile(self)

    def need_feature_generation(self):
        """Indicates that the current feature has C++ code to generate."""
        if self.feature_cmd_params:
            return True
        return False

    def skip_generating_command_json(self, command):
        """Method override"""
        return command in self.customImplementationRequired

    def decode_as_handle(self, value):
        """Method override
        Indicates that the given type should be decoded as a handle."""
        return (
            (
                self.is_handle_like(value.base_type)
                or value.name in self.formatAsHandle
            )
        )

    def decode_as_hex(self, value):
        """Method override"""
        return value.base_type in self.formatAsHex

    def make_consumer_func_body(self, return_type, name, values):
        """Return class member function definition."""
        body = ''
        body += KhronosExportDiveConsumerBodyGenerator.make_consumer_func_body(self, return_type, name, values)
        if KhronosExportDiveConsumerBodyGenerator.is_command_buffer_cmd(self, name):
            body += f'    util::DiveFunctionData function_data("{name}", UpdateAndGetCommandBufferRecordIndex(commandBuffer), call_info.index, args);\n'
        elif name in self.queueSubmit:
            # cmd_buffer_index is 0. The submit index is determined when processing the submits on the Dive side of things.
            body += f'    util::DiveFunctionData function_data("{name}", 0, call_info.index, args);\n'
        return body
