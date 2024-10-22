// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/promos_manager/promos_manager_coordinator.h"

#import <Foundation/Foundation.h>
#import <map>

#import "base/check.h"
#import "base/containers/small_map.h"
#import "base/metrics/histogram_functions.h"
#import "base/notreached.h"
#import "base/strings/sys_string_conversions.h"
#import "ios/chrome/app/tests_hook.h"
#import "ios/chrome/browser/application_context/application_context.h"
#import "ios/chrome/browser/credential_provider_promo/features.h"
#import "ios/chrome/browser/flags/system_flags.h"
#import "ios/chrome/browser/main/browser.h"
#import "ios/chrome/browser/promos_manager/promo_config.h"
#import "ios/chrome/browser/promos_manager/promos_manager.h"
#import "ios/chrome/browser/ui/app_store_rating/app_store_rating_display_handler.h"
#import "ios/chrome/browser/ui/app_store_rating/features.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/credential_provider_promo_commands.h"
#import "ios/chrome/browser/ui/commands/promos_manager_commands.h"
#import "ios/chrome/browser/ui/credential_provider_promo/credential_provider_promo_display_handler.h"
#import "ios/chrome/browser/ui/post_restore_signin/features.h"
#import "ios/chrome/browser/ui/post_restore_signin/post_restore_signin_provider.h"
#import "ios/chrome/browser/ui/promos_manager/bannered_promo_view_provider.h"
#import "ios/chrome/browser/ui/promos_manager/promos_manager_mediator.h"
#import "ios/chrome/browser/ui/promos_manager/standard_promo_alert_provider.h"
#import "ios/chrome/browser/ui/promos_manager/standard_promo_display_handler.h"
#import "ios/chrome/browser/ui/promos_manager/standard_promo_view_provider.h"
#import "ios/chrome/browser/ui/whats_new/promo/whats_new_promo_display_handler.h"
#import "ios/chrome/browser/ui/whats_new/whats_new_util.h"
#import "ios/chrome/common/ui/confirmation_alert/confirmation_alert_action_handler.h"
#import "ios/chrome/common/ui/confirmation_alert/confirmation_alert_view_controller.h"
#import "ios/chrome/common/ui/promo_style/promo_style_view_controller.h"
#import "ios/chrome/common/ui/promo_style/promo_style_view_controller_delegate.h"
#import "ios/chrome/grit/ios_strings.h"
#import "third_party/abseil-cpp/absl/types/optional.h"
#import "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PromosManagerCoordinator () <
    ConfirmationAlertActionHandler,
    UIAdaptivePresentationControllerDelegate,
    PromoStyleViewControllerDelegate> {
  // Promos that conform to the StandardPromoDisplayHandler protocol.
  base::small_map<
      std::map<promos_manager::Promo, id<StandardPromoDisplayHandler>>>
      _displayHandlerPromos;

  // Promos that conform to the StandardPromoViewProvider protocol.
  base::small_map<
      std::map<promos_manager::Promo, id<StandardPromoViewProvider>>>
      _viewProviderPromos;

  // Promos that conform to the BanneredPromoViewProvider protocol.
  base::small_map<
      std::map<promos_manager::Promo, id<BanneredPromoViewProvider>>>
      _banneredViewProviderPromos;

  // Promos that conform to the StandardPromoAlertProvider protocol.
  base::small_map<
      std::map<promos_manager::Promo, id<StandardPromoAlertProvider>>>
      _alertProviderPromos;
}

// A mediator that observes when it's a good time to display a promo.
@property(nonatomic, strong) PromosManagerMediator* mediator;

// The current StandardPromoViewProvider, if any.
@property(nonatomic, weak) id<StandardPromoViewProvider> provider;

// The current BanneredPromoViewProvider, if any.
@property(nonatomic, weak) id<BanneredPromoViewProvider> banneredProvider;

// The current ConfirmationAlertViewController, if any.
@property(nonatomic, strong) ConfirmationAlertViewController* viewController;

