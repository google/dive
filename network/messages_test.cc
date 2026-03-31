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

#include "messages.h"

#include <gtest/gtest.h>

#include <iostream>
#include <limits>

#include "absl/status/status_matchers.h"
#include "drawcall_filter_config.h"

namespace
{

using ::absl_testing::IsOkAndHolds;

TEST(MessagesTest, WriteAndReadBool)
{
    Network::Buffer buf;
    bool write_value = true;
    Network::WriteBoolToBuffer(write_value, buf);
    size_t offset = 0;
    ASSERT_THAT(Network::ReadBoolFromBuffer(buf, offset), IsOkAndHolds(write_value));

    buf.clear();
    write_value = false;
    Network::WriteBoolToBuffer(write_value, buf);
    offset = 0;
    ASSERT_THAT(Network::ReadBoolFromBuffer(buf, offset), IsOkAndHolds(write_value));
}

TEST(MessagesTest, WriteAndReadUint32)
{
    Network::Buffer buf;
    uint32_t write_value = 123456703;
    Network::WriteUint32ToBuffer(write_value, buf);
    size_t offset = 0;
    ASSERT_THAT(Network::ReadUint32FromBuffer(buf, offset), IsOkAndHolds(write_value));

    buf.clear();
    write_value = 0;
    Network::WriteUint32ToBuffer(write_value, buf);
    offset = 0;
    auto read_value = Network::ReadUint32FromBuffer(buf, offset);
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

TEST(MessagesTest, WriteAndReadUint64)
{
    Network::Buffer buf;
    uint64_t write_value = 123456703;
    Network::WriteUint64ToBuffer(write_value, buf);
    size_t offset = 0;
    auto read_value = Network::ReadUint64FromBuffer(buf, offset);
    ASSERT_TRUE(read_value.ok());
    ASSERT_EQ(write_value, *read_value);

    buf.clear();
    write_value = 0;
    Network::WriteUint64ToBuffer(write_value, buf);
    offset = 0;
    read_value = Network::ReadUint64FromBuffer(buf, offset);
    ASSERT_TRUE(read_value.ok());
    ASSERT_EQ(write_value, *read_value);

    buf.clear();
    write_value = std::numeric_limits<uint64_t>::max();
    Network::WriteUint64ToBuffer(write_value, buf);
    offset = 0;
    read_value = Network::ReadUint64FromBuffer(buf, offset);
    ASSERT_TRUE(read_value.ok());
    ASSERT_EQ(write_value, *read_value);
}

TEST(MessagesTest, WriteAndReadString)
{
    Network::Buffer buf;
    std::string write_str = "Hello Dive!";
    Network::WriteStringToBuffer(write_str, buf);
    size_t offset = 0;
    auto read_str = Network::ReadStringFromBuffer(buf, offset);
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
    Network::HandshakeRequest request;
    request.SetMajorVersion(345612);
    request.SetMinorVersion(567348);
    Network::Buffer buf;
    auto status = request.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.GetMessageType(), Network::MessageType::HANDSHAKE_REQUEST);

    Network::HandshakeResponse response;
    status = response.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(response.GetMessageType(), Network::MessageType::HANDSHAKE_RESPONSE);

    ASSERT_EQ(request.GetMajorVersion(), response.GetMajorVersion());
    ASSERT_EQ(request.GetMinorVersion(), response.GetMinorVersion());
}

TEST(MessagesTest, PingPongMessage)
{
    Network::PingMessage ping;
    Network::Buffer buf;
    auto status = ping.Serialize(buf);
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
    Network::Buffer buf;
    auto status = request.Serialize(buf);
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
    auto status = req_serialize.Serialize(buf);
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
    res_serialize.SetFileSize(std::numeric_limits<uint64_t>::max());
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
    ASSERT_EQ(res_serialize.GetFileSize(), res_deserialize.GetFileSize());
}

TEST(MessagesTest, FileSizeMessage)
{
    Network::FileSizeRequest req_serialize;
    req_serialize.SetString("/sdcard/captures/dive_capture_0222.rd");
    Network::Buffer buf;
    auto status = req_serialize.Serialize(buf);
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
    res_serialize.SetFileSize(std::numeric_limits<uint64_t>::max());
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
    ASSERT_EQ(res_serialize.GetFileSize(), res_deserialize.GetFileSize());
}

TEST(MessagesTest, RemoveFileRequest)
{
    Network::RemoveFileRequest req_serialize;
    req_serialize.SetString("/sdcard/captures/to_be_removed.rd");
    Network::Buffer buf;
    auto status = req_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_serialize.GetMessageType(), Network::MessageType::REMOVE_FILE_REQUEST);
    Network::RemoveFileRequest req_deserialize;
    status = req_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_deserialize.GetMessageType(), Network::MessageType::REMOVE_FILE_REQUEST);
    ASSERT_EQ(req_serialize.GetString(), req_deserialize.GetString());
}

