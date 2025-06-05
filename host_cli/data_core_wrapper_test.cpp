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

#include "data_core_wrapper.h"

#include <gtest/gtest.h>

namespace Dive
{
namespace HostCli
{
namespace
{

TEST(DataCoreWrapperTest, CheckInitialization)
{
    DataCoreWrapper data_core_wrapper;
    ASSERT_EQ(data_core_wrapper.IsDataCoreInitialized(), true);
    ASSERT_EQ(data_core_wrapper.IsGfxrLoaded(), false);
}

// TODO : Write more tests if it's possible to hook up actual files from tests/ or with a mock
// DataCore

}  // namespace
}  // namespace HostCli
}  // namespace Dive