// The current PromoStyleViewController, if any.
@property(nonatomic, strong) PromoStyleViewController* banneredViewController;

@end

@implementation PromosManagerCoordinator

#pragma mark - Initialization

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser {
  if (self = [super initWithBaseViewController:viewController
                                       browser:browser]) {
    [self registerPromos];

    BOOL promosExist = _displayHandlerPromos.size() > 0 ||
                       _viewProviderPromos.size() > 0 ||
                       _banneredViewProviderPromos.size() > 0 ||
                       _alertProviderPromos.size() > 0;

    if (promosExist) {
      // Don't create PromosManagerMediator unless promos exist that are
      // registered with PromosManagerCoordinator via `registerPromos`.
      _mediator = [[PromosManagerMediator alloc]
          initWithPromosManager:GetApplicationContext()->GetPromosManager()
          promoImpressionLimits:[self promoImpressionLimits]];
    }
  }

  return self;
}

#pragma mark - Public

- (void)start {
  absl::optional<promos_manager::Promo> nextPromoForDisplay =
      [self.mediator nextPromoForDisplay];

  if (nextPromoForDisplay.has_value())
    [self displayPromo:nextPromoForDisplay.value()];
}

- (void)stop {
  self.mediator = nil;
  [self dismissViewControllers];
}

- (void)dismissViewControllers {
  if (self.viewController) {
    [self.viewController.presentingViewController
        dismissViewControllerAnimated:YES
                           completion:nil];
    self.viewController = nil;
  }

  if (self.banneredViewController) {
    [self.banneredViewController.presentingViewController
        dismissViewControllerAnimated:YES
                           completion:nil];
    self.banneredViewController = nil;
  }
}

