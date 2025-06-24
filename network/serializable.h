/*
Copyright 2025 Google Inc.

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

#include <cstdint>
#include <vector>

#include "absl/status/statusor.h"

namespace Network
{

enum class MessageType : uint32_t;
using Buffer = std::vector<uint8_t>;

class ISerializable
{
public:
    virtual ~ISerializable() = default;

    // Returns the specific type identifier for this message.
    virtual MessageType GetMessageType() const = 0;

    // Serializes the object's payload into the destination buffer.
    // Returns absl::OkStatus() on success, or an error status on failure.
    virtual absl::Status Serialize(Buffer& dest) const = 0;

    // Deserializes the object's state from the source buffer.
    // Returns absl::OkStatus() on success, or an error status on failure.
    virtual absl::Status Deserialize(const Buffer& src) = 0;
};

}  // namespace Network