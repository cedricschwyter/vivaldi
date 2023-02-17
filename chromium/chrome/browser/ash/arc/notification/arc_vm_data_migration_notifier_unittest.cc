// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/arc/notification/arc_vm_data_migration_notifier.h"

#include "ash/components/arc/arc_features.h"
#include "ash/components/arc/arc_prefs.h"
#include "ash/components/arc/session/arc_session_runner.h"
#include "ash/components/arc/session/arc_vm_data_migration_status.h"
#include "ash/components/arc/test/fake_arc_session.h"
#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ash/arc/session/arc_session_manager.h"
#include "chrome/browser/ash/arc/test/test_arc_session_manager.h"
#include "chrome/browser/ash/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "chromeos/ash/components/dbus/concierge/concierge_client.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

namespace {

constexpr char kProfileName[] = "user@gmail.com";
constexpr char kGaiaId[] = "1234567890";

constexpr char kNotificationId[] = "arc_vm_data_migration_notification";

class ArcVmDataMigrationNotifierTest : public testing::Test {
 public:
  ArcVmDataMigrationNotifierTest() {
    base::CommandLine::ForCurrentProcess()->InitFromArgv(
        {"", "--arc-availability=officially-supported", "--enable-arcvm"});
  }

  ~ArcVmDataMigrationNotifierTest() override = default;

  ArcVmDataMigrationNotifierTest(const ArcVmDataMigrationNotifierTest&) =
      delete;
  ArcVmDataMigrationNotifierTest& operator=(
      const ArcVmDataMigrationNotifierTest&) = delete;

  void SetUp() override {
    ash::ConciergeClient::InitializeFake();
    ArcSessionManager::SetUiEnabledForTesting(false);
    arc_session_manager_ =
        CreateTestArcSessionManager(std::make_unique<ArcSessionRunner>(
            base::BindRepeating(FakeArcSession::Create)));

    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    testing_profile_ = profile_manager_->CreateTestingProfile(kProfileName);
    const AccountId account_id = AccountId::FromUserEmailGaiaId(
        testing_profile_->GetProfileUserName(), kGaiaId);
    auto fake_user_manager = std::make_unique<ash::FakeChromeUserManager>();
    fake_user_manager->AddUser(account_id);
    fake_user_manager->LoginUser(account_id);
    user_manager_ = std::make_unique<user_manager::ScopedUserManager>(
        std::move(fake_user_manager));
    DCHECK(ash::ProfileHelper::IsPrimaryProfile(testing_profile_));

    notification_tester_ =
        std::make_unique<NotificationDisplayServiceTester>(testing_profile_);
    arc_vm_data_migration_notifier_ =
        std::make_unique<ArcVmDataMigrationNotifier>(testing_profile_);

    arc_session_manager_->SetProfile(testing_profile_);
    arc_session_manager_->Initialize();
  }

  void TearDown() override {
    arc_session_manager_->Shutdown();
    notification_tester_.reset();
    user_manager_.reset();
    profile_manager_->DeleteTestingProfile(kProfileName);
    testing_profile_ = nullptr;
    profile_manager_.reset();
    arc_vm_data_migration_notifier_.reset();
    arc_session_manager_.reset();
    ash::ConciergeClient::Shutdown();
  }

  ArcSessionManager* arc_session_manager() {
    return arc_session_manager_.get();
  }

  NotificationDisplayServiceTester* notification_tester() {
    return notification_tester_.get();
  }

  TestingProfile* profile() { return testing_profile_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ArcSessionManager> arc_session_manager_;
  std::unique_ptr<ArcVmDataMigrationNotifier> arc_vm_data_migration_notifier_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  TestingProfile* testing_profile_ = nullptr;  // Owned by |profile_manager_|.
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_;
  std::unique_ptr<NotificationDisplayServiceTester> notification_tester_;
};

// Tests that no notification is shown when the migration is disabled.
TEST_F(ArcVmDataMigrationNotifierTest, MigrationDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kEnableArcVmDataMigration);

