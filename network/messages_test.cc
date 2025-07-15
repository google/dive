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

#include "absl/status/status_matchers.h"
#include <gtest/gtest.h>
#include <limits>
#include <iostream>
#include "messages.h"

namespace Network
{
namespace
{

using ::absl_testing::IsOkAndHolds;

TEST(MessagesTest, WriteAndReadUint32)
{
    Buffer         buf;
    const uint32_t write_value_1 = 123456703;
    WriteUint32ToBuffer(write_value_1, buf);
    size_t offset = 0;
    auto   read_value_1 = ReadUint32FromBuffer(buf, offset);
    ASSERT_TRUE(read_value_1.ok());
    ASSERT_EQ(write_value_1, *read_value_1);

    buf.clear();
    const uint32_t write_value_2 = 0;
    WriteUint32ToBuffer(write_value_2, buf);
    offset = 0;
    auto read_value_2 = ReadUint32FromBuffer(buf, offset);
    ASSERT_TRUE(read_value_2.ok());
    ASSERT_EQ(write_value_2, *read_value_2);

    buf.clear();
    const uint32_t write_value_3 = std::numeric_limits<uint32_t>::max();
    WriteUint32ToBuffer(write_value_3, buf);
    offset = 0;
    auto read_value_3 = ReadUint32FromBuffer(buf, offset);
    ASSERT_TRUE(read_value_3.ok());
    ASSERT_EQ(write_value_3, *read_value_3);
}

TEST(MessagesTest, WriteAndReadString)
{
    Buffer            buf;
    const std::string write_str_1 = "Hello Dive!";
    WriteStringToBuffer(write_str_1, buf);
    size_t offset = 0;
    auto   read_str_1 = ReadStringFromBuffer(buf, offset);
    ASSERT_TRUE(read_str_1.ok());
    ASSERT_EQ(write_str_1, *read_str_1);

    buf.clear();
    const std::string write_str_2 = "";
    WriteStringToBuffer(write_str_2, buf);
    offset = 0;
    auto read_str_2 = ReadStringFromBuffer(buf, offset);
    ASSERT_TRUE(read_str_2.ok());
    ASSERT_EQ(write_str_2, *read_str_2);
}

TEST(MessagesTest, SerializeAndDeserializeHandShakeMessage)
{
    HandshakeRequest request;
    request.SetMajorVersion(345612);
    request.SetMinorVersion(567348);
    Buffer buf;
    auto   status = request.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.GetMessageType(), MessageType::HANDSHAKE_REQUEST);

    HandShakeResponse response;
    status = response.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(response.GetMessageType(), MessageType::HANDSHAKE_RESPONSE);

    ASSERT_EQ(request.GetMajorVersion(), response.GetMajorVersion());
    ASSERT_EQ(request.GetMinorVersion(), response.GetMinorVersion());
}

TEST(MessagesTest, SerializeAndDeserializePingPongMessage)
{
    PingMessage ping;
    Buffer      buf;
    auto        status = ping.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(ping.GetMessageType(), MessageType::PING_MESSAGE);

    PongMessage pong;
    status = pong.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(pong.GetMessageType(), MessageType::PONG_MESSAGE);
}

TEST(MessagesTest, SerializeAndDeserializePm4CaptureMessage)
{
    Pm4CaptureRequest request;
    Buffer            buf;
    auto              status = request.Serialize(buf);
    ASSERT_TRUE(status.ok());
    status = request.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.GetMessageType(), MessageType::PM4_CAPTURE_REQUEST);

    Pm4CaptureResponse res_serialize;
    res_serialize.SetString("/sdcard/captures/dive_capture_0001.rd");
    buf.clear();
    status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), MessageType::PM4_CAPTURE_RESPONSE);
    Pm4CaptureResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), MessageType::PM4_CAPTURE_RESPONSE);
    ASSERT_EQ(res_serialize.GetString(), res_deserialize.GetString());
}

TEST(MessagesTest, SerializeAndDeserializeDownloadFileMessage)
{
    DownloadFileRequest req_serialize;
    req_serialize.SetString("/sdcard/captures/dive_capture_0456.rd");
    Buffer buf;
    auto   status = req_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_serialize.GetMessageType(), MessageType::DOWNLOAD_FILE_REQUEST);
    DownloadFileRequest req_deserialize;
    status = req_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_deserialize.GetMessageType(), MessageType::DOWNLOAD_FILE_REQUEST);
    ASSERT_EQ(req_serialize.GetString(), req_deserialize.GetString());

    DownloadFileResponse res_serialize;
    res_serialize.SetFound(false);
    res_serialize.SetErrorReason("File not found!");
    res_serialize.SetFilePath("/sdcard/captures/other_capture_0456.rd");
    res_serialize.SetFileSizeStr("234512340012367880000000000000");
    buf.clear();
    status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), MessageType::DOWNLOAD_FILE_RESPONSE);
    DownloadFileResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), MessageType::DOWNLOAD_FILE_RESPONSE);
    EXPECT_EQ(res_serialize.GetFound(), res_deserialize.GetFound());
    EXPECT_EQ(res_serialize.GetErrorReason(), res_deserialize.GetErrorReason());
    EXPECT_EQ(res_serialize.GetFilePath(), res_deserialize.GetFilePath());
    EXPECT_EQ(res_serialize.GetFileSizeStr(), res_deserialize.GetFileSizeStr());
}

TEST(MessagesTest, SerializeAndDeserializeFileSizeMessage)
{
    FileSizeRequest req_serialize;
    req_serialize.SetString("/sdcard/captures/dive_capture_0222.rd");
    Buffer buf;
    auto   status = req_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_serialize.GetMessageType(), MessageType::FILE_SIZE_REQUEST);
    FileSizeRequest req_deserialize;
    status = req_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_deserialize.GetMessageType(), MessageType::FILE_SIZE_REQUEST);
    ASSERT_EQ(req_serialize.GetString(), req_deserialize.GetString());

    FileSizeResponse res_serialize;
    res_serialize.SetFound(false);
    res_serialize.SetErrorReason("File not found!");
    res_serialize.SetFileSizeStr("256000000000000");
    buf.clear();
    status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), MessageType::FILE_SIZE_RESPONSE);
    FileSizeResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), MessageType::FILE_SIZE_RESPONSE);
    EXPECT_EQ(res_serialize.GetFound(), res_deserialize.GetFound());
    EXPECT_EQ(res_serialize.GetErrorReason(), res_deserialize.GetErrorReason());
    EXPECT_EQ(res_serialize.GetFileSizeStr(), res_deserialize.GetFileSizeStr());
}

}  // namespace
}  // namespace Network