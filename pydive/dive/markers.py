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

import pandas as pd

class Markers:
    def __init__(self, inner):
        self._inner = inner
        self._events = None
        self._command_buffers = None
        self._barriers = None
        self._labels = None
        self._layout_transitions = None
        self._pipeline_binds = None
        self._sub_events = None

    @property
    def events(self):
        if self._events is None:
            event_api_types = [
                'CmdDraw',
                'CmdDrawIndexed',
                'CmdDrawIndirect',
                'CmdDrawIndexedIndirect',
                'CmdDrawIndirectCountAMD',
                'CmdDrawIndexedIndirectCountAMD',
                'CmdDispatch',
                'CmdDispatchIndirect',
                'CmdCopyBuffer',
                'CmdCopyImage',
                'CmdBlitImage',
                'CmdCopyBufferToImage',
                'CmdCopyImageToBuffer',
                'CmdUpdateBuffer',
                'CmdFillBuffer',
                'CmdClearColorImage',
                'CmdClearDepthStencilImage',
                'CmdClearAttachments',
                'CmdResolveImage',
                'CmdWaitEvents',
                'CmdPipelineBarrier',
                'CmdResetQueryPool',
                'CmdCopyQueryPoolResults',
                'RenderPassColorClear',
                'RenderPassDepthStencilClear',
                'RenderPassResolve',
                'InternalUnknown',
                'CmdDrawIndirectCountKHR',
                'CmdDrawIndexedIndirectCountKHR',
            ]
            events = self._inner.events().to_dataframe()
            events['api_type'] = pd.Categorical.from_codes(events.api_type, categories=event_api_types)
            events['label'] = pd.Series(self._inner.get_label_strings(events.label)).str.decode('utf-8')
            self._events = events
        return self._events.copy(deep=False)

    @property
    def command_buffers(self):
        if self._command_buffers is None:
            streams = [
                'Gfx',
                'Ace A',
                'Ace B',
                'Ace C',
                'Ace D',
            ]
            cbs=self._inner.command_buffers().to_dataframe()
            self._command_buffers = cbs
        return self._command_buffers.copy(deep=False)

    @property
    def barriers(self):
        if self._barriers is None:
            self._barriers = self._inner.barriers().to_dataframe()
        return self._barriers.copy(deep=False)

    @property
    def labels(self):
        if self._labels is None:
            self._labels = self._inner.labels().to_dataframe()
        return self._labels.copy(deep=False)

    @property
    def layout_transitions(self):
        if self._layout_transitions is None:
            self._layout_transitions = self._inner.layout_transitions().to_dataframe()
        return self._layout_transitions.copy(deep=False)

    @property
    def pipeline_binds(self):
        if self._pipeline_binds is None:
            bind_points = [
                'GFX',
                'COMPUTE',
            ]
            binds=self._inner.pipeline_binds().to_dataframe()
            binds['bind_point'] = pd.Categorical.from_codes(binds.bind_point, categories=bind_points)
            self._pipeline_binds = binds
        return self._pipeline_binds.copy(deep=False)

    @property
    def sub_events(self):
        if self._sub_events is None:
            self._sub_events = self._inner.sub_events().to_dataframe()
        return self._sub_events