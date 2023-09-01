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
import _dive
from .opcodes import *

from _dive import CommandHierarchy

class CommandHierarchy():
    def __init__(self, cap):
        self._inner = _dive.CommandHierarchy(cap._inner)
        self._nodes = None
        self._engine_hierarchy_topology = None
        self._submit_hierarchy_topology = None
        self._event_hierarchy_topology = None
        self._rgp_hierarchy_topology = None

    @property
    def nodes(self):
        if self._nodes is None:
            df = self._inner.to_dataframe()
            df['desc'] = df.desc.str.decode('utf-8')
            df['type']=pd.Categorical.from_codes(df['type'], categories=[
                "Root",
                "Engine",
                "Submit",
                "Ib",
                "Marker",
                "DrawDispatchDma",
                "Sync",
                "PostambleState",
                "Packet",
                "Reg",
                "Field",
                "Present",
            ])
            df['submit_node_engine_type'] = pd.Categorical.from_codes(df['submit_node_engine_type'], categories=[
                "Universal",
                "Compute",
                "Dma",
                "Timer",
                "Other",
                "NA",
            ])
            df['ib_node_type'] = pd.Categorical.from_codes(df['ib_node_type'], categories=[
                "Normal",
                "Call",
                "Chain",
                "NA",
            ])
            df['marker_node_type'] = pd.Categorical.from_codes(df['marker_node_type'], categories=[
                "BeginEnd",
                "Insert",
                "RgpInternal",
                "DiveMetadata",
                "NA",
            ])
            df['packet_node_opcode'] = pd.Categorical.from_codes(df['packet_node_opcode'], categories=OPCODE_CATEGORIES)
            self._nodes = df
        return self._nodes

    @property
    def engine_hierarchy_topology(self):
        if self._engine_hierarchy_topology is None:
            self._engine_hierarchy_topology = Topology(self._inner.engine_hierarchy_topology())
        return self._engine_hierarchy_topology

    @property
    def submit_hierarchy_topology(self):
        if self._submit_hierarchy_topology is None:
            self._submit_hierarchy_topology = Topology(self._inner.submit_hierarchy_topology())
        return self._submit_hierarchy_topology

    @property
    def event_hierarchy_topology(self):
        if self._event_hierarchy_topology is None:
            self._event_hierarchy_topology = Topology(self._inner.event_hierarchy_topology())
        return self._event_hierarchy_topology

    @property
    def rgp_hierarchy_topology(self):
        if self._rgp_hierarchy_topology is None:
            self._rgp_hierarchy_topology = Topology(self._inner.rgp_hierarchy_topology())
        return self._rgp_hierarchy_topology

class Topology:
    def __init__(self, inner):
        self._inner = inner

    @property
    def parent(self):
        return pd.Series(self._inner.parent())

    @property
    def shared(self):
        return self._inner.shared()