#!/usr/bin/python3 -i
#
# Copyright (c) 2019-2020 Valve Corporation
# Copyright (c) 2019-2024 LunarG, Inc.
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
from base_generator import write


class BaseStructHandleMappersHeaderGenerator():
    """Base class for generating struct handle mappers header code."""

    def endFile(self):
        platform_type = self.get_api_prefix()

        self.newline()
        if platform_type == 'Vulkan':
            write(
                f'void MapPNextStructHandles(const void* value, void* wrapper, const CommonObjectInfoTable& object_info_table);',
                file=self.outFile
            )
        else:
            write(
                f'void MapNextStructHandles(const void* value, void* wrapper, const CommonObjectInfoTable& object_info_table);',
                file=self.outFile
            )

        self.newline()

        for struct in self.output_structs_with_handles:
            write(
                'void AddStructHandles(format::HandleId parent_id, const Decoded_{type}* id_wrapper, const {type}* handle_struct, CommonObjectInfoTable* object_info_table);'
                .format(type=struct),
                file=self.outFile
            )
            self.newline()

        for struct in self.output_structs_with_handles:
            if struct in self.structs_with_handle_ptrs:
                write(
                    'void SetStructHandleLengths(Decoded_{type}* wrapper);'
                    .format(type=struct),
                    file=self.outFile
                )
                self.newline()

        write('#include "decode/common_struct_handle_mappers.h"', file=self.outFile)

        write('GFXRECON_END_NAMESPACE(decode)', file=self.outFile)
        write('GFXRECON_END_NAMESPACE(gfxrecon)', file=self.outFile)

    def generate_feature(self):
        """Performs C++ code generation for the feature."""
        for struct in self.get_filtered_struct_names():
            if (
                (struct in self.structs_with_handles)
                or (struct in self.GENERIC_HANDLE_STRUCTS)
                or (struct in self.structs_with_map_data)
            ) and (struct not in self.STRUCT_MAPPERS_BLACKLIST):
                body = '\n'
                body += 'void MapStructHandles(Decoded_{}* wrapper, const CommonObjectInfoTable& object_info_table);'.format(
                    struct
                )
                write(body, file=self.outFile)