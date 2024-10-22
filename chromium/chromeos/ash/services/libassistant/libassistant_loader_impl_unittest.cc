// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/ash/services/libassistant/libassistant_loader_impl.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "chromeos/ash/components/dbus/dlcservice/dlcservice_client.h"
#include "chromeos/ash/services/assistant/public/cpp/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash::libassistant {

class LibassistantLoaderImplTest : public ::testing::Test {
 public:
  LibassistantLoaderImplTest() = default;
  LibassistantLoaderImplTest(const LibassistantLoaderImplTest&) = delete;
  LibassistantLoaderImplTest& operator=(const LibassistantLoaderImplTest&) =
      delete;
  ~LibassistantLoaderImplTest() override = default;

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  base::test::TaskEnvironment environment_;
};

TEST_F(LibassistantLoaderImplTest, ShouldCreateInstance) {
  auto* loader = LibassistantLoaderImpl::GetInstance();
  EXPECT_TRUE(loader);
}

TEST_F(LibassistantLoaderImplTest, ShouldRunCallbackWithoutDlcFeature) {
  // Enable LibAssistantV2 will also enable LibAssistantDlc. Therefore, in this
  // test, we disable both.
  feature_list_.InitWithFeatures(
      /*enabled_features=*/{},
      /*disabled_features=*/{assistant::features::kEnableLibAssistantDlc,
                             assistant::features::kEnableLibAssistantV2});

  auto* loader = LibassistantLoaderImpl::GetInstance();
  EXPECT_TRUE(loader);

  base::RunLoop run_loop;
  loader->Load(base::BindOnce(
      [](base::RunLoop* run_loop, bool success) {
        EXPECT_TRUE(success);
        run_loop->Quit();
      },
      &run_loop));
  run_loop.Run();
}

TEST_F(LibassistantLoaderImplTest, ShouldRunCallbackWithDlcFeature) {
  feature_list_.InitAndEnableFeature(
      assistant::features::kEnableLibAssistantDlc);

  auto* loader = LibassistantLoaderImpl::GetInstance();
  EXPECT_TRUE(loader);

  // Should fail without dlcservice client.
  base::RunLoop run_loop1;
  loader->Load(base::BindOnce(
      [](base::RunLoop* run_loop, bool success) {
        EXPECT_FALSE(success);
        run_loop->Quit();
      },
      &run_loop1));
  run_loop1.Run();

  // Should success with dlcservice client.
  base::RunLoop run_loop2;
  DlcserviceClient::InitializeFake();
  loader->Load(base::BindOnce(
      [](base::RunLoop* run_loop, bool success) {
        EXPECT_TRUE(success);
        run_loop->Quit();
      },
      &run_loop2));
  run_loop2.Run();
  DlcserviceClient::Shutdown();
}

}  // namespace ash::libassistant
