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

import sys, inspect
from vulkan_consumer_header_generator import VulkanConsumerHeaderGenerator, VulkanConsumerHeaderGeneratorOptions, write


class VulkanExportDiveConsumerHeaderGeneratorOptions(VulkanConsumerHeaderGeneratorOptions):
    """Options for generating a C++ class for a GFXR Vulkan capture file to dive."""

    def __init__(
        self,
        class_name,
        base_class_header,
        is_override,
        constructor_args='',
        blacklists=None,  # Path to JSON file listing apicalls and structs to ignore.
        platform_types=None,  # Path to JSON file listing platform (WIN32, X11, etc.) defined types.
        filename=None,
        directory='.',
        prefix_text='',
        protect_file=False,
        protect_feature=True,
        extra_headers=[]
    ):
        VulkanConsumerHeaderGeneratorOptions.__init__(
            self,
            class_name,
            base_class_header,
            is_override,
            constructor_args,
            blacklists,
            platform_types,
            filename,
            directory,
            prefix_text,
            protect_file,
            protect_feature,
            extra_headers=extra_headers
        )

class VulkanExportDiveConsumerHeaderGenerator(VulkanConsumerHeaderGenerator):
    """VulkanExportDiveConsumerHeaderGenerator - subclass of VulkanConsumerHeaderGenerator.
    Generate a C++ class for Vulkan capture file to JSON file generation.
    """

    def __init__(
        self, err_file=sys.stderr, warn_file=sys.stderr, diag_file=sys.stdout
    ):
        VulkanConsumerHeaderGenerator.__init__(
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

    def beginFile(self, gen_opts):
        VulkanConsumerHeaderGenerator.beginFile(self, gen_opts)

        # TODO: Each code generator is passed a blacklist like framework\generated\vulkan_generators\blacklists.json
        # of functions and structures not to generate code for. Once the feature is implemented, the following can be
        # replaced with adding vkCreateRayTracingPipelinesKHR in corresponding blacklist.
        if 'vkCreateRayTracingPipelinesKHR' in self.APICALL_BLACKLIST:
            self.APICALL_BLACKLIST.remove('vkCreateRayTracingPipelinesKHR')

    def write_class_contents(self):
        """
        Method Override
        Performs C++ code generation for the feature.
        """
        for cmd in self.get_all_filtered_cmd_names():
            if cmd not in self.customImplementationRequired:
                info = self.all_cmd_params[cmd]
                return_type = info[0]
                values = info[2]

                decl = self.make_consumer_func_decl(
                    return_type, 'Process_' + cmd, values
                )

                cmddef = '\n'
                if self.genOpts.is_override:
                    cmddef += self.indent(
                        'virtual ' + decl + ' override;', self.INDENT_SIZE
                    )
                else:
                    cmddef += self.indent(
                        'virtual ' + decl + ' {}', self.INDENT_SIZE
                    )

                write(cmddef, file=self.outFile)
