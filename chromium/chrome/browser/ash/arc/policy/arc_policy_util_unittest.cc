// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/arc/policy/arc_policy_util.h"

#include <map>
#include <string>

#include "base/json/json_writer.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc::policy_util {

namespace {

constexpr int kUnknownBucket = 0;
constexpr int kOptionalBucket = 1;
constexpr int kRequiredBucket = 2;
constexpr int kPreloadBucket = 3;
constexpr int kForceInstalledBucket = 4;
constexpr int kBlockedBucket = 5;
constexpr int kAvailableBucket = 6;
constexpr int kRequiredForSetupBucket = 7;
constexpr int kKioskBucket = 8;
constexpr char kInstallTypeHistogram[] = "Arc.Policy.InstallTypesOnDevice";

std::map<std::string, std::string> kTestMap = {
    {"testPackage", "FORCE_INSTALLED"}, {"testPackage2", "BLOCKED"},
    {"testPackage3", "BLOCKED"},        {"testPackage4", "AVAILABLE"},
    {"testPackage5", "AVAILABLE"},      {"testPackage6", "REQUIRED"}};

}  // namespace

class ArcPolicyUtilTest : public testing::Test {
 public:
  ArcPolicyUtilTest(const ArcPolicyUtilTest&) = delete;
  ArcPolicyUtilTest& operator=(const ArcPolicyUtilTest&) = delete;

 protected:
  ArcPolicyUtilTest() = default;
  base::HistogramTester tester;
};

std::string CreatePolicyWithAppInstalls(
    std::map<std::string, std::string> package_map) {
  base::DictionaryValue arc_policy;
  auto list = std::make_unique<base::ListValue>();

  for (auto entry : package_map) {
    base::Value::Dict package;
    package.Set("packageName", entry.first);
    package.Set("installType", entry.second);
    list->Append(base::Value(std::move(package)));
  }

  arc_policy.SetList("applications", std::move(list));
  std::string arc_policy_string;
  base::JSONWriter::Write(arc_policy, &arc_policy_string);
  return arc_policy_string;
}

TEST_F(ArcPolicyUtilTest, GetRequestedPackagesFromArcPolicy) {
  std::set<std::string> expected = {"testPackage", "testPackage6"};
  std::string policy = CreatePolicyWithAppInstalls(kTestMap);
  std::set<std::string> result =
      arc::policy_util::GetRequestedPackagesFromArcPolicy(policy);

  EXPECT_EQ(result, expected);
}

TEST_F(ArcPolicyUtilTest, RecordInstallTypesInPolicy_OneOfEachType) {
  std::map<std::string, std::string> test_map = {
      {"testPackage", "OPTIONAL"},
      {"testPackage2", "REQUIRED"},
      {"testPackage3", "PRELOAD"},
      {"testPackage4", "FORCE_INSTALLED"},
      {"testPackage5", "BLOCKED"},
      {"testPackage6", "AVAILABLE"},
      {"testPackage7", "REQUIRED_FOR_SETUP"},
      {"testPackage8", "KIOSK"},
      {"testPackage9", "UNKNOWN"}};

  std::string policy = CreatePolicyWithAppInstalls(test_map);
  arc::policy_util::RecordInstallTypesInPolicy(policy);

  tester.ExpectBucketCount(kInstallTypeHistogram, kUnknownBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kOptionalBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kRequiredBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kPreloadBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kForceInstalledBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kBlockedBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kAvailableBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kRequiredForSetupBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kKioskBucket, 1);
  tester.ExpectTotalCount(kInstallTypeHistogram, 9);
}

TEST_F(ArcPolicyUtilTest, RecordInstallTypesInPolicy_ComplexPolicy) {
  std::string policy = CreatePolicyWithAppInstalls(kTestMap);
  arc::policy_util::RecordInstallTypesInPolicy(policy);

  tester.ExpectBucketCount(kInstallTypeHistogram, kForceInstalledBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kBlockedBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kAvailableBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kRequiredBucket, 1);
  tester.ExpectTotalCount(kInstallTypeHistogram, 4);
}

TEST_F(ArcPolicyUtilTest, RecordInstallTypesInPolicy_PolicyUpdate) {
  std::string policy = CreatePolicyWithAppInstalls(kTestMap);
  arc::policy_util::RecordInstallTypesInPolicy(policy);

  tester.ExpectBucketCount(kInstallTypeHistogram, kForceInstalledBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kBlockedBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kAvailableBucket, 1);
  tester.ExpectBucketCount(kInstallTypeHistogram, kRequiredBucket, 1);
  tester.ExpectTotalCount(kInstallTypeHistogram, 4);

  kTestMap["anotherTestPackage"] = "BLOCKED";
  kTestMap["anotherTestPackage2"] = "KIOSK";
  policy = CreatePolicyWithAppInstalls(kTestMap);
  arc::policy_util::RecordInstallTypesInPolicy(policy);
  tester.ExpectBucketCount(kInstallTypeHistogram, kForceInstalledBucket, 2);
  tester.ExpectBucketCount(kInstallTypeHistogram, kBlockedBucket, 2);
  tester.ExpectBucketCount(kInstallTypeHistogram, kAvailableBucket, 2);
  tester.ExpectBucketCount(kInstallTypeHistogram, kRequiredBucket, 2);
  tester.ExpectBucketCount(kInstallTypeHistogram, kKioskBucket, 1);
  tester.ExpectTotalCount(kInstallTypeHistogram, 9);
}

}  // namespace arc::policy_util
