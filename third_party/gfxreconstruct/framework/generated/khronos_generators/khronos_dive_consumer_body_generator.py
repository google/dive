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
from khronos_base_generator import write
from reformat_code import format_cpp_code, indent_cpp_code, remove_trailing_newlines

class KhronosExportDiveConsumerBodyGenerator():
    """KhronosExportDiveConsumerBodyGenerator
    Generates C++ member definitions for the KhronosExportDiveConsumer class responsible for
    displaying decoded Khronos API call parameter data in dive.
    """
    def is_command_buffer_cmd(self, command):
        if 'vkCmd' in command:
            return True
        return False

    def generate_content(self):
        """Method may be overridden"""
        api_data = self.get_api_data()
        export_prefix = '{}ExportDiveConsumer::Process_'.format(
            api_data.api_class_prefix
        )

        write(
            remove_trailing_newlines(
                indent_cpp_code(
                    '''
            using util::JsonOptions;
        '''
                )
            ),
            file=self.outFile
        )

        for cmd in self.get_all_filtered_cmd_names():
            if self.skip_generating_command_json(cmd):
                continue

            info = self.all_cmd_params[cmd]
            return_type = info[0]
            values = info[2]

            cmddef = '\n'
            cmddef += self.make_consumer_func_decl(
                return_type, export_prefix + cmd, values
            ) + '\n'
            cmddef += format_cpp_code(
                '''
                {
                '''
            )
            # Only include full processing for vulkan commands and vkQueueSubmits since those are what are displayed in dive.
            if self.is_command_buffer_cmd(cmd) or cmd in self.queueSubmit:
                cmddef += '\n'
                cmddef += format_cpp_code(
                    '''
                        nlohmann::ordered_json dive_data;
                        const JsonOptions json_options;
                    ''', 1
                )
            cmddef += '\n'
            # Only include this for vulkan commands since those are what are displayed in dive.
            cmddef += self.make_consumer_func_body(return_type, cmd, values)
            if self.is_command_buffer_cmd(cmd) or cmd in self.queueSubmit:
                cmddef += format_cpp_code(
                '''
                    WriteBlockEnd(function_data);
                }
                ''', 1
                )
            else:
                cmddef += format_cpp_code(
                '''
                }
                ''', 1
                )
            write(cmddef, file=self.outFile)

    def skip_generating_command_json(self, command):
        """Method may be overriden"""
        return False

    def decode_as_handle(self, value):
        """Method may be overridden.
        Indicates that the given type should be decoded as a handle."""
        return self.is_handle_like(value.base_type)

    def decode_as_hex(self, value):
        """Method may be overriden"""
        return False

    def make_consumer_func_body(self, return_type, name, values):
        """Return ExportDiveConsumer class member function definition."""
        body = ''

        if (len(values) > 0 and self.is_command_buffer_cmd(name)) or name in self.queueSubmit:
            body += f'    auto& args = dive_data["args"];\n'
            # Handle function arguments
            for value in values:
                flagsEnumType = value.base_type

                # Default to letting the right function overload to be resolved based on argument types,
                # including enums, strings ints, floats etc.:
                # Note there are overloads for scalars and pointers/arrays.
                to_dive = 'FieldToJson(args["{0}"], {0}, json_options)'

                # Special cases:
                if self.is_boolean_type(value.base_type):
                    to_dive = 'Bool32ToJson(args["{0}"], {0}, json_options)'
                elif value.name == 'ppData' or self.decode_as_hex(value):
                    to_dive = 'FieldToJsonAsHex(args["{0}"], {0}, json_options)'
                elif self.decode_as_handle(value):
                    to_dive = 'HandleToJson(args["{0}"], {0}, json_options)'
                elif self.is_flags(value.base_type):
                    if value.base_type in self.flags_type_aliases:
                        flagsEnumType = self.flags_type_aliases[value.base_type
                                                                ]
                    if not (value.is_pointer or value.is_array):
                        to_dive = 'FieldToJson({2}_t(), args["{0}"], {0}, json_options)'
                    else:
                        # Default to outputting as the raw type but warn:
                        print(
                            "Missing conversion of pointers to",
                            flagsEnumType,
                            "in",
                            name,
                            file=sys.stderr
                        )

                to_dive = to_dive.format(
                    value.name, value.base_type, flagsEnumType
                )
                body += '    {0};\n'.format(to_dive)
        return body
