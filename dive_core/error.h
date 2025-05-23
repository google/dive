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

#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>

namespace Dive
{

//--------------------------------------------------------------------------------------------------
// ErrorCode differentiates different kinds of errors
enum class ErrorCode
{
    Ok,                   // No error
    DataChunkReadFailed,  // Attempt to read a data chunk from a trace file failed
    SqttCorrupt,          // SQTT data cannot be decoded into a valid SQTT message
    SqttUnexpectedMsg,    // SQTT message does not make sense, in the context of the previous SQTT
                          // messages
    SqttMismatch,         // SQTT data from one SE doesn't match with SQTT data from another SE
    SqttBadTimestamp,     // SQTT timestamp/time_reset message does not make sense
    RgpMarkerCorrupt,     // RGP marker data cannot be decoded into a valid marker
    RgpMarkerUnexpected,  // RGP marker does not make sense, in the context of previous RGP markers
    RgpMarkerMismatch,    // RGP marker from one source (SQTT shader engine, or PM4) does not match
                          // with RGP markers from another source
    Count,                // The number of error codes. Count is **not** a valid error code
};

//--------------------------------------------------------------------------------------------------
// EmptyErrorPayload is the default type of error payload, containing no data
struct EmptyErrorPayload
{};

//--------------------------------------------------------------------------------------------------
// SqttErrorPayload is the error payload type for SQTT errors
struct SqttErrorPayload
{
    // The shader engine whose SQTT data triggered the error
    uint32_t m_shader_engine = UINT32_MAX;

    // The index of the message that triggered the error
    uint32_t m_msg_index = UINT32_MAX;

    SqttErrorPayload(uint32_t shader_engine, uint32_t msg_index) :
        m_shader_engine(shader_engine),
        m_msg_index(msg_index)
    {
    }
};

//--------------------------------------------------------------------------------------------------
// RgpMarkerErrorPayload is the error payload type for RGP Marker errors
struct RgpMarkerErrorPayload
{
    // The shader engine whose SQTT data triggered the error, or UINT32_MAX for an error from the
    // PM4 data
    uint32_t m_shader_engine = UINT32_MAX;

    // The index of the SQTT User Event message containing the marker data that triggered this
    // error, or UINT32_MAX for an error from the PM4 data
    uint32_t m_msg_index = UINT32_MAX;

    // The PM4 address containing the marker data that triggered this error, or UINT64_MAX  for an
    // error from the SQTT data
    uint64_t m_pm4_addr = UINT64_MAX;

    RgpMarkerErrorPayload(uint32_t shader_engine, uint32_t msg_index, uint64_t pm4_addr) :
        m_shader_engine(shader_engine),
        m_msg_index(msg_index),
        m_pm4_addr(pm4_addr)
    {
    }
};

//--------------------------------------------------------------------------------------------------
// ErrorPayloadT is a template that associates ErrorCode values with payload types.
// This template definition associates every error code with EmptyErrorPayload by default, and the
// subsequent template specializations override the payload type for different error codes.
template<ErrorCode> struct ErrorPayloadT
{
    using PayloadT = EmptyErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::SqttCorrupt>
{
    using PayloadT = SqttErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::SqttUnexpectedMsg>
{
    using PayloadT = SqttErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::SqttMismatch>
{
    using PayloadT = SqttErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::SqttBadTimestamp>
{
    using PayloadT = SqttErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::RgpMarkerCorrupt>
{
    using PayloadT = RgpMarkerErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::RgpMarkerUnexpected>
{
    using PayloadT = RgpMarkerErrorPayload;
};
template<> struct ErrorPayloadT<ErrorCode::RgpMarkerMismatch>
{
    using PayloadT = RgpMarkerErrorPayload;
};

//--------------------------------------------------------------------------------------------------
// ErrorInfo contains the data for an Error. This base class excludes the payload. This class should
// only ever be used as a base class of an ErrorInfoT that holds the correct payload for the error
// code.
struct ErrorInfo
{
    ErrorCode   m_code;
    std::string m_desc;

protected:
    inline ErrorInfo(ErrorCode code) :
        m_code(code)
    {
    }
};

//--------------------------------------------------------------------------------------------------
// ErrorInfoT contains the data for an Error, including the payload.
template<ErrorCode C> struct ErrorInfoT : public ErrorInfo
{
    typename ErrorPayloadT<C>::PayloadT m_payload;
    inline ErrorInfoT(const typename ErrorPayloadT<C>::PayloadT &payload) :
        ErrorInfo(C),
        m_payload(payload)
    {
    }
};

//--------------------------------------------------------------------------------------------------
// Error represents a possible error. An Error value can be in one of two states:
//  - Ok/no error. Error code=ErrorCode::Ok. An Ok error value coerces to `false`, allowing, e.g.
//       if(err) return err;
//  - An actual error, including the following data:
//    - A human readable description
//    - An error code
//    - Payload data (type depends on error code)
class Error
{
public:
    // Builder allows << operators to be used to construct an Error value
    // Typical usage looks something like: `MyError() << "Helpful description of my error";`
    // Where `MyError()` is a function that sets up the error code and payload for some type of
    // error, and the description gives more specific information.
    class Builder
    {
    public:
        operator Error();
        template<typename T> Builder &operator<<(const T &val)
        {
            m_buf << val;
            return *this;
        }
        Builder &operator<<(std::ostream &(*manip)(std::ostream &))
        {
            m_buf << manip;
            return *this;
        }

    protected:
        friend class Error;
        Builder() = default;
        template<ErrorCode C> void Init(const typename ErrorPayloadT<C>::PayloadT &payload)
        {
            m_info = std::make_shared<ErrorInfoT<C>>(payload);
        }
        std::shared_ptr<ErrorInfo> m_info;
        std::ostringstream         m_buf;
    };
    friend class Builder;

    // MakeBuilder creates a Builder and initializes the error code and payload
    template<ErrorCode C> static Builder New(const typename ErrorPayloadT<C>::PayloadT &payload)
    {
        Builder b;
        b.Init<C>(payload);
        return b;
    }

    template<ErrorCode C> static Builder New()
    {
        typename ErrorPayloadT<C>::PayloadT payload;
        return New<C>(payload);
    }

    // The default constructor returns Ok/non-error
    Error() = default;

    // The `Ok` function helps make it clear that code is returning a non-error, e.g.
    inline static Error Ok() { return Error(); }

    // Error values coerce to bool
    // - Ok/non-errors coerce to false
    // - Actual errors coerce to true
    // This allows code like `if (err) return err;`
    operator bool() const { return Code() != ErrorCode::Ok; }

    // Code returns the ErrorCode of this error, or ErrorCode::Ok for non-errors
    ErrorCode Code() const;

    // Description returns the description of this error, or "" for non-errors
    const char *Description() const;

    // Payload returns the error-code specific payload
    template<ErrorCode C> const typename ErrorPayloadT<C>::PayloadT &Payload() const;

protected:
    std::shared_ptr<ErrorInfo> m_info;
};

template<ErrorCode C> const typename ErrorPayloadT<C>::PayloadT &Error::Payload() const
{
    if (Code() != C)
        abort();
    return static_cast<const ErrorInfoT<C> *>(m_info.get())->m_data;
}
template<> const EmptyErrorPayload &Error::Payload<ErrorCode::Ok>() const;
}  // namespace Dive