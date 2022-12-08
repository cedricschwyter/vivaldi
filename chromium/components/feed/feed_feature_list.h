// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEED_FEED_FEATURE_LIST_H_
#define COMPONENTS_FEED_FEED_FEATURE_LIST_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/signin/public/base/consent_level.h"

// TODO(crbug.com/1165828): Clean up feedv1 features.

namespace feed {

BASE_DECLARE_FEATURE(kInterestFeedContentSuggestions);
BASE_DECLARE_FEATURE(kInterestFeedV2);
BASE_DECLARE_FEATURE(kInterestFeedV2Autoplay);
BASE_DECLARE_FEATURE(kInterestFeedV2Hearts);
BASE_DECLARE_FEATURE(kInterestFeedV2Scrolling);

extern const base::FeatureParam<std::string> kDisableTriggerTypes;
extern const base::FeatureParam<int> kSuppressRefreshDurationMinutes;
extern const base::FeatureParam<int> kTimeoutDurationSeconds;
extern const base::FeatureParam<bool> kThrottleBackgroundFetches;
extern const base::FeatureParam<bool> kOnlySetLastRefreshAttemptOnSuccess;

// TODO(b/213622639): The following two features are obsolete and should be
// removed.
// Determines whether conditions should be reached before enabling the upload of
// click and view actions in the feed (e.g., the user needs to view X cards).
// For example, this is needed when the notice card is at the second position in
// the feed.
BASE_DECLARE_FEATURE(kInterestFeedV1ClicksAndViewsConditionalUpload);
BASE_DECLARE_FEATURE(kInterestFeedV2ClicksAndViewsConditionalUpload);

// Feature that allows the client to automatically dismiss the notice card based
// on the clicks and views on the notice card.
#if BUILDFLAG(IS_IOS)
BASE_DECLARE_FEATURE(kInterestFeedNoticeCardAutoDismiss);
#endif

// Feature that allows users to keep up with and consume web content.
BASE_DECLARE_FEATURE(kWebFeed);

// Use the new DiscoFeed endpoint.
BASE_DECLARE_FEATURE(kDiscoFeedEndpoint);

// Feature that enables xsurface to provide the metrics reporting state to an
// xsurface feed.
BASE_DECLARE_FEATURE(kXsurfaceMetricsReporting);

// Whether to log reliability events.
BASE_DECLARE_FEATURE(kReliabilityLogging);

// Feature that enables refreshing feeds triggered by the users.
BASE_DECLARE_FEATURE(kFeedInteractiveRefresh);

// Feature that shows placeholder cards instead of a loading spinner at first
// load.
BASE_DECLARE_FEATURE(kFeedLoadingPlaceholder);

// Param allowing animations to be disabled when showing the placeholder on
// instant start.
extern const base::FeatureParam<bool>
    kEnableFeedLoadingPlaceholderAnimationOnInstantStart;

// Feature that allows tuning the size of the image memory cache. Value is a
// percentage of the maximum size calculated for the device.
BASE_DECLARE_FEATURE(kFeedImageMemoryCacheSizePercentage);

// Feature that enables clearing the image memory cache when the feed is
// destroyed.
BASE_DECLARE_FEATURE(kFeedClearImageMemoryCache);

// Feature that enables showing a callout to help users return to the top of the
// feeds quickly.
BASE_DECLARE_FEATURE(kFeedBackToTop);

// Feature that enables StAMP cards in the feed.
BASE_DECLARE_FEATURE(kFeedStamp);

// Feature that provides the user assistance in discovering the web feed.
BASE_DECLARE_FEATURE(kWebFeedAwareness);

// Feature that provides the user assistance in using the web feed.
BASE_DECLARE_FEATURE(kWebFeedOnboarding);

// Feature that enables sorting by different heuristics in the web feed.
BASE_DECLARE_FEATURE(kWebFeedSort);

// Feature that causes the "open in new tab" menu item to appear on feed items
// on Start Surface.
BASE_DECLARE_FEATURE(kEnableOpenInNewTabFromStartSurfaceFeed);

// Feature that causes the WebUI version of the Feed to be enabled.
BASE_DECLARE_FEATURE(kWebUiFeed);
extern const base::FeatureParam<std::string> kWebUiFeedUrl;
extern const base::FeatureParam<bool> kWebUiDisableContentSecurityPolicy;

std::string GetFeedReferrerUrl();

// Personalize feed for unsigned users.
BASE_DECLARE_FEATURE(kPersonalizeFeedUnsignedUsers);

// Personalize feed for signed in users who haven't enabled sync.
BASE_DECLARE_FEATURE(kPersonalizeFeedNonSyncUsers);

// Returns the consent level needed to request a personalized feed.
signin::ConsentLevel GetConsentLevelNeededForPersonalizedFeed();

// Feature that enables tracking the acknowledgement state for the info cards.
BASE_DECLARE_FEATURE(kInfoCardAcknowledgementTracking);

// Feature that enables the Crow feature.
// Owned by the CwF team but located here until it makes sense to create a crow
// component, since it is being used in the feed component.
BASE_DECLARE_FEATURE(kShareCrowButton);

// Feature that when enabled completely removes all Feeds from chrome.
BASE_DECLARE_FEATURE(kIsAblated);

// When enabled, schedule a background refresh for a feed sometime after the
// last user engagement with that feed.
BASE_DECLARE_FEATURE(kFeedCloseRefresh);
// On each qualifying user engagement, schedule a background refresh this many
// minutes out.
extern const base::FeatureParam<int> kFeedCloseRefreshDelayMinutes;
// If true, schedule the refresh only when the user scrolls or interacts. If
// false, schedule only when the feed surface is opened to content.
extern const base::FeatureParam<bool> kFeedCloseRefreshRequireInteraction;

// When enabled, no view cache is used.
BASE_DECLARE_FEATURE(kFeedNoViewCache);
// When enabled, replace all items.
BASE_DECLARE_FEATURE(kFeedReplaceAll);

// When enabled, play the feed video via inline playback.
BASE_DECLARE_FEATURE(kFeedVideoInlinePlayback);

// When enabled, compute Good Visits locally and log them to a histogram.
BASE_DECLARE_FEATURE(kClientGoodVisits);
// The maximum time between sequential interactions with the feed that are
// considered as a single visit.
extern const base::FeatureParam<base::TimeDelta> kVisitTimeout;
// A feed visit is "good" if the user spends at least this much time in the feed
// and scrolls at least once.
extern const base::FeatureParam<base::TimeDelta> kGoodTimeInFeed;
// A feed visit is "good" if the user spends at least this much time in an
// article.
extern const base::FeatureParam<base::TimeDelta> kLongOpenTime;
// When calculating time spent in feed for good visits, drop periods of
// viewport-stable feed viewing shorter than this.
extern const base::FeatureParam<base::TimeDelta>
    kMinStableContentSliceVisibilityTime;
// When calculating time spent in feed for good visits, cap long periods of
// viewport-stable feed viewing to this time.
extern const base::FeatureParam<base::TimeDelta>
    kMaxStableContentSliceVisibilityTime;
// Minimum slice exposure needed for counting time in feed for good visits.
extern const base::FeatureParam<double> kSliceVisibleExposureThreshold;
// Minimum slice coverage of viewport needed for counting time in feed for good
// visits.
extern const base::FeatureParam<double> kSliceVisibleCoverageThreshold;

// When enabled, allow tagging experiments with only an experiment ID.
BASE_DECLARE_FEATURE(kFeedExperimentIDTagging);

}  // namespace feed

#endif  // COMPONENTS_FEED_FEED_FEATURE_LIST_H_