- (void)displayPromo:(promos_manager::Promo)promo {
  if (tests_hook::DisablePromoManagerFullScreenPromos()) {
    return;
  }

  auto handler_it = _displayHandlerPromos.find(promo);
  auto provider_it = _viewProviderPromos.find(promo);
  auto bannered_provider_it = _banneredViewProviderPromos.find(promo);
  auto alert_provider_it = _alertProviderPromos.find(promo);

  id<PromosManagerCommands> promosManagerCommandsHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), PromosManagerCommands);

  if (handler_it != _displayHandlerPromos.end()) {
    id<StandardPromoDisplayHandler> handler = handler_it->second;

    if ([handler respondsToSelector:@selector(setHandler:)])
      handler.handler = promosManagerCommandsHandler;

    [handler handleDisplay];

    [self.mediator recordImpression:handler.config.identifier];

    base::UmaHistogramEnumeration("IOS.PromosManager.Promo", promo);
    base::UmaHistogramEnumeration("IOS.PromosManager.Promo.Type",
                                  promos_manager::IOSPromosManagerPromoType::
                                      kStandardPromoDisplayHandler);

    if ([handler respondsToSelector:@selector(promoWasDisplayed)]) {
      [handler promoWasDisplayed];
    }
  } else if (provider_it != _viewProviderPromos.end()) {
    id<StandardPromoViewProvider> provider = provider_it->second;

    if ([provider respondsToSelector:@selector(setHandler:)])
      provider.handler = promosManagerCommandsHandler;

    self.viewController = [provider viewController];
    self.viewController.presentationController.delegate = self;
    self.viewController.actionHandler = self;

    self.provider = provider;

    [self.baseViewController presentViewController:self.viewController
                                          animated:YES
                                        completion:nil];

    [self.mediator recordImpression:provider.config.identifier];

    base::UmaHistogramEnumeration("IOS.PromosManager.Promo", promo);
    base::UmaHistogramEnumeration(
        "IOS.PromosManager.Promo.Type",
        promos_manager::IOSPromosManagerPromoType::kStandardPromoViewProvider);

    if ([provider respondsToSelector:@selector(promoWasDisplayed)]) {
      [provider promoWasDisplayed];
    }
  } else if (bannered_provider_it != _banneredViewProviderPromos.end()) {
    id<BanneredPromoViewProvider> banneredProvider =
        bannered_provider_it->second;

    if ([banneredProvider respondsToSelector:@selector(setHandler:)])
      banneredProvider.handler = promosManagerCommandsHandler;

    self.banneredViewController = [banneredProvider viewController];

    self.banneredViewController.presentationController.delegate = self;
    self.banneredViewController.delegate = self;
    self.banneredProvider = banneredProvider;

    [self.baseViewController presentViewController:self.banneredViewController
                                          animated:YES
                                        completion:nil];

    [self.mediator recordImpression:banneredProvider.config.identifier];

    base::UmaHistogramEnumeration("IOS.PromosManager.Promo", promo);
    base::UmaHistogramEnumeration(
        "IOS.PromosManager.Promo.Type",
        promos_manager::IOSPromosManagerPromoType::kBanneredPromoViewProvider);

    if ([banneredProvider respondsToSelector:@selector(promoWasDisplayed)]) {
      [banneredProvider promoWasDisplayed];
    }
  } else if (alert_provider_it != _alertProviderPromos.end()) {
    id<StandardPromoAlertProvider> alertProvider = alert_provider_it->second;

    if ([alertProvider respondsToSelector:@selector(setHandler:)])
      alertProvider.handler = promosManagerCommandsHandler;

    DCHECK([alertProvider.title length] != 0);
    DCHECK([alertProvider.message length] != 0);
    // The "Default Action" should always be implemented by feature
    DCHECK([alertProvider
        respondsToSelector:@selector(standardPromoAlertDefaultAction)]);

    UIAlertController* alert = [UIAlertController
        alertControllerWithTitle:alertProvider.title
                         message:alertProvider.message
                  preferredStyle:UIAlertControllerStyleAlert];

    NSString* defaultActionButtonText =
        [alertProvider respondsToSelector:@selector(defaultActionButtonText)]
            ? alertProvider.defaultActionButtonText
            : l10n_util::GetNSString(
                  IDS_IOS_PROMOS_MANAGER_ALERT_PROMO_DEFAULT_PRIMARY_BUTTON_TEXT);
    NSString* cancelActionButtonText =
        [alertProvider respondsToSelector:@selector(cancelActionButtonText)]
            ? alertProvider.cancelActionButtonText
            : l10n_util::GetNSString(
                  IDS_IOS_PROMOS_MANAGER_ALERT_PROMO_DEFAULT_CANCEL_BUTTON_TEXT);

    UIAlertAction* defaultAction = [UIAlertAction
        actionWithTitle:defaultActionButtonText
                  style:UIAlertActionStyleDefault
                handler:^(UIAlertAction* action) {
                  if ([alertProvider respondsToSelector:@selector
                                     (standardPromoAlertDefaultAction)])
                    [alertProvider standardPromoAlertDefaultAction];
                }];

    UIAlertAction* cancelAction = [UIAlertAction
        actionWithTitle:cancelActionButtonText
                  style:UIAlertActionStyleCancel
                handler:^(UIAlertAction* action) {
                  if ([alertProvider respondsToSelector:@selector
                                     (standardPromoAlertCancelAction)]) {
                    [alertProvider standardPromoAlertCancelAction];
                    [self dismissViewControllers];
                  }
                }];

    [alert addAction:defaultAction];
    [alert addAction:cancelAction];
    alert.preferredAction = defaultAction;

    [self.baseViewController presentViewController:alert
                                          animated:YES
                                        completion:nil];

    [self.mediator recordImpression:alertProvider.config.identifier];

    base::UmaHistogramEnumeration("IOS.PromosManager.Promo", promo);
    base::UmaHistogramEnumeration(
        "IOS.PromosManager.Promo.Type",
        promos_manager::IOSPromosManagerPromoType::kStandardPromoAlertProvider);

    if ([alertProvider respondsToSelector:@selector(promoWasDisplayed)]) {
      [alertProvider promoWasDisplayed];
    }
  } else {
    // Deregister the promo in edge cases:
    //
    // 1. When promos are forced for display (via Experimental Settings toggle)
    // but not properly enabled (via chrome://flags).
    //
    // 2. When the promo's flag is disabled but was registered before and hasn't
    // been displayed yet.
    //
    // These are niche edge cases that almost exclusively occur during local,
    // manual testing.
    absl::optional<promos_manager::Promo> maybeForcedPromo =
        promos_manager::PromoForName(base::SysNSStringToUTF8(
            experimental_flags::GetForcedPromoToDisplay()));

    if (maybeForcedPromo.has_value()) {
      promos_manager::Promo forcedPromo = maybeForcedPromo.value();

      if ([self isPromoUnregistered:forcedPromo]) {
        base::UmaHistogramEnumeration(
            "IOS.PromosManager.Promo.ForcedDisplayFailure", forcedPromo);
      }
    } else {
      base::UmaHistogramEnumeration("IOS.PromosManager.Promo.DisplayFailure",
                                    promo);

      [self.mediator deregisterPromo:promo];
    }
  }
}

