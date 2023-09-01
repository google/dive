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

#pragma once

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace Dive
{

//--------------------------------------------------------------------------------------------------
// numpy_type<T>::type is the basic type (int, uint32_t, float, etc) for T, which is safe to use in
// a Python numpy array. E.g.
//   - `numpy_type<uint32_t>::type` is `uint32_t`
//   - `numpy_type<EventmarkerId>::type` is `uint32_t` (see template specilaziation below)
template<typename T, typename = void> struct numpy_type
{
    using type = T;
};

//--------------------------------------------------------------------------------------------------
// This specialization of numpy_type<T> applies to the `*Id` types (e.g. `EventMarkerId`).
template<typename T> struct numpy_type<T, std::void_t<typename T::basic_type>>
{
    using type = typename T::basic_type;
};

//--------------------------------------------------------------------------------------------------
// IdAccessor wraps a member function expecting an ID argument in a form useable for a Python
// binding. E.g.
// - Given: `SqttStreamId SqttEventsInfo::GetStream(Id id) const`
// - Then: `f = IdAccessor(&SqttEventsInfo::GetStream)` is a function with signature
//         `SqttStreamId f(const SqttEventsInfo& self, uint32_t id)`
template<typename C, typename T, typename ID> constexpr auto IdAccessor(T (C::*f)(ID) const)
{
    return [f](const C &self, typename ID::basic_type id) {
        if (!self.IsValidId(ID(id)))
            throw std::out_of_range("id out of range");
        return (T)(self.*f)(ID(id));
    };
}

//--------------------------------------------------------------------------------------------------
// ArrayAccessor wraps a member function returning a pointer into a function returning a numpy array
// that is useable for a Python binding.
//
// Note: ArrayAccessor assumes that the class containing the member function also contains a member
// `size` that returns the number of elements in the array.
//
// E.g.
// - Given: `const CommandBufferMarkerId* EventMarkerInfo::CommandBufferPtr() const`
// - Then: `f = ArrayAccessor(&EventMarkerInfo::CommandBufferPtr)` is a function with signature
//         `const uint32_t* f(const EventMarkerInfo& self)`
template<typename C, typename T> constexpr auto ArrayAccessor(const T *(C::*f)() const)
{
    return [f](const C &self) {
        return py::array_t<typename numpy_type<T>::type>(self.size(),
                                                         (const typename numpy_type<T>::type
                                                          *)(self.*f)());
    };
}

//--------------------------------------------------------------------------------------------------
// GenArrayAccessor wraps a member function expecting an ID argument into a function returning a
// numpy array, where the element `i` of the numpy array is constructed by calling the member
// function with id `i`.
//
// Notes:
//   - Assumes the class containng the member function also has a `size` member that returns the
//   number of array elements
//   - Assumes the valid IDs are 0 <= i < size()
//   - Works for `ID` types that are basic integers (e.g. `uint64_t`) or `*Id` wrapper types (e.g.
//   `EventMarkerId`)
//
// E.g.
//   - Given: `bool SqttEventsInfo::IsHardwareContextStalled(Id id) const`
//   - Then: `f = GenArrayAccessor(&SqttEventsInfo::IsHardwareContextStalled)` has signature
//           `const bool* f(const SqttEventsInfo&)`
template<typename C, typename T, typename ID> constexpr auto GenArrayAccessor(T (C::*f)(ID) const)
{
    return [f](const C &self) {
        py::array_t<typename numpy_type<T>::type> result(self.size());
        auto                                      r = result.template mutable_unchecked<1>();
        for (typename numpy_type<ID>::type i = 0; i < self.size(); ++i)
            r(i) = (typename numpy_type<T>::type)((self.*f)(ID(i)));
        return result;
    };
}

//--------------------------------------------------------------------------------------------------
// Array(size, ptr) constructs a numpy array with `size` elements, beginning at `ptr`.
// Note: the data is copied into python-managed memory.
template<typename I, typename T>
py::array_t<typename numpy_type<T>::type> Array(I size, const T *ptr)
{
    return py::array_t<typename numpy_type<T>::type>(size,
                                                     (const typename numpy_type<T>::type *)ptr);
}

//--------------------------------------------------------------------------------------------------
// Array(vec) constructs a numpy array containing the elements in the vector `vec`.
// Note: the data is copied into python-managed memory.
template<typename I, typename T>
py::array_t<typename numpy_type<T>::type> Array(const std::vector<T> &v)
{
    return py::array_t<typename numpy_type<T>::type>(v.size(),
                                                     (
                                                     const typename numpy_type<T>::type *)v.data());
}

//--------------------------------------------------------------------------------------------------
// GenArray creates a new numpy array, given a number of elements, and a function `gen` such that
// `gen(i)` is the `i`th element of the array.
template<typename T, typename Gen, typename I>
py::array_t<typename numpy_type<T>::type> GenArray(I size, Gen gen)
{
    py::array_t<typename numpy_type<T>::type> result(size);
    auto                                      r = result.template mutable_unchecked<1>();
    for (I i = 0; i < size; ++i)
        r(i) = (typename numpy_type<T>::type)(gen(i));
    return result;
}

//--------------------------------------------------------------------------------------------------
// GenArray(obj, size, gen) constructs a new numpy array with `size` elements, where element `i` is
// constructed by calling the member function `gen` on `obj`.
template<typename T, typename U, typename C, typename I>
py::array_t<T> GenArray(const C &h, I size, U (C::*gen)(I) const)
{
    py::array_t<T> result(size);
    auto           r = result.template mutable_unchecked<1>();
    for (uint64_t i = 0; i < size; ++i)
        r(i) = (T)((h.*gen)(i));
    return result;
}

//--------------------------------------------------------------------------------------------------
// GenArray(obj, gen) constructs a new numpy array with `obj.size()` elements, where element `i` is
// constructed by calling the member function `gen` on `obj`.
template<typename T, typename U, typename C, typename I>
py::array_t<T> GenArray(const C &h, U (C::*gen)(I) const)
{
    return GenArray<T>(h, (I)h.size(), gen);
}

//--------------------------------------------------------------------------------------------------
// GenStrArray(sz, gen) constructs a numpy array of `sz` strings, where element `i` is constructed
// by calling `gen(i)`, which is assumed to return a `const char *`.
template<typename G> py::array GenStrArray(size_t sz, G gen)
{
    size_t max_width = 0;
    for (size_t i = 0; i < sz; ++i)
        max_width = std::max(strlen(gen(i)), max_width);
    max_width += 1;
    py::dtype             dtype("|S" + std::to_string(max_width));
    std::array<size_t, 1> shape{ sz };
    std::array<size_t, 1> stride{ max_width };
    py::array             result(dtype, shape, stride);

    char *data = reinterpret_cast<char *>(result.request().ptr);
    memset(data, 0, sz * max_width);
    for (size_t i = 0; i < sz; ++i)
        strcpy(&data[i * max_width], gen(i));
    return result;
}

}  // namespace Dive