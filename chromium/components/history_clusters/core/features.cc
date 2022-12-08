// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history_clusters/core/features.h"

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_piece_forward.h"
#include "base/strings/string_split.h"
#include "build/build_config.h"
#include "ui/base/l10n/l10n_util.h"

namespace history_clusters {

namespace {

constexpr auto enabled_by_default_desktop_only =
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
    base::FEATURE_DISABLED_BY_DEFAULT;
#else
    base::FEATURE_ENABLED_BY_DEFAULT;
#endif

}  // namespace

namespace internal {

BASE_FEATURE(kJourneys, "Journeys", enabled_by_default_desktop_only);

BASE_FEATURE(kJourneysLabels,
             "JourneysLabel",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kPersistedClusters,
             "HistoryClustersPersistedClusters",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kOmniboxAction,
             "JourneysOmniboxAction",
             enabled_by_default_desktop_only);

BASE_FEATURE(kOmniboxHistoryClusterProvider,
             "JourneysOmniboxHistoryClusterProvider",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNonUserVisibleDebug,
             "JourneysNonUserVisibleDebug",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kUserVisibleDebug,
             "JourneysUserVisibleDebug",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kPersistContextAnnotationsInHistoryDb,
             "JourneysPersistContextAnnotationsInHistoryDb",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kHistoryClustersInternalsPage,
             "HistoryClustersInternalsPage",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kHistoryClustersUseContinueOnShutdown,
             "HistoryClustersUseContinueOnShutdown",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kHistoryClustersKeywordFiltering,
             "HistoryClustersKeywordFiltering",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace internal

BASE_FEATURE(kJourneysSurveyForHistoryEntrypoint,
             "JourneysSurveyForHistoryEntrypoint",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<base::TimeDelta>
    kJourneysSurveyForHistoryEntrypointDelay{
        &kJourneysSurveyForHistoryEntrypoint, "survey-delay-duration",
        base::Seconds(6)};

BASE_FEATURE(kJourneysSurveyForOmniboxEntrypoint,
             "JourneysSurveyForOmniboxEntrypoint",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<base::TimeDelta>
    kJourneysSurveyForOmniboxEntrypointDelay{
        &kJourneysSurveyForOmniboxEntrypoint, "survey-delay-duration",
        base::Seconds(6)};

}  // namespace history_clusters
