"""
 Copyright 2023 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """

import _dive

import pandas as pd
import numpy as np

from .command_hierarchy import CommandHierarchy
from .markers import Markers


class CaptureData:

    def __init__(self,
                 path):
        self._inner = _dive.CaptureData()
        self._command_hierarchy = None
        self._markers = None
        self._events = None
        self._submit_infos = None
        self._indirect_buffers = {}
        self._event_pipelines = None
        self._load_file(path)

    def _load_file(self, path):
        res = self._inner.load_file(path)
        if res != _dive.CaptureData.LoadResult.kSuccess:
            raise (Exception(str(res)))
        self._command_hierarchy = CommandHierarchy(self)

    @property
    def command_hierarchy(self):
        return self._command_hierarchy

    @property
    def markers(self):
        if self._markers is None:
            self._markers = Markers(self._inner.get_markers())
        return self._markers

    @property
    def submit_infos(self):
        if self._submit_infos is None:
            engine_type = []
            queue_type = []
            engine_index = []
            is_dummy_submit = []
            num_indirect_buffers = []

            for i in range(self._inner.get_num_submits()):
                submit_info = self._inner.get_submit_info(i)
                engine_type.append(submit_info.get_engine_type())
                queue_type.append(submit_info.get_queue_type())
                engine_index.append(submit_info.get_engine_index())
                is_dummy_submit.append(submit_info.is_dummy_submit())
                num_indirect_buffers.append(
                    submit_info.get_num_indirect_buffers())

            engine_type = _enum_to_categorical(engine_type)
            queue_type = _enum_to_categorical(queue_type)

            self._submit_infos = pd.DataFrame({
                'engine_type': engine_type,
                'queue_type': queue_type,
                'engine_index': engine_index,
                'is_dummy_submit': is_dummy_submit,
                'num_indirect_buffers': num_indirect_buffers,
            })

        return self._submit_infos.copy(deep=False)

    def _get_indirect_buffers(self, submit_index):
        submit_info = self._inner.get_submit_info(submit_index)
        va_addr = []
        size_in_dwords = []
        vmid = []
        is_constant_engine = []
        skip = []
        for i in range(submit_info.get_num_indirect_buffers()):
            ib = submit_info.get_indirect_buffer_info(i)
            va_addr.append(ib.va_addr)
            size_in_dwords.append(ib.size_in_dwords)
            vmid.append(ib.vmid)
            is_constant_engine.append(ib.is_constant_engine)
            skip.append(ib.skip)

        return pd.DataFrame({
            'va_addr': va_addr,
            'size_in_dwords': size_in_dwords,
            'vmid': vmid,
            'is_constant_engine': is_constant_engine,
            'skip': skip,
        })

    def get_indirect_buffers(self, submit_index=None):
        if submit_index not in self._indirect_buffers:
            ibs = None
            if submit_index is None:
                dfs = [
                    self._get_indirect_buffers(i)
                    for i in range(self._inner.get_num_submits())
                ]
                for i, df in enumerate(dfs):
                    df.insert(0, 'submit', i)
                ibs = pd.concat(dfs)
            else:
                ibs = self._get_indirect_buffers(submit_index)

            self._indirect_buffers[submit_index] = ibs

        return self._indirect_buffers[submit_index].copy(deep=False)

    indirect_buffers = property(get_indirect_buffers)


def _enum_to_categorical(values, count=None):
    if len(values) == 0:
        return values
    c = type(values[0])
    if count is None:
        count = c.kCount
    categories = [c(i).name[1:] for i in range(int(count))]
    arr = np.array(values, dtype=np.uint32)
    return pd.Categorical.from_codes(arr, categories=categories)
