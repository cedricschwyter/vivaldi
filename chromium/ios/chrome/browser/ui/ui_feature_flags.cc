// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ui_feature_flags.h"

BASE_FEATURE(kDefaultBrowserBlueDotPromo,
             "DefaultBrowserBlueDotPromo",
             base::FEATURE_DISABLED_BY_DEFAULT);

constexpr base::FeatureParam<BlueDotPromoUserGroup>::Option
    kBlueDotPromoUserGroupOptions[] = {
        {BlueDotPromoUserGroup::kAllDBPromosDisabled, "all-db-promos-disabled"},
        {BlueDotPromoUserGroup::kAllDBPromosEnabled, "all-db-promos-enabled"},
        {BlueDotPromoUserGroup::kOnlyBlueDotPromoEnabled,
         "only-blue-dot-promo-enabled"}};

constexpr base::FeatureParam<BlueDotPromoUserGroup> kBlueDotPromoUserGroupParam{
    &kDefaultBrowserBlueDotPromo, "user-group",
    BlueDotPromoUserGroup::kOnlyBlueDotPromoEnabled,
    &kBlueDotPromoUserGroupOptions};

BASE_FEATURE(kExpandedTabStrip,
             "ExpandedTabStrip",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kTestFeature, "TestFeature", base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSharedHighlightingIOS,
             "SharedHighlightingIOS",
             base::FEATURE_ENABLED_BY_DEFAULT);

// TODO(crbug.com/1128242): Remove this flag after the refactoring work is
// finished.
BASE_FEATURE(kModernTabStrip,
             "ModernTabStrip",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kIncognitoNtpRevamp,
             "IncognitoNtpRevamp",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kIOS3PIntentsInIncognito,
             "IOS3pIntentsInIncognito",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kDefaultBrowserFullscreenPromoExperiment,
             "DefaultBrowserFullscreenPromoExperiment",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kDefaultBrowserIntentsShowSettings,
             "DefaultBrowserIntentsShowSettings",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kIOSCustomBrowserEditMenu,
             "IOSCustomBrowserEditMenu",
             base::FEATURE_DISABLED_BY_DEFAULT);

const char kIOSEditMenuPartialTranslateNoIncognitoParam[] =
    "IOSEditMenuPartialTranslateNoIncognitoParam";

BASE_FEATURE(kIOSEditMenuPartialTranslate,
             "IOSEditMenuPartialTranslate",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kIOSNewOmniboxImplementation,
             "kIOSNewOmniboxImplementation",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kRemoveCrashInfobar,
             "RemoveCrashInfobar",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kIOSLocationBarUseNativeContextMenu,
             "IOSLocationBarUseNativeContextMenu",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kUseLensToSearchForImage,
             "UseLensToSearchForImage",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kEnableLensInHomeScreenWidget,
             "EnableLensInHomeScreenWidget",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kEnableLensInKeyboard,
             "EnableLensInKeyboard",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kEnableLensInNTP,
             "EnableLensInNTP",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kEnableLensContextMenuAltText,
             "EnableLensContextMenuAltText",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kEnableLensInOmniboxCopiedImage,
             "EnableLensInOmniboxCopiedImage",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kRemoveExcessNTPs,
             "RemoveExcessNTPs",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kEnableShortenedPasswordAutoFillInstruction,
             "EnableShortenedPasswordAutoFillInstruction",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kUseSFSymbols, "UseSFSymbols", base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kUseSFSymbolsInOmnibox,
             "UseSFSymbolsInOmnibox",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kCalendarExperienceKit,
             "CalendarExperienceKit",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kEnableExpKitAppleCalendar,
             "EnableExpKitAppleCalendar",
             base::FEATURE_DISABLED_BY_DEFAULT);

const char kExperienceKitMapsVariationName[] = "ExperienceKitMapsVariant";
extern const char kEnableExperienceKitMapsVariationSrp[] = "with SRP";

BASE_FEATURE(kMapsExperienceKit,
             "MapsExperienceKit",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kEnableMiniMap,
             "EnableMiniMap",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kTabGridRecencySort,
             "TabGridRecencySort",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsTabGridSortedByRecency() {
  return base::FeatureList::IsEnabled(kTabGridRecencySort);
}

BASE_FEATURE(kMultilineFadeTruncatingLabel,
             "MultilineFadeTruncatingLabel",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNotificationSettingsMenuItem,
             "NotificationSettingsMenuItem",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSpotlightReadingListSource,
             "SpotlightReadingListSource",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kConsistencyNewAccountInterface,
             "ConsistencyNewAccountInterface",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsConsistencyNewAccountInterfaceEnabled() {
  return base::FeatureList::IsEnabled(kConsistencyNewAccountInterface);
}

BASE_FEATURE(kAddToHomeScreen,
             "AddToHomeScreen",
             base::FEATURE_DISABLED_BY_DEFAULT);

const char kAddToHomeScreenDisableIncognitoParam[] =
    "AddToHomeScreenDisableIncognitoParam";

bool ShouldAddToHomeScreen(bool in_incognito) {
  if (!base::FeatureList::IsEnabled(kAddToHomeScreen)) {
    return false;
  }
  if (!in_incognito) {
    return true;
  }
  return !base::GetFieldTrialParamByFeatureAsBool(
      kAddToHomeScreen, kAddToHomeScreenDisableIncognitoParam, false);
}

bool ShouldShowPartialTranslateInIncognito() {
  if (!base::FeatureList::IsEnabled(kIOSEditMenuPartialTranslate)) {
    return false;
  }
  return !base::GetFieldTrialParamByFeatureAsBool(
      kIOSEditMenuPartialTranslate,
      kIOSEditMenuPartialTranslateNoIncognitoParam, false);
}

BASE_FEATURE(kNewNTPOmniboxLayout,
             "kNewNTPOmniboxLayout",
             base::FEATURE_DISABLED_BY_DEFAULT);
