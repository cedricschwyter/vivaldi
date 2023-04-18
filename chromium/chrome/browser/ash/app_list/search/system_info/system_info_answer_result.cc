// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/app_list/search/system_info/system_info_answer_result.h"

#include "ash/public/cpp/app_list/app_list_metrics.h"
#include "ash/webui/diagnostics_ui/url_constants.h"
#include "base/strings/strcat.h"
#include "chrome/browser/ash/app_list/search/common/icon_constants.h"
#include "chrome/browser/ash/app_list/search/common/search_result_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/system_web_apps/system_web_app_ui_utils.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#include "url/gurl.h"

namespace app_list {
namespace {

constexpr char kOsSettingsResultPrefix[] = "os-settings://";

}  // namespace

SystemInfoAnswerResult::SystemInfoAnswerResult(
    Profile* profile,
    const std::u16string& query,
    const std::string& url_path,
    const gfx::ImageSkia& icon,
    double relevance_score,
    const std::u16string& title,
    const std::u16string& description,
    AnswerCardDisplayType card_display_type,
    SystemInfoCategory system_info_category)
    : system_info_category_(system_info_category),
      profile_(profile),
      query_(query),
      url_path_(url_path) {
  // TODO(b/263994165): Handle bar chart display types.
  if (card_display_type == AnswerCardDisplayType::kTextCard) {
    SetDisplayType(DisplayType::kAnswerCard);
  }
  set_relevance(relevance_score);
  SetIcon(IconInfo(icon, kAppIconDimension));
  SetTitle(title);
  SetCategory(Category::kSettings);
  SetResultType(ResultType::kSystemInfo);
  UpdateTitleAndDetails(title, description);
  SetMetricsType(ash::SYSTEM_INFO);
  std::string id =
      system_info_category_ == SystemInfoCategory::kSettings
          ? base::StrCat({kOsSettingsResultPrefix, url_path_})
          : base::StrCat({ash::kChromeUIDiagnosticsAppUrl, url_path_});
  set_id(id);
}

SystemInfoAnswerResult::~SystemInfoAnswerResult() = default;

void SystemInfoAnswerResult::UpdateTitleAndDetails(
    const std::u16string& title,
    const std::u16string& description) {
  std::vector<TextItem> title_vector;
  title_vector.push_back(CreateStringTextItem(title));
  SetTitleTextVector(title_vector);

  std::vector<TextItem> details_vector;
  details_vector.push_back(CreateStringTextItem(description));
  SetDetailsTextVector(details_vector);
}

void SystemInfoAnswerResult::Open(int event_flags) {
  if (system_info_category_ == SystemInfoCategory::kSettings) {
    chrome::SettingsWindowManager::GetInstance()->ShowOSSettings(profile_,
                                                                 url_path_);
  } else {
    ::ash::SystemAppLaunchParams launch_params;
    launch_params.url = GURL(id());
    ash::LaunchSystemWebAppAsync(profile_, ash::SystemWebAppType::DIAGNOSTICS,
                                 launch_params);
  }
}

}  // namespace app_list
