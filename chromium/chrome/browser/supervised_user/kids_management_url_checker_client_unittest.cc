// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/supervised_user/core/browser/kids_management_url_checker_client.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "base/values.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/supervised_user/kids_chrome_management/kids_chrome_management_client_factory.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/account_id/account_id.h"
#include "components/supervised_user/core/browser/kids_chrome_management_client.h"
#include "components/supervised_user/core/browser/proto/kidschromemanagement_messages.pb.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/login/users/fake_chrome_user_manager.h"
#include "components/user_manager/scoped_user_manager.h"
#endif

using testing::_;

namespace {

using kids_chrome_management::ClassifyUrlResponse;

ClassifyUrlResponse::DisplayClassification ConvertClassification(
    safe_search_api::ClientClassification classification) {
  switch (classification) {
    case safe_search_api::ClientClassification::kAllowed:
      return ClassifyUrlResponse::ALLOWED;
    case safe_search_api::ClientClassification::kRestricted:
      return ClassifyUrlResponse::RESTRICTED;
    case safe_search_api::ClientClassification::kUnknown:
      return ClassifyUrlResponse::UNKNOWN_DISPLAY_CLASSIFICATION;
  }
}

// Build fake response proto with a response according to |classification|.
std::unique_ptr<ClassifyUrlResponse> BuildResponseProto(
    safe_search_api::ClientClassification classification) {
  auto response_proto = std::make_unique<ClassifyUrlResponse>();

  response_proto->set_display_classification(
      ConvertClassification(classification));
  return response_proto;
}

class KidsChromeManagementClientForTesting : public KidsChromeManagementClient {
 public:
  explicit KidsChromeManagementClientForTesting(
      content::BrowserContext* context)
      : KidsChromeManagementClient(Profile::FromBrowserContext(context)
                                       ->GetDefaultStoragePartition()
                                       ->GetURLLoaderFactoryForBrowserProcess(),
                                   IdentityManagerFactory::GetForProfile(
                                       Profile::FromBrowserContext(context))) {}

  KidsChromeManagementClientForTesting(
      const KidsChromeManagementClientForTesting&) = delete;
  KidsChromeManagementClientForTesting& operator=(
      const KidsChromeManagementClientForTesting&) = delete;

  ~KidsChromeManagementClientForTesting() override = default;

  void ClassifyURL(
      std::unique_ptr<kids_chrome_management::ClassifyUrlRequest> request_proto,
      KidsChromeManagementClient::KidsChromeManagementCallback callback)
      override {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  std::move(response_proto_), error_code_));
  }

  void SetupResponse(std::unique_ptr<ClassifyUrlResponse> response_proto,
                     KidsChromeManagementClient::ErrorCode error_code) {
    response_proto_ = std::move(response_proto);
    error_code_ = error_code;
  }

 private:
  std::unique_ptr<ClassifyUrlResponse> response_proto_;
  KidsChromeManagementClient::ErrorCode error_code_;
};

}  // namespace

class KidsManagementURLCheckerClientTest : public testing::Test {
 public:
  KidsManagementURLCheckerClientTest() = default;

  KidsManagementURLCheckerClientTest(
      const KidsManagementURLCheckerClientTest&) = delete;
  KidsManagementURLCheckerClientTest& operator=(
      const KidsManagementURLCheckerClientTest&) = delete;

  void SetUp() override {
    test_profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(test_profile_manager_->SetUp());

// ChromeOS requires an ash::FakeChromeUserManager for the tests to work.
#if BUILDFLAG(IS_CHROMEOS_ASH)
    const char kEmail[] = "account@gmail.com";
    const AccountId test_account_id(AccountId::FromUserEmail(kEmail));
    user_manager_ = new ash::FakeChromeUserManager;
    user_manager_->AddUser(test_account_id);
    user_manager_->LoginUser(test_account_id);
    user_manager_->SwitchActiveUser(test_account_id);
    test_profile_ = test_profile_manager_->CreateTestingProfile(
        test_account_id.GetUserEmail());

    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        base::WrapUnique(user_manager_));
#else
    test_profile_ =
        test_profile_manager_->CreateTestingProfile(chrome::kInitialProfile);
#endif

    DCHECK(test_profile_);

