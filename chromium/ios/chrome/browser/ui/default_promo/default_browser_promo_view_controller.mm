// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/default_promo/default_browser_promo_view_controller.h"

#import "base/feature_list.h"
#import "ios/chrome/browser/ui/default_promo/default_browser_string_util.h"
#import "ios/chrome/browser/ui/default_promo/default_browser_utils.h"
#import "ios/chrome/grit/ios_google_chrome_strings.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ui/base/l10n/l10n_util_mac.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/ui/promos/vivaldi_promo_constants.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation DefaultBrowserPromoViewController

#pragma mark - Public

- (void)loadView {

  if (IsVivaldiRunning()) {
    self.image = [UIImage imageNamed:vSwitchToVivaldi];
  } else {
  self.image = [UIImage imageNamed:@"default_browser_illustration"];
  } // End Vivaldi

  self.customSpacingAfterImage = 30;

  self.helpButtonAvailable = YES;
  self.helpButtonAccessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_HELP_ACCESSIBILITY_LABEL);

  self.showDismissBarButton = NO;
  self.titleString = GetDefaultBrowserPromoTitle();

  if (IsVivaldiRunning()) {
    self.subtitleString =
        IsInModifiedStringsGroup()
            ? l10n_util::GetNSString(IDS_IOS_DEFAULT_BROWSER_LEARN_MORE_MESSAGE)
            : l10n_util::GetNSString(
                IDS_VIVALDI_IOS_DEFAULT_BROWSER_NON_MODAL_DESCRIPTION);
  } else {
  self.subtitleString =
      IsInModifiedStringsGroup()
          ? l10n_util::GetNSString(IDS_IOS_DEFAULT_BROWSER_LEARN_MORE_MESSAGE)
          : l10n_util::GetNSString(IDS_IOS_DEFAULT_BROWSER_DESCRIPTION);
  } // End Vivaldi

  self.primaryActionString = l10n_util::GetNSString(IDS_IOS_OPEN_SETTINGS);
  if (IsInRemindMeLaterGroup() &&
      !ShouldShowRemindMeLaterDefaultBrowserFullscreenPromo()) {
    // Show the Remind Me Later button if the user is in the correct experiment
    // group and this isn't the second impression.
    self.secondaryActionString = l10n_util::GetNSString(
        IDS_IOS_DEFAULT_BROWSER_REMIND_ME_LATER_BUTTON_TEXT);
    self.tertiaryActionString =
        l10n_util::GetNSString(IDS_IOS_DEFAULT_BROWSER_SECONDARY_BUTTON_TEXT);
  } else {
    self.secondaryActionString =
        l10n_util::GetNSString(IDS_IOS_DEFAULT_BROWSER_SECONDARY_BUTTON_TEXT);
  }
  self.dismissBarButtonSystemItem = UIBarButtonSystemItemCancel;
  [super loadView];
}

@end
