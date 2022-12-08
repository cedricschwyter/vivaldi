// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/segmentation_platform/segmentation_platform_config.h"

#include <memory>

#include "base/containers/cxx20_erase_vector.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "components/segmentation_platform/embedder/default_model/cross_device_user_segment.h"
#include "components/segmentation_platform/embedder/default_model/feed_user_segment.h"
#include "components/segmentation_platform/embedder/default_model/frequent_feature_user_model.h"
#include "components/segmentation_platform/embedder/default_model/low_user_engagement_model.h"
#include "components/segmentation_platform/embedder/default_model/resume_heavy_user_model.h"
#include "components/segmentation_platform/embedder/default_model/search_user_model.h"
#include "components/segmentation_platform/embedder/default_model/shopping_user_model.h"
#include "components/segmentation_platform/internal/config_parser.h"
#include "components/segmentation_platform/public/config.h"
#include "components/segmentation_platform/public/constants.h"
#include "components/segmentation_platform/public/features.h"
#include "components/segmentation_platform/public/proto/segmentation_platform.pb.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/commerce/shopping_service_factory.h"
#include "chrome/browser/feature_guide/notifications/feature_notification_guide_service.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "chrome/browser/segmentation_platform/default_model/chrome_start_model_android.h"
#include "chrome/browser/segmentation_platform/default_model/chrome_start_model_android_v2.h"
#include "components/commerce/core/commerce_feature_list.h"
#include "components/commerce/core/shopping_service.h"
#include "components/segmentation_platform/embedder/default_model/intentional_user_model.h"
#include "components/segmentation_platform/embedder/default_model/power_user_segment.h"
#include "components/segmentation_platform/embedder/default_model/price_tracking_action_model.h"
#include "components/segmentation_platform/embedder/default_model/query_tiles_model.h"
#include "components/segmentation_platform/embedder/input_delegate/price_tracking_input_delegate.h"
#endif

namespace segmentation_platform {

using proto::SegmentId;

namespace {

constexpr int kChromeLowUserEngagementSelectionTTLDays = 7;

#if BUILDFLAG(IS_ANDROID)

constexpr int kAdaptiveToolbarDefaultSelectionTTLDays = 56;

#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
std::unique_ptr<Config> GetConfigForAdaptiveToolbar() {
  auto config = std::make_unique<Config>();
  config->segmentation_key = kAdaptiveToolbarSegmentationKey;
  config->segmentation_uma_name = kAdaptiveToolbarUmaName;

  int segment_selection_ttl_days = base::GetFieldTrialParamByFeatureAsInt(
      chrome::android::kAdaptiveButtonInTopToolbarCustomizationV2,
      kVariationsParamNameSegmentSelectionTTLDays,
      kAdaptiveToolbarDefaultSelectionTTLDays);
  config->segment_selection_ttl = base::Days(segment_selection_ttl_days);
  // Do not set unknown TTL so that the platform ignores unknown results.

  // A hardcoded list of segment IDs known to the segmentation platform.
  config->AddSegmentId(SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB);
  config->AddSegmentId(SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE);
  config->AddSegmentId(SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE);

  return config;
}
#endif

#if BUILDFLAG(IS_ANDROID)
bool IsEnabledContextualPageActions() {
  if (!base::FeatureList::IsEnabled(features::kContextualPageActions))
    return false;

  bool is_price_tracking_enabled =
      base::FeatureList::IsEnabled(
          features::kContextualPageActionPriceTracking) &&
      base::FeatureList::IsEnabled(commerce::kShoppingList);

  bool is_reader_mode_enabled =
      base::FeatureList::IsEnabled(features::kContextualPageActionReaderMode);

  return is_price_tracking_enabled || is_reader_mode_enabled;
}

std::unique_ptr<Config> GetConfigForContextualPageActions(
    content::BrowserContext* context) {
  auto config = std::make_unique<Config>();
  config->segmentation_key = kContextualPageActionsKey;
  config->segmentation_uma_name = kContextualPageActionsUmaName;
  config->AddSegmentId(
      SegmentId::OPTIMIZATION_TARGET_CONTEXTUAL_PAGE_ACTION_PRICE_TRACKING,
      std::make_unique<PriceTrackingActionModel>());

  auto shopping_service_getter = base::BindRepeating(
      commerce::ShoppingServiceFactory::GetForBrowserContextIfExists, context);
  auto bookmark_model_getter =
      base::BindRepeating(BookmarkModelFactory::GetForBrowserContext, context);
  auto price_tracking_input_delegate =
      std::make_unique<processing::PriceTrackingInputDelegate>(
          shopping_service_getter, bookmark_model_getter);
  config->input_delegates[proto::CustomInput_FillPolicy_PRICE_TRACKING_HINTS] =
      std::move(price_tracking_input_delegate);
  config->on_demand_execution = true;
  return config;
}

#endif  // BUILDFLAG(IS_ANDROID)

bool IsLowEngagementFeatureEnabled() {
  // TODO(ssid): Remove this extra feature and change feature guide to use the
  // segmentation defined feature.
#if BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(
          feature_guide::features::kSegmentationModelLowEngagedUsers)) {
    return true;
  }
#endif
  return base::FeatureList::IsEnabled(
      features::kSegmentationPlatformLowEngagementFeature);
}

