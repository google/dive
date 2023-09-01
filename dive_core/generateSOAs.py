# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict

from jinja2 import Environment, FileSystemLoader
"""
This script generates structure-of-array C++ classes from a template (structure_of_arrays.jinja)
and a json description of the types (marker_types.json).

At the simplest level, the generated classes can be thought of as several arrays, each with the
same number of elements. E.g. `EventMarkerInfo` has arrays `VertexOffsetRegIdx` and `ThreadX`,
among others.

The elements in these arrays are identified by a class-specific identifier type. E.g. the elements
in the arrays in `EventMarkerInfo` are identified by `EventMarkerId`. E.g.

```
EventMarkerInfo &events = marker_data.Events();
uint32_t thread_x = events.ThreadX(EventMarkerId(0));
events.SetThreadY(EventMarkeId(1), 7);
```

# Element References

In addition to the Id-based getters and setters, the generated types also support references. E.g.
the following is equivalent to the code above:

```
EventMarkerInfo &events = marker_data.Events();
uint32_t thread_x = events[EventMarkerId(0)].ThreadX();
events[EventMarkerId(1)].SetThreadY(7);
```

# Iterators

The generated types also support iterator semantics. E.g. the following is equivalent to both of
the above code examples:

```
EventMarkerInfo &events = marker_data.Events();
auto it = events.find(EventMarkerId(0));
uint32_t thread_x = it->ThreadX();
it = events.find(EventMarkerId(1));
it->SetThreadY(7);
```

The iterators also allow looping over the elements, using either explicit iterators or range for
loops. E.g.

```
EventMarkerInfo &events = marker_data.Events();
for(auto it=events.begin(); it != events.end(); ++it) {
    it->SetThreadX(2);
}
for(auto e: events) {
    e.SetThreadX(2);
}
```

# Adding new elements

New elements can be added to all of the arrays in the class using `Add()`, which returns an iterator. E.g.
```
EventmarkerInfo &events = marker_data.Events();
auto it = events.Add();
it->SetThreadY(7);
```
"""


def clang_format(path: str) -> None:
    """
    Run clang-format-7 on C++ source file given by the path
    """
    if 'CLANG_FORMAT' in os.environ:
        clang_format_path = os.environ['CLANG_FORMAT']
    else:
        clang_format_path = shutil.which("clang-format-7.0.1") or \
                            shutil.which("clang-format-7") or \
                            shutil.which("clang-format")
    if not clang_format_path or not os.path.isfile(clang_format_path):
        raise (Exception("Could not find clang-format"))
    res = subprocess.run([clang_format_path, "--version"],
                         capture_output=True,
                         check=True)
    if not res.stdout.decode('utf-8').startswith("clang-format version 7"):
        raise (Exception("Incorrect clang-format version"))
    subprocess.run([clang_format_path, "-i", path], check=True)


def snake_case(value: str) -> str:
    """
    Converts a camel/Pascal case identifier (e.g. "MyIdentifier") into a snake case identifier (e.g. "my_identifier").

    This is used by the template.
    """
    return re.sub('((?<=[a-z0-9])[A-Z]|(?!^)[A-Z](?=[a-z]))', r'_\1',
                  value).lower()


def _is_numeric(ty: str) -> str:
    return ty in ["int32_t", "uint32_t", "int64_t", "uint64_t"]


def generate(spec: Dict, gen_name: str) -> None:
    """
    Generates the header and source files for a loaded spec
    """
    template_dir = os.path.abspath("dive_core")
    loader = FileSystemLoader(template_dir)
    env = Environment(loader=loader,
                      trim_blocks=True,
                      lstrip_blocks=True,
                      keep_trailing_newline=True,
                      autoescape=False)
    env.tests['numeric'] = _is_numeric
    env.filters['snake_case'] = snake_case

    def gen_file(macro, path, **kwargs):
        Path(os.path.dirname(path)).mkdir(parents=True, exist_ok=True)
        with open(path, 'w') as f:
            tmpl = ''.join(
                ["{% import 'struct_of_arrays.jinja' as macros %}", macro])
            env.from_string(tmpl).stream(**kwargs).dump(f)
        if path.endswith(".h") or path.endswith(".cpp"):
            clang_format(path)

    spec_options = []
    if "options" in spec["header"]:
        spec_options=spec["header"]["options"]
    env.globals['options'] = spec_options

    gen_file("{{macros.soa_h(soas, includes, namespace, gen_name)}}",
             spec["header"]["path"],
             soas=spec["soa_types"],
             includes=spec["header"]["includes"],
             namespace=spec["namespace"],
             gen_name=gen_name)

    gen_file(
        "{{macros.soa_cpp(soas, sys_includes, includes, namespace, gen_name)}}",
        spec["src"]["path"],
        soas=spec["soa_types"],
        includes=spec["src"]["includes"],
        sys_includes=spec["src"]["sys_includes"],
        namespace=spec["namespace"],
        gen_name=gen_name)

    gen_file(
        "{{macros.soa_py_cpp(soas, py_bind_func, includes, namespace, gen_name)}}",
        spec["py_wrapper"]["path"],
        soas=spec["soa_types"],
        includes=spec["py_wrapper"]["includes"],
        py_bind_func=spec["py_wrapper"]["func"],
        namespace=spec["namespace"],
        gen_name=gen_name)

    gen_file("{{macros.natvis(soas, gen_name)}}",
             spec["natvis"]["path"],
             soas=spec["soa_types"],
             gen_name=gen_name)


def main():
    if len(sys.argv) != 2:
        print(sys.argv[0] + " <Path to struct-of-arrays json spec file>")
        sys.exit()
    print(sys.argv[1])
    json_path = os.path.abspath(sys.argv[1])

    # Find the root of the Dive source tree relative to this script. This allows the script to
    # output code to the correct location regardless of the working directory where the script is
    # invoked.
    # *NOTE*: This will break if this script is moved to a different location.
    script_path = os.path.abspath(__file__)
    script_dir, script_name = os.path.split(script_path)

    # Attempt to detect if this script has been moved
    assert (os.path.basename(script_dir) == "dive_core")

    dive_src = os.path.dirname(script_dir)
    print(dive_src)
    os.chdir(dive_src)

    with open(json_path) as f:
        spec = json.load(f)
        generate(spec, script_name)


if __name__ == "__main__":
    main()
