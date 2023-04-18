// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_INACTIVE_TABS_FEATURES_H_
#define IOS_CHROME_BROWSER_TABS_INACTIVE_TABS_FEATURES_H_

#import "base/feature_list.h"

namespace base {
class TimeDelta;
}  // namespace base

// Feature flag that sets the tab inactivity threshold.
BASE_DECLARE_FEATURE(kTabInactivityThreshold);

// Feature parameters for Inactive Tabs. If no parameter is set, the default
// (two weeks) threshold will be used.
extern const char kTabInactivityThresholdParameterName[];
extern const char kTabInactivityThresholdOneWeekParam[];
extern const char kTabInactivityThresholdTwoWeeksParam[];
extern const char kTabInactivityThresholdThreeWeeksParam[];

// Convenience method for determining if Inactive Tabs is enabled.
bool IsInactiveTabsEnabled();

// Convenience method for determining the tab inactivity threshold.
// The default is 15 days.
// Note: Even though a week is 7 days, the threshold value is returned with one
// extra day in all cases as it matches the user expectations in the following
// case:
//
//     The user opens a tab every Monday. Last Monday it was opened at 10:05am.
//     The tab should not immediately be considered inactive at 10:06am today.
//
// The padding is here to encompass a flexbility of a day.
base::TimeDelta TabInactivityThreshold();

// Convenience method for getting a displayable representation of the thrsshold.
// This is the number of days as a string.
// The default is "14".
std::u16string TabInactivityThresholdDisplayString();

// Feature flag to enable the display of the count of Inactive Tabs in Tab Grid.
BASE_DECLARE_FEATURE(kShowInactiveTabsCount);

// Whether the count of Inactive Tabs should be shown.
bool IsShowInactiveTabsCountEnabled();

#endif  // IOS_CHROME_BROWSER_TABS_INACTIVE_TABS_FEATURES_H_
