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

#include <string>
#include "gtest/gtest.h"
#include "android_application.h"

namespace Dive
{
namespace
{

TEST(ParsePackageForActivityTest, StandardSingleActivity)
{
    const std::string kInput = R"(
Activity Resolver Table:
  Non-Data Actions:
      android.intent.action.MAIN:
        1e368bb de.saschawillems.vulkanBloom/de.saschawillems.vulkanSample.VulkanActivity filter 35ec1d8
          Action: "android.intent.action.MAIN"
          Category: "android.intent.category.LAUNCHER"
)";
    EXPECT_EQ(AndroidApplication::ParsePackageForActivity(kInput, "de.saschawillems.vulkanBloom"),
              "de.saschawillems.vulkanSample.VulkanActivity");
}

TEST(ParsePackageForActivityTest, MultipleMainActivitiesTakesFirst)
{
    const std::string kInput = R"(
Activity Resolver Table:
  Non-Data Actions:
      android.intent.action.MAIN:
        82a3ab5 com.example.app/com.example.app.MainActivity filter 82a3ab5
          Action: "android.intent.action.MAIN"
          Category: "android.intent.category.LAUNCHER"
        6776c7b com.example.app/com.example.app.TvActivity filter 608da98
          Action: "android.intent.action.MAIN"
          Category: "android.intent.category.LEANBACK_LAUNCHER"
)";
    EXPECT_EQ(AndroidApplication::ParsePackageForActivity(kInput, "com.example.app"),
              "com.example.app.MainActivity");
}

TEST(ParsePackageForActivityTest, RegressionTestMixedActions)
{
    const std::string kInput = R"(
Activity Resolver Table:
  Non-Data Actions:
      android.intent.action.MAIN:
        82a3ab5 com.google.android.apps.gmm.dev/com.google.android.maps.MapsActivity filter 82a3ab5
          Action: "android.intent.action.MAIN"
          Category: "android.intent.category.LAUNCHER"
      android.intent.action.VIEW:
        89fc44d com.google.android.apps.gmm.dev/com.google.android.maps.driveabout.app.NavigationActivity filter a643b02
          Action: "android.intent.action.VIEW"
          Category: "android.intent.category.DEFAULT"
      com.google.android.primes.action.DEBUG_PRIMES_EVENTS:
        383e5b9 com.google.android.apps.gmm.dev/com.google.android.libraries.performance.primes.debug.PrimesEventActivity filter 53d5dfe
          Action: "com.google.android.primes.action.DEBUG_PRIMES_EVENTS"
          Category: "android.intent.category.DEFAULT"
  MIME Typed Actions:
      android.intent.action.SEND:
        bd1f6ec com.google.android.apps.gmm.dev/com.google.android.maps.MapsActivity filter fb1b997
)";
    EXPECT_EQ(AndroidApplication::ParsePackageForActivity(kInput,
                                                          "com.google.android.apps.gmm.dev"),
              "com.google.android.maps.MapsActivity");
}

TEST(ParsePackageForActivityTest, ReturnsEmptyIfNotFound)
{
    const std::string kInput = R"(
Activity Resolver Table:
  Non-Data Actions:
      android.intent.action.VIEW:
        89fc44d com.example.app/com.example.app.DeepLinkActivity filter a643b02
)";
    EXPECT_EQ(AndroidApplication::ParsePackageForActivity(kInput, "com.example.app"), "");
}

TEST(ParsePackageForActivityTest, ReturnsEmptyIfPackageNotInMain)
{
    const std::string kInput = R"(
Activity Resolver Table:
  Non-Data Actions:
      android.intent.action.MAIN:
        82a3ab5 com.other.app/com.other.app.MainActivity filter 82a3ab5
)";
    EXPECT_EQ(AndroidApplication::ParsePackageForActivity(kInput, "com.example.app"), "");
}

}  // namespace
}  // namespace Dive