    test_kids_chrome_management_client_ =
        std::make_unique<KidsChromeManagementClientForTesting>(test_profile_);
    url_classifier_ = std::make_unique<KidsManagementURLCheckerClient>(
        test_kids_chrome_management_client_.get(), "us");
  }

 protected:
  void SetupClientResponse(std::unique_ptr<ClassifyUrlResponse> response_proto,
                           KidsChromeManagementClient::ErrorCode error_code) {
    test_kids_chrome_management_client_->SetupResponse(
        std::move(response_proto), error_code);
  }

  // Asynchronously checks the URL and waits until finished.
  void CheckURL(const GURL& url) {
    StartCheckURL(url);
    task_environment_.RunUntilIdle();
  }

  // Starts a URL check, but doesn't wait for ClassifyURL() to finish.
  void CheckURLWithoutResponse(const GURL& url) { StartCheckURL(url); }

  MOCK_METHOD2(OnCheckDone,
               void(const GURL& url,
                    safe_search_api::ClientClassification classification));

  content::BrowserTaskEnvironment task_environment_;
  raw_ptr<TestingProfile> test_profile_;
  std::unique_ptr<TestingProfileManager> test_profile_manager_;
  std::unique_ptr<KidsChromeManagementClientForTesting>
      test_kids_chrome_management_client_;
  std::unique_ptr<KidsManagementURLCheckerClient> url_classifier_;
#if BUILDFLAG(IS_CHROMEOS_ASH)
  ash::FakeChromeUserManager* user_manager_;
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
#endif

 private:
  void StartCheckURL(const GURL& url) {
    url_classifier_->CheckURL(
        url, base::BindOnce(&KidsManagementURLCheckerClientTest::OnCheckDone,
                            base::Unretained(this)));
  }
};

TEST_F(KidsManagementURLCheckerClientTest, Simple) {
  {
    GURL url("http://randomurl1.com");

    safe_search_api::ClientClassification classification =
        safe_search_api::ClientClassification::kAllowed;

    EXPECT_CALL(*this, OnCheckDone(url, classification));

    SetupClientResponse(BuildResponseProto(classification),
                        KidsChromeManagementClient::ErrorCode::kSuccess);

    CheckURL(url);
  }
  {
    GURL url("http://randomurl2.com");

    safe_search_api::ClientClassification classification =
        safe_search_api::ClientClassification::kRestricted;

    EXPECT_CALL(*this, OnCheckDone(url, classification));

    SetupClientResponse(BuildResponseProto(classification),
                        KidsChromeManagementClient::ErrorCode::kSuccess);
    CheckURL(url);
  }
}

TEST_F(KidsManagementURLCheckerClientTest, AccessTokenError) {
  GURL url("http://randomurl3.com");

  safe_search_api::ClientClassification classification =
      safe_search_api::ClientClassification::kUnknown;

  SetupClientResponse(BuildResponseProto(classification),
                      KidsChromeManagementClient::ErrorCode::kTokenError);

  EXPECT_CALL(*this, OnCheckDone(url, classification));
  CheckURL(url);
}

TEST_F(KidsManagementURLCheckerClientTest, NetworkErrors) {
  {
    GURL url("http://randomurl4.com");

    safe_search_api::ClientClassification classification =
        safe_search_api::ClientClassification::kUnknown;

    SetupClientResponse(BuildResponseProto(classification),
                        KidsChromeManagementClient::ErrorCode::kNetworkError);

    EXPECT_CALL(*this, OnCheckDone(url, classification));

    CheckURL(url);
  }

  {
    GURL url("http://randomurl5.com");

    safe_search_api::ClientClassification classification =
        safe_search_api::ClientClassification::kUnknown;

    SetupClientResponse(BuildResponseProto(classification),
                        KidsChromeManagementClient::ErrorCode::kHttpError);

    EXPECT_CALL(*this, OnCheckDone(url, classification));

    CheckURL(url);
  }
}

TEST_F(KidsManagementURLCheckerClientTest, ServiceError) {
  GURL url("http://randomurl6.com");

  safe_search_api::ClientClassification classification =
      safe_search_api::ClientClassification::kUnknown;

  SetupClientResponse(BuildResponseProto(classification),
                      KidsChromeManagementClient::ErrorCode::kServiceError);

  EXPECT_CALL(*this, OnCheckDone(url, classification));
  CheckURL(url);
}

TEST_F(KidsManagementURLCheckerClientTest, DestroyClientBeforeCallback) {
  GURL url("http://randomurl7.com");

  EXPECT_CALL(*this, OnCheckDone(_, _)).Times(0);
  CheckURLWithoutResponse(url);

  // Destroy the URLCheckerClient.
  url_classifier_.reset();

  // Now run the callback.
  task_environment_.RunUntilIdle();
}
