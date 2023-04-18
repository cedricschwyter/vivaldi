// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/default_promo/default_browser_string_util.h"

#import "ios/chrome/browser/ui/default_promo/default_browser_utils.h"
#import "ios/chrome/grit/ios_google_chrome_strings.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ui/base/l10n/l10n_util_mac.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import <UIKit/UIKit.h>

using l10n_util::GetNSString;

NSString* GetDefaultBrowserPromoTitle() {

  if (IsVivaldiRunning()) {
    return GetNSString(IDS_VIVALDI_IOS_DEFAULT_BROWSER_NON_MODAL_TITLE);
  } // End Vivaldi

    return GetNSString(IDS_IOS_DEFAULT_BROWSER_TITLE_CTA_EXPERIMENT_SWITCH);
}

NSString* GetDefaultBrowserLearnMoreText() {
  if (IsInModifiedStringsGroup()) {
    return GetNSString(IDS_IOS_DEFAULT_BROWSER_LEARN_MORE_INSTRUCTIONS_MESSAGE);
  }
    return GetNSString(
        IDS_IOS_DEFAULT_BROWSER_LEARN_MORE_INSTRUCTIONS_MESSAGE_CTA_EXPERIMENT);
}