#pragma mark - PromoStyleViewControllerDelegate

// Invoked when the primary action button is tapped.
- (void)didTapPrimaryActionButton {
  DCHECK(self.banneredProvider);

  if (![self.banneredProvider
          respondsToSelector:@selector(standardPromoPrimaryAction)])
    return;

  [self.banneredProvider standardPromoPrimaryAction];
}

// Invoked when the secondary action button is tapped.
- (void)didTapSecondaryActionButton {
  DCHECK(self.banneredProvider);

  // Sometimes the secondary action button for a PromoStyleViewController is
  // used as the dismiss action button.
  if ([self.banneredProvider
          respondsToSelector:@selector(standardPromoDismissAction)]) {
    [self.banneredProvider standardPromoDismissAction];
    [self dismissViewControllers];
  } else if ([self.banneredProvider
                 respondsToSelector:@selector(standardPromoSecondaryAction)]) {
    [self.banneredProvider standardPromoSecondaryAction];
  }
}

// Invoked when the tertiary action button is tapped.
- (void)didTapTertiaryActionButton {
  DCHECK(self.banneredProvider);

  if (![self.banneredProvider
          respondsToSelector:@selector(standardPromoTertiaryAction)])
    return;

  [self.banneredProvider standardPromoTertiaryAction];
}

// Invoked when the top left question mark button is tapped.
- (void)didTapLearnMoreButton {
  DCHECK(self.banneredProvider);

  if (![self.banneredProvider
          respondsToSelector:@selector(standardPromoLearnMoreAction)])
    return;

  [self.banneredProvider standardPromoLearnMoreAction];
}

// Invoked when a link in the disclaimer is tapped.
- (void)didTapURLInDisclaimer:(NSURL*)URL {
  // TODO(crbug.com/1363906): Complete `didTapURLInDisclaimer` to bring users to
  // Settings page.
}

#pragma mark - ConfirmationAlertActionHandler

- (void)confirmationAlertPrimaryAction {
  DCHECK(self.provider);

  if (![self.provider respondsToSelector:@selector(standardPromoPrimaryAction)])
    return;

  [self.provider standardPromoPrimaryAction];
}

- (void)confirmationAlertSecondaryAction {
  DCHECK(self.provider);

  if (![self.provider
          respondsToSelector:@selector(standardPromoSecondaryAction)])
    return;

  [self.provider standardPromoSecondaryAction];
}

- (void)confirmationAlertTertiaryAction {
  DCHECK(self.provider);

  if (![self.provider
          respondsToSelector:@selector(standardPromoTertiaryAction)])
    return;

  [self.provider standardPromoTertiaryAction];
}

- (void)confirmationAlertLearnMoreAction {
  DCHECK(self.provider);

  if (![self.provider
          respondsToSelector:@selector(standardPromoLearnMoreAction)])
    return;

  [self.provider standardPromoLearnMoreAction];
}

