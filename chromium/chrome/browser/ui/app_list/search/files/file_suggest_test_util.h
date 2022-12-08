// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_FILES_FILE_SUGGEST_TEST_UTIL_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_FILES_FILE_SUGGEST_TEST_UTIL_H_

#include <string>
#include <vector>

#include "chrome/browser/ui/app_list/search/files/mock_file_suggest_keyed_service_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace app_list {

/*
The suggest item metadata. It matches the json response used by
`ItemSuggestCache`. A sample json response is listed as below:
 R"(
    {
      "item": [
        {
          "itemId": "id",
          "displayText": "text",
          "predictionReason": "reason"
        }
      ],
      "suggestionSessionId": "session id"
    })";
*/
struct SuggestItemMetadata {
  std::string item_id;
  std::string display_text;
  std::string prediction_reason;
};

// Creates a json string used to update the item suggest cache.
std::string CreateItemSuggestUpdateJsonString(
    const std::vector<SuggestItemMetadata>& item_data_array,
    const std::string& session_id);

// Waits until `mock` is notified of the file suggestion update.
void WaitForFileSuggestionUpdate(
    const testing::NiceMock<MockFileSuggestKeyedServiceObserver>& mock,
    app_list::FileSuggestionType expected_type);

// Waits until `service` is ready.
void WaitUntilFileSuggestServiceReady(FileSuggestKeyedService* service);

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_FILES_FILE_SUGGEST_TEST_UTIL_H_
