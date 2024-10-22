// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_INFO_PAGE_INFO_FEATURES_H_
#define CHROME_BROWSER_PAGE_INFO_PAGE_INFO_FEATURES_H_

#include "base/feature_list.h"
namespace page_info {

// Returns true if kPageInfoAboutThisSiteMoreInfo and dependent features are
// enabled.
bool IsMoreAboutThisSiteFeatureEnabled();

// Returns true if kPageInfoAboutThisSiteDescriptionPlaceholder and dependent
// features are enabled.
bool IsDescriptionPlaceholderFeatureEnabled();

// Returns true if `kPageInfoAboutThisSiteNewIcon` and dependent features are
// enabled.
bool IsAboutThisSiteNewIconFeatureEnabled();

// Returns true if `kPageInfoAboutThisSiteNonMsbb` and dependent features are
// enabled.
bool IsAboutThisSiteForNonMsbbFeatureEnabled();

#if !BUILDFLAG(IS_ANDROID)
// Returns true if kAboutThisSitePersistentSidePanelEntry and dependent
// features are enabled.
bool IsPersistentSidePanelEntryFeatureEnabled();

// Enables the persistent "About this site" entry in the side panel.
BASE_DECLARE_FEATURE(kAboutThisSitePersistentSidePanelEntry);
#endif

}  // namespace page_info

#endif  // CHROME_BROWSER_PAGE_INFO_PAGE_INFO_FEATURES_H_