  arc_session_manager()->StartArcForTesting();
  EXPECT_FALSE(notification_tester()->GetNotification(kNotificationId));
  EXPECT_EQ(
      ArcVmDataMigrationStatus::kUnnotified,
      static_cast<ArcVmDataMigrationStatus>(
          profile()->GetPrefs()->GetInteger(prefs::kArcVmDataMigrationStatus)));
}

// Tests that a notification is shown when the migration is enabled but not
// started nor finished yet.
TEST_F(ArcVmDataMigrationNotifierTest, MigrationEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kEnableArcVmDataMigration);

  arc_session_manager()->StartArcForTesting();
  EXPECT_TRUE(notification_tester()->GetNotification(kNotificationId));
  EXPECT_EQ(
      ArcVmDataMigrationStatus::kNotified,
      static_cast<ArcVmDataMigrationStatus>(
          profile()->GetPrefs()->GetInteger(prefs::kArcVmDataMigrationStatus)));
}

// Tests that a notification is shown when the user has been notified of the
// migration but not started the migration yet.
TEST_F(ArcVmDataMigrationNotifierTest, MigrationNotified) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kEnableArcVmDataMigration);
  profile()->GetPrefs()->SetInteger(
      prefs::kArcVmDataMigrationStatus,
      static_cast<int>(ArcVmDataMigrationStatus::kNotified));

  arc_session_manager()->StartArcForTesting();
  EXPECT_TRUE(notification_tester()->GetNotification(kNotificationId));
  EXPECT_EQ(
      ArcVmDataMigrationStatus::kNotified,
      static_cast<ArcVmDataMigrationStatus>(
          profile()->GetPrefs()->GetInteger(prefs::kArcVmDataMigrationStatus)));
}

// Tests that a notification is shown when the user has confirmed the migration
// but ARC session is started without entering the migration screen.
TEST_F(ArcVmDataMigrationNotifierTest, MigrationConfirmed) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kEnableArcVmDataMigration);
  profile()->GetPrefs()->SetInteger(
      prefs::kArcVmDataMigrationStatus,
      static_cast<int>(ArcVmDataMigrationStatus::kConfirmed));

  arc_session_manager()->StartArcForTesting();
  EXPECT_TRUE(notification_tester()->GetNotification(kNotificationId));
  // The migration status is set back to kNotified.
  EXPECT_EQ(
      ArcVmDataMigrationStatus::kNotified,
      static_cast<ArcVmDataMigrationStatus>(
          profile()->GetPrefs()->GetInteger(prefs::kArcVmDataMigrationStatus)));
}

// Tests that no notification is shown once the migration has started.
TEST_F(ArcVmDataMigrationNotifierTest, MigrationStarted) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kEnableArcVmDataMigration);
  profile()->GetPrefs()->SetInteger(
      prefs::kArcVmDataMigrationStatus,
      static_cast<int>(ArcVmDataMigrationStatus::kStarted));

  arc_session_manager()->StartArcForTesting();
  EXPECT_FALSE(notification_tester()->GetNotification(kNotificationId));
  EXPECT_EQ(
      ArcVmDataMigrationStatus::kStarted,
      static_cast<ArcVmDataMigrationStatus>(
          profile()->GetPrefs()->GetInteger(prefs::kArcVmDataMigrationStatus)));
}

// Tests that no notification is shown once the migration has finished.
TEST_F(ArcVmDataMigrationNotifierTest, MigrationFinished) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kEnableArcVmDataMigration);
  profile()->GetPrefs()->SetInteger(
      prefs::kArcVmDataMigrationStatus,
      static_cast<int>(ArcVmDataMigrationStatus::kFinished));

  arc_session_manager()->StartArcForTesting();
  EXPECT_FALSE(notification_tester()->GetNotification(kNotificationId));
  EXPECT_EQ(
      ArcVmDataMigrationStatus::kFinished,
      static_cast<ArcVmDataMigrationStatus>(
          profile()->GetPrefs()->GetInteger(prefs::kArcVmDataMigrationStatus)));
}

// Tests that no notification is shown even when the migration is enabled if
// virtio-blk /data is forcibly enabled via kEnableVirtioBlkForData.
TEST_F(ArcVmDataMigrationNotifierTest, VirtioBlkDataForced) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {kEnableArcVmDataMigration, kEnableVirtioBlkForData}, {});

  arc_session_manager()->StartArcForTesting();
  EXPECT_FALSE(notification_tester()->GetNotification(kNotificationId));
}

}  // namespace

}  // namespace arc