TEST(MessageTest, RemoveFileResponseSuccess)
{
    Network::RemoveFileResponse res_serialize;
    res_serialize.SetSuccess(true);
    res_serialize.SetErrorReason("");
    Network::Buffer buf;
    auto status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), Network::MessageType::REMOVE_FILE_RESPONSE);
    Network::RemoveFileResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_deserialize.GetMessageType(), Network::MessageType::REMOVE_FILE_RESPONSE);
    ASSERT_EQ(res_serialize.GetSuccess(), res_deserialize.GetSuccess());
    ASSERT_EQ(res_serialize.GetErrorReason(), res_deserialize.GetErrorReason());
}

TEST(MessagesTest, RemoveFileResponseFailure)
{
    Network::RemoveFileResponse res_serialize_fail;
    res_serialize_fail.SetSuccess(false);
    res_serialize_fail.SetErrorReason("File not found!");
    Network::Buffer buf;
    auto status = res_serialize_fail.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize_fail.GetMessageType(), Network::MessageType::REMOVE_FILE_RESPONSE);
    Network::RemoveFileResponse res_deserialize_fail;
    status = res_deserialize_fail.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize_fail.GetSuccess(), res_deserialize_fail.GetSuccess());
    ASSERT_EQ(res_serialize_fail.GetErrorReason(), res_deserialize_fail.GetErrorReason());
}

TEST(MessagesTest, DrawcallFilterConfigRequest)
{
    Network::DrawcallFilterConfigRequest req_serialize;
    req_serialize.SetTargetRenderPassName("ShadowMapPass");
    req_serialize.SetVertexCount(100);
    req_serialize.SetIndexCount(200);
    req_serialize.SetInstanceCount(5);
    req_serialize.SetMaxDrawcalls(1000);
    req_serialize.SetFilterByVertexCount(true);
    req_serialize.SetFilterByIndexCount(false);
    req_serialize.SetFilterByInstanceCount(true);
    req_serialize.SetEnableDrawcallLimit(true);
    req_serialize.SetFilterByAlphaBlended(true);
    req_serialize.SetFilterByRenderPass(true);

    Network::Buffer buf;
    absl::Status status = req_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_serialize.GetMessageType(), Network::MessageType::DRAWCALL_FILTER_CONFIG_REQUEST);

    Network::DrawcallFilterConfigRequest req_deserialize;
    status = req_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req_deserialize.GetMessageType(),
              Network::MessageType::DRAWCALL_FILTER_CONFIG_REQUEST);
    ASSERT_EQ(req_serialize.GetTargetRenderPassName(), req_deserialize.GetTargetRenderPassName());
    ASSERT_EQ(req_serialize.GetVertexCount(), req_deserialize.GetVertexCount());
    ASSERT_EQ(req_serialize.GetIndexCount(), req_deserialize.GetIndexCount());
    ASSERT_EQ(req_serialize.GetInstanceCount(), req_deserialize.GetInstanceCount());
    ASSERT_EQ(req_serialize.GetMaxDrawcalls(), req_deserialize.GetMaxDrawcalls());
    ASSERT_EQ(req_serialize.GetFilterByVertexCount(), req_deserialize.GetFilterByVertexCount());
    ASSERT_EQ(req_serialize.GetFilterByIndexCount(), req_deserialize.GetFilterByIndexCount());
    ASSERT_EQ(req_serialize.GetFilterByInstanceCount(), req_deserialize.GetFilterByInstanceCount());
    ASSERT_EQ(req_serialize.GetEnableDrawcallLimit(), req_deserialize.GetEnableDrawcallLimit());
    ASSERT_EQ(req_serialize.GetFilterByAlphaBlended(), req_deserialize.GetFilterByAlphaBlended());
    ASSERT_EQ(req_serialize.GetFilterByRenderPass(), req_deserialize.GetFilterByRenderPass());
}

