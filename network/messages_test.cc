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

#include <gtest/gtest.h>
#include <limits>
#include <iostream>
#include "messages.h"

namespace
{

TEST(MessagesTest, WriteAndReadUint32)
{
    Network::Buffer buf;
    uint32_t        write_value = 123456703;
    Network::WriteUint32ToBuffer(write_value, buf);
    size_t offset = 0;
    auto   read_value = Network::ReadUint32FromBuffer(buf, offset);
    ASSERT_TRUE(read_value.ok());
    ASSERT_EQ(write_value, *read_value);

    buf.clear();
    write_value = 0;
    Network::WriteUint32ToBuffer(write_value, buf);
    offset = 0;
    read_value = Network::ReadUint32FromBuffer(buf, offset);
    ASSERT_TRUE(read_value.ok());
    ASSERT_EQ(write_value, *read_value);

    buf.clear();
    write_value = std::numeric_limits<uint32_t>::max();
    Network::WriteUint32ToBuffer(write_value, buf);
    offset = 0;
    read_value = Network::ReadUint32FromBuffer(buf, offset);
    ASSERT_TRUE(read_value.ok());
    ASSERT_EQ(write_value, *read_value);
}

TEST(MessagesTest, WriteAndReadString)
{
    Network::Buffer buf;
    std::string     write_str = "Hello Dive!";
    Network::WriteStringToBuffer(write_str, buf);
    size_t offset = 0;
    auto   read_str = Network::ReadStringFromBuffer(buf, offset);
    ASSERT_TRUE(read_str.ok());
    ASSERT_EQ(write_str, *read_str);

    buf.clear();
    write_str = "";
    Network::WriteStringToBuffer(write_str, buf);
    offset = 0;
    read_str = Network::ReadStringFromBuffer(buf, offset);
    ASSERT_TRUE(read_str.ok());
    ASSERT_EQ(write_str, *read_str);
}

TEST(MessagesTest, HandShakeMessage)
{
    Network::HandShakeRequest request;
    request.SetMajorVersion(345612);
    request.SetMinorVersion(567348);
    Network::Buffer buf;
    auto            status = request.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.GetMessageType(), Network::MessageType::HANDSHAKE_REQUEST);

    Network::HandShakeResponse response;
    status = response.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(response.GetMessageType(), Network::MessageType::HANDSHAKE_RESPONSE);

    ASSERT_EQ(request.GetMajorVersion(), response.GetMajorVersion());
    ASSERT_EQ(request.GetMinorVersion(), response.GetMinorVersion());
}

TEST(MessagesTest, PingPongMessage)
{
    Network::PingMessage ping;
    Network::Buffer      buf;
    auto                 status = ping.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(ping.GetMessageType(), Network::MessageType::PING_MESSAGE);

    Network::PongMessage pong;
    status = pong.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(pong.GetMessageType(), Network::MessageType::PONG_MESSAGE);
}

TEST(MessagesTest, Pm4CaptureMessage)
{
    Network::Pm4CaptureRequest request;
    Network::Buffer            buf;
    auto                       status = request.Serialize(buf);
    ASSERT_TRUE(status.ok());
    status = request.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.GetMessageType(), Network::MessageType::PM4_CAPTURE_REQUEST);

    Network::Pm4CaptureResponse res_serialize;
    res_serialize.SetString("/sdcard/captures/dive_capture_0001.rd");
    buf.clear();
    status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), Network::MessageType::PM4_CAPTURE_RESPONSE);
    Network::Pm4CaptureResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), Network::MessageType::PM4_CAPTURE_RESPONSE);
    ASSERT_EQ(res_serialize.GetString(), res_deserialize.GetString());
}

TEST(MessagesTest, DownloadFileMessage)
{
    Network::DownloadFileRequest req_serialize;
    req_serialize.SetString("/sdcard/captures/dive_capture_0456.rd");
    Network::Buffer buf;
    auto            status = req_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_serialize.GetMessageType(), Network::MessageType::DOWNLOAD_FILE_REQUEST);
    Network::DownloadFileRequest req_deserialize;
    status = req_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_deserialize.GetMessageType(), Network::MessageType::DOWNLOAD_FILE_REQUEST);
    ASSERT_EQ(req_serialize.GetString(), req_deserialize.GetString());

    Network::DownloadFileResponse res_serialize;
    res_serialize.SetFound(false);
    res_serialize.SetErrorReason("File not found!");
    res_serialize.SetFilePath("/sdcard/captures/other_capture_0456.rd");
    res_serialize.SetFileSizeStr("234512340012367880000000000000");
    buf.clear();
    status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), Network::MessageType::DOWNLOAD_FILE_RESPONSE);
    Network::DownloadFileResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), Network::MessageType::DOWNLOAD_FILE_RESPONSE);
    ASSERT_EQ(res_serialize.GetFound(), res_deserialize.GetFound());
    ASSERT_EQ(res_serialize.GetErrorReason(), res_deserialize.GetErrorReason());
    ASSERT_EQ(res_serialize.GetFilePath(), res_deserialize.GetFilePath());
    ASSERT_EQ(res_serialize.GetFileSizeStr(), res_deserialize.GetFileSizeStr());
}

TEST(MessagesTest, FileSizeMessage)
{
    Network::FileSizeRequest req_serialize;
    req_serialize.SetString("/sdcard/captures/dive_capture_0222.rd");
    Network::Buffer buf;
    auto            status = req_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_serialize.GetMessageType(), Network::MessageType::FILE_SIZE_REQUEST);
    Network::FileSizeRequest req_deserialize;
    status = req_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_deserialize.GetMessageType(), Network::MessageType::FILE_SIZE_REQUEST);
    ASSERT_EQ(req_serialize.GetString(), req_deserialize.GetString());

    Network::FileSizeResponse res_serialize;
    res_serialize.SetFound(false);
    res_serialize.SetErrorReason("File not found!");
    res_serialize.SetFileSizeStr("256000000000000");
    buf.clear();
    status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), Network::MessageType::FILE_SIZE_RESPONSE);
    Network::FileSizeResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), Network::MessageType::FILE_SIZE_RESPONSE);
    ASSERT_EQ(res_serialize.GetFound(), res_deserialize.GetFound());
    ASSERT_EQ(res_serialize.GetErrorReason(), res_deserialize.GetErrorReason());
    ASSERT_EQ(res_serialize.GetFileSizeStr(), res_deserialize.GetFileSizeStr());
}

}  // namespace