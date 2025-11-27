/*
 Copyright 2025 Google LLC

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

#include "dive/utils/version_info.h"

#include "absl/status/status_matchers.h"
#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Dive
{
namespace
{

using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using testing::ElementsAre;
using testing::Pair;

TEST(GetDeviceLibraryVersionMapTest, BasicPass)
{
    std::string csv_content = "sha,0343dcaa2d0335a46dd673d19b56963d9b045879\n"
                              "version,1.1.2\nbuild_type,Debug\nrelease_type,dev\nabi,arm64-v8a";

    absl::StatusOr<std::map<std::string_view, std::string_view>> got = GetDeviceLibraryVersionMap(
    csv_content);

    EXPECT_THAT(got,
                IsOkAndHolds(ElementsAre(Pair(VersionInfoConstants::kNameAbi, "arm64-v8a"),
                                         Pair(VersionInfoConstants::kNameBuildType, "Debug"),
                                         Pair(VersionInfoConstants::kNameReleaseType, "dev"),
                                         Pair(VersionInfoConstants::kNameSha,
                                              "0343dcaa2d0335a46dd673d19b56963d9b045879"),
                                         Pair(VersionInfoConstants::kNameVersion, "1.1.2"))))
    << got.status().message();
}

TEST(GetDeviceLibraryVersionMapTest, RowTooManyFieldsFail)
{
    std::string
    csv_content = "sha,0343dcaa2d0335a46dd673d19b56963d9b045879\n"
                  "version,1.1.2\nbuild_type,Debug,PLACEHOLDER\nrelease_type,dev\nabi,arm64-v8a";

    std::string want_err_msg = "GetDeviceLibraryVersionMap() unexpected number of fields in row: "
                               "build_type,Debug,PLACEHOLDER";

    absl::StatusOr<std::map<std::string_view, std::string_view>> got = GetDeviceLibraryVersionMap(
    csv_content);

    EXPECT_THAT(got, StatusIs(absl::StatusCode::kFailedPrecondition, want_err_msg))
    << got.status().message();
}

TEST(GetDeviceLibraryVersionMapTest, RowTooFewFieldsFail)
{
    std::string csv_content = "sha,0343dcaa2d0335a46dd673d19b56963d9b045879\n"
                              "version,1.1.2\nPLACEHOLDER\nrelease_type,dev\nabi,arm64-v8a";

    std::string want_err_msg = "GetDeviceLibraryVersionMap() unexpected number of fields in row: "
                               "PLACEHOLDER";

    absl::StatusOr<std::map<std::string_view, std::string_view>> got = GetDeviceLibraryVersionMap(
    csv_content);

    EXPECT_THAT(got, StatusIs(absl::StatusCode::kFailedPrecondition, want_err_msg))
    << got.status().message();
}

TEST(GetDeviceLibraryVersionMapTest, ExtraKeyFail)
{
    std::string csv_content = "sha,0343dcaa2d0335a46dd673d19b56963d9b045879\n"
                              "version,1.1.2\nnew_key,PLACEHOLDER\nbuild_type,Debug\nrelease_type,"
                              "dev\nabi,arm64-v8a";

    std::string want_err_msg = absl::
    StrFormat("GetDeviceLibraryVersionMap() invalid device map: want key count %d, got: %d",
              VersionInfoConstants::kKeyCount,
              VersionInfoConstants::kKeyCount + 1);

    absl::StatusOr<std::map<std::string_view, std::string_view>> got = GetDeviceLibraryVersionMap(
    csv_content);

    EXPECT_THAT(got, StatusIs(absl::StatusCode::kFailedPrecondition, want_err_msg))
    << got.status().message();
}

TEST(GetDeviceLibraryVersionMapTest, MissingKeyFail)
{
    std::string csv_content = "sha,0343dcaa2d0335a46dd673d19b56963d9b045879\n"
                              "version,1.1.2\nrelease_type,"
                              "dev\nabi,arm64-v8a";

    std::string want_err_msg = absl::
    StrFormat("GetDeviceLibraryVersionMap() invalid device map: want key count %d, got: %d",
              VersionInfoConstants::kKeyCount,
              VersionInfoConstants::kKeyCount - 1);

    absl::StatusOr<std::map<std::string_view, std::string_view>> got = GetDeviceLibraryVersionMap(
    csv_content);

    EXPECT_THAT(got, StatusIs(absl::StatusCode::kFailedPrecondition, want_err_msg))
    << got.status().message();
}

TEST(GetDeviceLibraryVersionMapTest, WrongKeyFail)
{
    std::string csv_content = "sha,0343dcaa2d0335a46dd673d19b56963d9b045879\n"
                              "version,1.1.2\nPLACEHOLDER_KEY,PLACEHOLDER_VALUE\nrelease_type,"
                              "dev\nabi,arm64-v8a";

    std::string
    want_err_msg = absl::StrFormat("GetDeviceLibraryVersionMap() invalid device map: key error: %s",
                                   VersionInfoConstants::kNameBuildType);

    absl::StatusOr<std::map<std::string_view, std::string_view>> got = GetDeviceLibraryVersionMap(
    csv_content);

    EXPECT_THAT(got, StatusIs(absl::StatusCode::kFailedPrecondition, want_err_msg))
    << got.status().message();
}

}  // namespace
}  // namespace Dive