std::unique_ptr<ModelProvider> GetLowEngagementDefaultModel() {
  if (!base::GetFieldTrialParamByFeatureAsBool(
          features::kSegmentationPlatformLowEngagementFeature,
          kDefaultModelEnabledParam, true)) {
    return nullptr;
  }
  return std::make_unique<LowUserEngagementModel>();
}
std::unique_ptr<Config> GetConfigForChromeLowUserEngagement() {
  auto config = std::make_unique<Config>();
  config->segmentation_key = kChromeLowUserEngagementSegmentationKey;
  config->segmentation_uma_name = kChromeLowUserEngagementUmaName;
  config->AddSegmentId(
      SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_CHROME_LOW_USER_ENGAGEMENT,
      GetLowEngagementDefaultModel());

#if BUILDFLAG(IS_ANDROID)
  int segment_selection_ttl_days = base::GetFieldTrialParamByFeatureAsInt(
      feature_guide::features::kSegmentationModelLowEngagedUsers,
      kVariationsParamNameSegmentSelectionTTLDays,
      kChromeLowUserEngagementSelectionTTLDays);
#else
  int segment_selection_ttl_days = base::GetFieldTrialParamByFeatureAsInt(
      features::kSegmentationPlatformLowEngagementFeature,
      kVariationsParamNameSegmentSelectionTTLDays,
      kChromeLowUserEngagementSelectionTTLDays);
#endif

  config->segment_selection_ttl = base::Days(segment_selection_ttl_days);
  config->unknown_selection_ttl = base::Days(segment_selection_ttl_days);
  return config;
}

}  // namespace

std::vector<std::unique_ptr<Config>> GetSegmentationPlatformConfig(
    content::BrowserContext* context) {
  std::vector<std::unique_ptr<Config>> configs;
#if BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(
          chrome::android::kAdaptiveButtonInTopToolbarCustomizationV2)) {
    configs.emplace_back(GetConfigForAdaptiveToolbar());
  }
  if (IsEnabledContextualPageActions()) {
    configs.emplace_back(GetConfigForContextualPageActions(context));
  }

  configs.emplace_back(ChromeStartModel::GetConfig());
  configs.emplace_back(QueryTilesModel::GetConfig());
  configs.emplace_back(ChromeStartModelV2::GetConfig());
  configs.emplace_back(IntentionalUserModel::GetConfig());
  configs.emplace_back(PowerUserSegment::GetConfig());
  configs.emplace_back(FrequentFeatureUserModel::GetConfig());
#endif
  // TODO(ssid): Move this check into the model.
  if (IsLowEngagementFeatureEnabled()) {
    configs.emplace_back(GetConfigForChromeLowUserEngagement());
  }

  configs.emplace_back(SearchUserModel::GetConfig());
  configs.emplace_back(FeedUserSegment::GetConfig());
  configs.emplace_back(ShoppingUserModel::GetConfig());
  configs.emplace_back(CrossDeviceUserSegment::GetConfig());
  configs.emplace_back(ResumeHeavyUserModel::GetConfig());

  base::EraseIf(configs, [](const auto& config) { return !config.get(); });

  AppendConfigsFromExperiments(configs);
  return configs;
}

void AppendConfigsFromExperiments(
    std::vector<std::unique_ptr<Config>>& out_configs) {
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);
  std::vector<std::string> param_values;

  for (const auto& active_group : active_groups) {
    base::FieldTrialParams params;
    if (base::GetFieldTrialParams(active_group.trial_name, &params)) {
      const auto& it = params.find(kSegmentationConfigParamName);
      if (it == params.end())
        continue;
      param_values.push_back(it->second);
    }
  }

  for (const std::string& param : param_values) {
    auto config = ParseConfigFromString(param);
    VLOG(1) << "Segmentation config param from experiment, "
            << (config ? "added successfully: " : "failed to parse: ") << param;
    if (config) {
      out_configs.push_back(std::move(config));
    }
  }
}

FieldTrialRegisterImpl::FieldTrialRegisterImpl() = default;
FieldTrialRegisterImpl::~FieldTrialRegisterImpl() = default;

void FieldTrialRegisterImpl::RegisterFieldTrial(base::StringPiece trial_name,
                                                base::StringPiece group_name) {
  // The register method is called early in startup once the platform is
  // initialized. So, in most cases the client will register the field trial
  // before uploading the first UMA log of the current session. We do not want
  // to annotate logs from the previous session. (These comes in two types:
  // histograms persisted from the previous session or stability information
  // about the previous session.) Groups are not stable across sessions; we
  // don't know if the current segmentation applies to the previous session.
  // Incidentally, the platform records metrics to track the movement between
  // groups.
  // TODO(ssid): Move to a MetricsProvider approach to fill the groups so we are
  // able to track how often we miss the first session log due to delays in
  // platform initialization.
  ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
      trial_name, group_name,
      variations::SyntheticTrialAnnotationMode::kCurrentLog);
}

void FieldTrialRegisterImpl::RegisterSubsegmentFieldTrialIfNeeded(
    base::StringPiece trial_name,
    SegmentId segment_id,
    int subsegment_rank) {
  absl::optional<std::string> group_name;
  // TODO(ssid): Make GetSubsegmentName as a ModelProvider API so that clients
  // can simply implement it instead of adding conditions here, once the
  // subsegment process is more stable.
  if (segment_id == SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_FEED_USER) {
    group_name = FeedUserSegment::GetSubsegmentName(subsegment_rank);
  }
#if BUILDFLAG(IS_ANDROID)
  if (segment_id == SegmentId::POWER_USER_SEGMENT) {
    group_name = PowerUserSegment::GetSubsegmentName(subsegment_rank);
  }
#endif
  if (segment_id == SegmentId::CROSS_DEVICE_USER_SEGMENT) {
    group_name = CrossDeviceUserSegment::GetSubsegmentName(subsegment_rank);
  }
  if (segment_id == SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SEARCH_USER) {
    group_name = SearchUserModel::GetSubsegmentName(subsegment_rank);
  }

  if (!group_name) {
    return;
  }
  RegisterFieldTrial(trial_name, *group_name);
}

}  // namespace segmentation_platform