- (void)confirmationAlertDismissAction {
  DCHECK(self.provider || self.banneredProvider);

  if ([self.provider
          respondsToSelector:@selector(standardPromoDismissAction)]) {
    [self.provider standardPromoDismissAction];
    [self dismissViewControllers];
  } else if ([self.banneredProvider
                 respondsToSelector:@selector(standardPromoDismissAction)]) {
    [self.banneredProvider standardPromoDismissAction];
    [self dismissViewControllers];
  }
}

#pragma mark - UIAdaptivePresentationControllerDelegate

- (void)presentationControllerDidDismiss:
    (UIPresentationController*)presentationController {
  DCHECK(self.provider || self.banneredProvider);

  if ([self.provider respondsToSelector:@selector(standardPromoDismissSwipe)]) {
    [self.provider standardPromoDismissSwipe];
    [self dismissViewControllers];
  } else if ([self.banneredProvider
                 respondsToSelector:@selector(standardPromoDismissSwipe)]) {
    [self.banneredProvider standardPromoDismissSwipe];
    [self dismissViewControllers];
  } else {
    [self confirmationAlertDismissAction];
  }
}

#pragma mark - Private

- (void)registerPromos {
  // Add StandardPromoDisplayHandler promos here. For example:
  if (IsAppStoreRatingEnabled()) {
    _displayHandlerPromos[promos_manager::Promo::AppStoreRating] =
        [[AppStoreRatingDisplayHandler alloc] init];
  }

  // Add StandardPromoViewProvider promos here. For example:
  // TODO(crbug.com/1360880): Create first StandardPromoViewProvider promo.

  // BanneredPromoViewProvider promo(s) below:
  if (post_restore_signin::features::CurrentPostRestoreSignInType() ==
      post_restore_signin::features::PostRestoreSignInType::kFullscreen)
    _banneredViewProviderPromos
        [promos_manager::Promo::PostRestoreSignInFullscreen] =
            [[PostRestoreSignInProvider alloc] init];

  // StandardPromoAlertProvider promo(s) below:
  if (post_restore_signin::features::CurrentPostRestoreSignInType() ==
      post_restore_signin::features::PostRestoreSignInType::kAlert)
    _alertProviderPromos[promos_manager::Promo::PostRestoreSignInAlert] =
        [[PostRestoreSignInProvider alloc] init];

  // WhatsNewPromoHandler promo below:
  if (IsWhatsNewEnabled()) {
    _displayHandlerPromos[promos_manager::Promo::WhatsNew] =
        [[WhatsNewPromoDisplayHandler alloc] init];
  }

  // CredentialProvider Promo handler
  if (IsCredentialProviderExtensionPromoEnabled()) {
    id<CredentialProviderPromoCommands> handler = HandlerForProtocol(
        self.browser->GetCommandDispatcher(), CredentialProviderPromoCommands);
    _displayHandlerPromos[promos_manager::Promo::CredentialProviderExtension] =
        [[CredentialProviderPromoDisplayHandler alloc] initWithHandler:handler];
  }
}

- (PromoConfigsSet)promoImpressionLimits {
  PromoConfigsSet result;

  for (auto const& [promo, handler] : _displayHandlerPromos)
    result.emplace(handler.config);

  for (auto const& [promo, provider] : _viewProviderPromos)
    result.emplace(provider.config);

  for (auto const& [promo, banneredProvider] : _banneredViewProviderPromos)
    result.emplace(banneredProvider.config);

  for (auto const& [promo, alertProvider] : _alertProviderPromos)
    result.emplace(alertProvider.config);

  return result;
}

// Checks if `promo` is properly registered within this coordinator.
- (BOOL)isPromoUnregistered:(promos_manager::Promo)promo {
  auto handler_it = _displayHandlerPromos.find(promo);
  auto provider_it = _viewProviderPromos.find(promo);
  auto bannered_provider_it = _banneredViewProviderPromos.find(promo);
  auto alert_provider_it = _alertProviderPromos.find(promo);

  return handler_it == _displayHandlerPromos.end() &&
         provider_it == _viewProviderPromos.end() &&
         bannered_provider_it == _banneredViewProviderPromos.end() &&
         alert_provider_it == _alertProviderPromos.end();
}

@end
