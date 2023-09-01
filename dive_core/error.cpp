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

#include "error.h"

namespace Dive
{
// Code returns the ErrorCode of this error, or ErrorCode::Ok for non-errors

ErrorCode Error::Code() const
{
    if (m_info != nullptr)
        return m_info->m_code;
    else
        return ErrorCode::Ok;
}
const char *Error::Description() const
{
    if (m_info != nullptr)
        return m_info->m_desc.c_str();
    else
        return "";
}
template<> const EmptyErrorPayload &Error::Payload<ErrorCode::Ok>() const
{
    static const EmptyErrorPayload kEmptyPayload;
    if (Code() != ErrorCode::Ok)
        abort();
    return kEmptyPayload;
}
Error::Builder::operator Error()
{
    m_info->m_desc = m_buf.str();
    Error err;
    std::swap(err.m_info, m_info);
    return err;
}
}  // namespace Dive