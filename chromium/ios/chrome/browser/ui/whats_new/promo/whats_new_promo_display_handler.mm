// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/whats_new/promo/whats_new_promo_display_handler.h"

#import "base/check.h"
#import "base/metrics/user_metrics.h"
#import "ios/chrome/browser/promos_manager/constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation WhatsNewPromoDisplayHandler

#pragma mark - StandardPromoDisplayHandler

- (void)handleDisplay {
  DCHECK(self.handler);
  [self.handler showWhatsNewPromo];
}

#pragma mark - PromoProtocol

- (promos_manager::Promo)identifier {
  return promos_manager::Promo::WhatsNew;
}

- (void)promoWasDisplayed {
  base::RecordAction(base::UserMetricsAction("WhatsNew.Promo.Displayed"));
}

@end
