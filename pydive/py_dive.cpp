/*
 Copyright 2020 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "dive_core/capture_data.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/marker_data.h"
#include "py_common.h"

namespace py = pybind11;
using namespace pybind11::literals;
using namespace Dive;
namespace Dive
{
void py_marker_types(py::module &m);
void py_marker_data(py::module &m);
void py_command_hierarchy(py::module &m);
void py_capture_data(py::module &m);
}  // namespace Dive

PYBIND11_MODULE(_dive, m)
{

    m.doc() = R"pbdoc(
        Dive Python bindings
        -----------------------

        .. currentmodule:: _dive

        .. autosummary::
           :toctree: _generate

           CaptureData
    )pbdoc";

    m.attr("kNumSe") = py::int_(kNumSe);
    m.attr("kNumSh") = py::int_(kNumSh);
    m.attr("kNumCusPerSh") = py::int_(kNumCusPerSh);
    m.attr("kNumSimdsPerCu") = py::int_(kNumSimdsPerCu);
    m.attr("kNumWavefrontsPerSimd") = py::int_(kNumWavefrontsPerSimd);
    m.attr("kNumHardwareContext") = py::int_(kNumHardwareContext);

    py_marker_types(m);
    py_marker_data(m);
    py_command_hierarchy(m);
    py_capture_data(m);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}