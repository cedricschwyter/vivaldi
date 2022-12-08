// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/ranking/removed_results_ranker.h"

#include "ash/public/cpp/app_list/app_list_types.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/search/files/file_result.h"
#include "chrome/browser/ui/app_list/search/files/file_suggest_keyed_service_factory.h"
#include "chrome/browser/ui/app_list/search/files/file_suggest_test_util.h"
#include "chrome/browser/ui/app_list/search/files/mock_file_suggest_keyed_service.h"
#include "chrome/browser/ui/app_list/search/ranking/types.h"
#include "chrome/browser/ui/app_list/search/search_controller.h"
#include "chrome/browser/ui/app_list/search/test/ranking_test_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace app_list {

namespace {

using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

std::unique_ptr<TestResult> MakeResult(const std::string& id) {
  return std::make_unique<TestResult>(id);
}

Results MakeResults(std::vector<std::string> ids) {
  Results res;
  for (const std::string& id : ids) {
    res.push_back(MakeResult(id));
  }
  return res;
}

}  // namespace

class RemovedResultsRankerTest : public testing::Test {
 public:
  // testing::Test:
  void SetUp() override {
    testing_profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    EXPECT_TRUE(testing_profile_manager_->SetUp());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    profile_ = testing_profile_manager_->CreateTestingProfile(
        "primary_profile@test",
        {{FileSuggestKeyedServiceFactory::GetInstance(),
          base::BindRepeating(
              &MockFileSuggestKeyedService::BuildMockFileSuggestKeyedService,
              temp_dir_.GetPath().Append("proto"))}});
    WaitUntilFileSuggestServiceReady(
        FileSuggestKeyedServiceFactory::GetInstance()->GetService(profile_));
    ranker_ = std::make_unique<RemovedResultsRanker>(profile_);
  }

  void Wait() { task_environment_.RunUntilIdle(); }

  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<TestingProfileManager> testing_profile_manager_;
  TestingProfile* profile_ = nullptr;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<RemovedResultsRanker> ranker_;
};

TEST_F(RemovedResultsRankerTest, UpdateResultRanks) {
  // Request to remove some results.
  ranker_->Remove(MakeResult("A").get());
  ranker_->Remove(MakeResult("C").get());
  ranker_->Remove(MakeResult("E").get());
  Wait();

  ResultsMap results_map;
  results_map[ResultType::kInstalledApp] = MakeResults({"A", "B"});
  results_map[ResultType::kInternalApp] = MakeResults({"C", "D"});
  results_map[ResultType::kOmnibox] = MakeResults({"E"});

  // Installed apps: The 0th result ("A") is marked to be filtered.
  ranker_->UpdateResultRanks(results_map, ResultType::kInstalledApp);
  EXPECT_TRUE(results_map[ResultType::kInstalledApp][0]->scoring().filter);
  EXPECT_FALSE(results_map[ResultType::kInstalledApp][1]->scoring().filter);

  // Internal apps: The 0th result ("C") is marked to be filtered.
  ranker_->UpdateResultRanks(results_map, ResultType::kInternalApp);
  EXPECT_TRUE(results_map[ResultType::kInternalApp][0]->scoring().filter);
  EXPECT_FALSE(results_map[ResultType::kInternalApp][1]->scoring().filter);

  // Omnibox: The 0th result ("C") is marked to be filtered.
  //
  // TODO(crbug.com/1272361): Ranking here should not affect Omnibox results,
  // after support is added to the autocomplete controller for removal of
  // non-zero state Omnibox results.
  ranker_->UpdateResultRanks(results_map, ResultType::kOmnibox);
  EXPECT_TRUE(results_map[ResultType::kOmnibox][0]->scoring().filter);
}

TEST_F(RemovedResultsRankerTest, RankEmptyResults) {
  Wait();

  ResultsMap results_map;
  results_map[ResultType::kInstalledApp] =
      MakeResults(std::vector<std::string>());

  ranker_->UpdateResultRanks(results_map, ResultType::kInstalledApp);
  EXPECT_TRUE(results_map[ResultType::kInstalledApp].empty());
}

TEST_F(RemovedResultsRankerTest, RankDuplicateResults) {
  Wait();

  // Request to remove some results.
  ranker_->Remove(MakeResult("A").get());
  ranker_->Remove(MakeResult("C").get());
  Wait();

  ResultsMap results_map;
  // Include some duplicated results.
  results_map[ResultType::kInstalledApp] = MakeResults({"A", "A", "B"});
  results_map[ResultType::kInternalApp] = MakeResults({"C", "D"});

  // Installed apps: The 0th and 1st results ("A") are marked to be filtered.
  ranker_->UpdateResultRanks(results_map, ResultType::kInstalledApp);
  EXPECT_TRUE(results_map[ResultType::kInstalledApp][0]->scoring().filter);
  EXPECT_TRUE(results_map[ResultType::kInstalledApp][1]->scoring().filter);
  EXPECT_FALSE(results_map[ResultType::kInstalledApp][2]->scoring().filter);

  // Internal apps: The 0th result ("C") is marked to be filtered.
  ranker_->UpdateResultRanks(results_map, ResultType::kInternalApp);
  EXPECT_TRUE(results_map[ResultType::kInternalApp][0]->scoring().filter);
  EXPECT_FALSE(results_map[ResultType::kInternalApp][1]->scoring().filter);
}

// Verifies that the ranker removes a result through the file suggest keyed
// service if the result is a file suggestion.
TEST_F(RemovedResultsRankerTest, RemoveFileSuggestions) {
  Wait();

  const base::FilePath drive_file_result_path("file_A");
  FileResult drive_file_result(
      "zero_state_drive://" + drive_file_result_path.value(),
      drive_file_result_path, absl::nullopt,
      ash::AppListSearchResultType::kZeroStateDrive,
      ash::SearchResultDisplayType::kList, /*relevance=*/0.5f,
      /*query=*/std::u16string(), FileResult::Type::kFile, profile_);
  MockFileSuggestKeyedService* mock_service =
      static_cast<MockFileSuggestKeyedService*>(
          FileSuggestKeyedServiceFactory::GetInstance()->GetService(profile_));
  auto drive_file_metadata = drive_file_result.CloneMetadata();
  EXPECT_CALL(*mock_service, RemoveSuggestionBySearchResultAndNotify)
      .WillOnce([&](const ash::SearchResultMetadata& search_result) {
        EXPECT_EQ(search_result.result_type, drive_file_metadata->result_type);
        EXPECT_EQ(search_result.id, drive_file_metadata->id);
      });
  ranker_->Remove(&drive_file_result);

  const base::FilePath local_file_path("file_B");
  FileResult local_file_result(
      "zero_state_file://" + local_file_path.value(), local_file_path,
      absl::nullopt, ash::AppListSearchResultType::kZeroStateDrive,
      ash::SearchResultDisplayType::kList, /*relevance=*/0.5f,
      /*query=*/std::u16string(), FileResult::Type::kFile, profile_);
  auto local_file_metadata = local_file_result.CloneMetadata();
  EXPECT_CALL(*mock_service, RemoveSuggestionBySearchResultAndNotify)
      .WillOnce([&](const ash::SearchResultMetadata& search_result) {
        EXPECT_EQ(search_result.result_type, local_file_metadata->result_type);
        EXPECT_EQ(search_result.id, local_file_metadata->id);
      });
  ranker_->Remove(&local_file_result);
}

}  // namespace app_list