TEST(MessagesTest, DrawcallFilterConfigResponse)
{
    Network::DrawcallFilterConfigResponse res;
    Network::Buffer buf;
    absl::Status status = res.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res.GetMessageType(), Network::MessageType::DRAWCALL_FILTER_CONFIG_RESPONSE);

    status = res.Deserialize(buf);
    ASSERT_TRUE(status.ok());
}

TEST(MessagesTest, LivePSOsRequest)
{
    Network::LivePSOsRequest req;
    Network::Buffer buf;
    absl::Status status = req.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req.GetMessageType(), Network::MessageType::LIVE_PSOS_REQUEST);

    status = req.Deserialize(buf);
    ASSERT_TRUE(status.ok());
}

TEST(MessagesTest, LivePSOsResponse)
{
    Network::LivePSOsResponse res_serialize;
    std::vector<Network::PSOInfo> mock_psos;
    mock_psos.push_back({"PSO_alpha_blend_enabled", 123456789012345ULL, true});
    mock_psos.push_back({"MyCustomEngineOpaqueMaterial", 987654321098765ULL, false});
    res_serialize.SetPSOs(mock_psos);

    Network::Buffer buf;
    absl::Status status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), Network::MessageType::LIVE_PSOS_RESPONSE);

    Network::LivePSOsResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());

    const auto& deserialized_psos = res_deserialize.GetPSOs();
    ASSERT_EQ(deserialized_psos.size(), 2);

    ASSERT_EQ(deserialized_psos[0].name, "PSO_alpha_blend_enabled");
    ASSERT_EQ(deserialized_psos[0].pipeline_handle, 123456789012345ULL);
    ASSERT_EQ(deserialized_psos[0].has_alpha_blend, true);

    ASSERT_EQ(deserialized_psos[1].name, "MyCustomEngineOpaqueMaterial");
    ASSERT_EQ(deserialized_psos[1].pipeline_handle, 987654321098765ULL);
    ASSERT_EQ(deserialized_psos[1].has_alpha_blend, false);
}

TEST(MessagesTest, LiveRenderPassesRequest)
{
    Network::LiveRenderPassesRequest req;
    Network::Buffer buf;
    absl::Status status = req.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(req.GetMessageType(), Network::MessageType::LIVE_RENDER_PASSES_REQUEST);

    status = req.Deserialize(buf);
    ASSERT_TRUE(status.ok());
}

TEST(MessagesTest, LiveRenderPassesResponse)
{
    Network::LiveRenderPassesResponse res_serialize;
    std::vector<Network::RenderPassInfo> mock_rps;
    mock_rps.push_back({"ShadowPass", 0x12345678});
    mock_rps.push_back({"MainForwardPass", 0x87654321});
    res_serialize.SetRenderPasses(mock_rps);

    Network::Buffer buf;
    absl::Status status = res_serialize.Serialize(buf);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(res_serialize.GetMessageType(), Network::MessageType::LIVE_RENDER_PASSES_RESPONSE);

    Network::LiveRenderPassesResponse res_deserialize;
    status = res_deserialize.Deserialize(buf);
    ASSERT_TRUE(status.ok());

    const auto& deserialized_rps = res_deserialize.GetRenderPasses();
    ASSERT_EQ(deserialized_rps.size(), 2);

    ASSERT_EQ(deserialized_rps[0].render_pass_handle, 0x12345678);
    ASSERT_EQ(deserialized_rps[0].name, "ShadowPass");

    ASSERT_EQ(deserialized_rps[1].render_pass_handle, 0x87654321);
    ASSERT_EQ(deserialized_rps[1].name, "MainForwardPass");
}

}  // namespace
