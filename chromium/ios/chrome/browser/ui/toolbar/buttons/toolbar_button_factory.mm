// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"

#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/icons/symbols.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_actions_handler.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_visibility_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_new_tab_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tab_grid_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_constants.h"
#import "ios/chrome/browser/ui/util/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/grit/ios_theme_resources.h"
#import "ui/base/l10n/l10n_util.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/ui/ad_tracker_blocker/vivaldi_atb_constants.h"
#import "ios/ui/toolbar/vivaldi_toolbar_constants.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using l10n_util::GetNSString;
using vivaldi::IsVivaldiRunning;
// End Vivaldi

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The size of the symbol image.
const CGFloat kSymbolToolbarPointSize = 24;

}  // namespace

@implementation ToolbarButtonFactory

- (instancetype)initWithStyle:(ToolbarStyle)style {
  self = [super init];
  if (self) {
    _style = style;
    _toolbarConfiguration = [[ToolbarConfiguration alloc] initWithStyle:style];
  }
  return self;
}

#pragma mark - Buttons

- (ToolbarButton*)backButton {
  UIImage* backImage;
  backImage = UseSymbols() ? DefaultSymbolWithPointSize(kBackSymbol,
                                                        kSymbolToolbarPointSize)
                           : [UIImage imageNamed:@"toolbar_back"];
  ToolbarButton* backButton = [ToolbarButton
      toolbarButtonWithImage:[backImage
                                 imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:backButton width:kAdaptiveToolbarButtonWidth];
  backButton.accessibilityLabel = l10n_util::GetNSString(IDS_ACCNAME_BACK);
  [backButton addTarget:self.actionHandler
                 action:@selector(backAction)
       forControlEvents:UIControlEventTouchUpInside];
  backButton.visibilityMask = self.visibilityConfiguration.backButtonVisibility;
  return backButton;
}

// Returns a forward button without visibility mask configured.
- (ToolbarButton*)forwardButton {
  UIImage* forwardImage =
      UseSymbols()
          ? DefaultSymbolWithPointSize(kForwardSymbol, kSymbolToolbarPointSize)
          : [UIImage imageNamed:@"toolbar_forward"];
  ToolbarButton* forwardButton = [ToolbarButton
      toolbarButtonWithImage:[forwardImage
                                 imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:forwardButton width:kAdaptiveToolbarButtonWidth];
  forwardButton.visibilityMask =
      self.visibilityConfiguration.forwardButtonVisibility;
  forwardButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_ACCNAME_FORWARD);
  [forwardButton addTarget:self.actionHandler
                    action:@selector(forwardAction)
          forControlEvents:UIControlEventTouchUpInside];
  return forwardButton;
}

- (ToolbarTabGridButton*)tabGridButton {
  UIImage* tabGridImage =
      UseSymbols() ? CustomSymbolWithPointSize(kSquareNumberSymbol,
                                               kSymbolToolbarPointSize)
                   : tabGridImage = [UIImage imageNamed:@"toolbar_switcher"];
  ToolbarTabGridButton* tabGridButton =
      [ToolbarTabGridButton toolbarButtonWithImage:tabGridImage];
  [self configureButton:tabGridButton width:kAdaptiveToolbarButtonWidth];
  SetA11yLabelAndUiAutomationName(tabGridButton, IDS_IOS_TOOLBAR_SHOW_TABS,
                                  kToolbarStackButtonIdentifier);
  [tabGridButton addTarget:self.actionHandler
                    action:@selector(tabGridTouchDown)
          forControlEvents:UIControlEventTouchDown];
  [tabGridButton addTarget:self.actionHandler
                    action:@selector(tabGridTouchUp)
          forControlEvents:UIControlEventTouchUpInside];
  tabGridButton.visibilityMask =
      self.visibilityConfiguration.tabGridButtonVisibility;
  return tabGridButton;
}

- (ToolbarButton*)toolsMenuButton {
  ToolbarButton* toolsMenuButton;
  if (UseSymbols()) {
    toolsMenuButton = [ToolbarButton
        toolbarButtonWithImage:DefaultSymbolWithPointSize(
                                   kMenuSymbol, kSymbolToolbarPointSize)];
  } else {
    toolsMenuButton = [[ToolbarToolsMenuButton alloc] initWithFrame:CGRectZero];
  }

  if (IsVivaldiRunning()) {
    UIImage* menuImage = [UIImage imageNamed:@"toolbar_menu"];
    toolsMenuButton = [ToolbarToolsMenuButton toolbarButtonWithImage:menuImage];
  } // End Vivaldi

  SetA11yLabelAndUiAutomationName(toolsMenuButton, IDS_IOS_TOOLBAR_SETTINGS,
                                  kToolbarToolsMenuButtonIdentifier);
  [self configureButton:toolsMenuButton width:kAdaptiveToolbarButtonWidth];
  [toolsMenuButton.heightAnchor
      constraintEqualToConstant:kAdaptiveToolbarButtonWidth]
      .active = YES;
  [toolsMenuButton addTarget:self.actionHandler
                      action:@selector(toolsMenuAction)
            forControlEvents:UIControlEventTouchUpInside];
  toolsMenuButton.visibilityMask =
      self.visibilityConfiguration.toolsMenuButtonVisibility;
  return toolsMenuButton;
}

- (ToolbarButton*)shareButton {
  UIImage* shareImage =
      UseSymbols()
          ? DefaultSymbolWithPointSize(kShareSymbol, kSymbolToolbarPointSize)
          : [UIImage imageNamed:@"toolbar_share"];
  ToolbarButton* shareButton =
      [ToolbarButton toolbarButtonWithImage:shareImage];
  [self configureButton:shareButton width:kAdaptiveToolbarButtonWidth];
  SetA11yLabelAndUiAutomationName(shareButton, IDS_IOS_TOOLS_MENU_SHARE,
                                  kToolbarShareButtonIdentifier);
  shareButton.titleLabel.text = @"Share";
  [shareButton addTarget:self.actionHandler
                  action:@selector(shareAction)
        forControlEvents:UIControlEventTouchUpInside];
  shareButton.visibilityMask =
      self.visibilityConfiguration.shareButtonVisibility;
  return shareButton;
}

- (ToolbarButton*)reloadButton {
  UIImage* reloadImage =
      UseSymbols() ? CustomSymbolWithPointSize(kArrowClockWiseSymbol,
                                               kSymbolToolbarPointSize)
                   : [UIImage imageNamed:@"toolbar_reload"];
  ToolbarButton* reloadButton = [ToolbarButton
      toolbarButtonWithImage:[reloadImage
                                 imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:reloadButton width:kAdaptiveToolbarButtonWidth];
  reloadButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_ACCNAME_RELOAD);
  [reloadButton addTarget:self.actionHandler
                   action:@selector(reloadAction)
         forControlEvents:UIControlEventTouchUpInside];
  reloadButton.visibilityMask =
      self.visibilityConfiguration.reloadButtonVisibility;
  return reloadButton;
}

- (ToolbarButton*)stopButton {
  UIImage* stopImage = UseSymbols() ? DefaultSymbolWithPointSize(
                                          kXMarkSymbol, kSymbolToolbarPointSize)
                                    : [UIImage imageNamed:@"toolbar_stop"];
  ToolbarButton* stopButton = [ToolbarButton toolbarButtonWithImage:stopImage];
  [self configureButton:stopButton width:kAdaptiveToolbarButtonWidth];
  stopButton.accessibilityLabel = l10n_util::GetNSString(IDS_IOS_ACCNAME_STOP);
  [stopButton addTarget:self.actionHandler
                 action:@selector(stopAction)
       forControlEvents:UIControlEventTouchUpInside];
  stopButton.visibilityMask = self.visibilityConfiguration.stopButtonVisibility;
  return stopButton;
}

- (ToolbarButton*)openNewTabButton {
  ToolbarButton* newTabButton;
  if (UseSymbols()) {
    if (@available(iOS 15, *)) {
      UIImage* image = SymbolWithPalette(
          CustomSymbolWithPointSize(kNewTabSymbol, kSymbolToolbarPointSize), @[
            [UIColor colorNamed:kGrey600Color],
            [self.toolbarConfiguration
                locationBarBackgroundColorWithVisibility:1]
          ]);
      newTabButton = [ToolbarButton toolbarButtonWithImage:image];
    } else {
      newTabButton = [ToolbarButton
          toolbarButtonWithImage:
              [[UIImage imageNamed:@"plus_circle_fill_ios14"]
                  imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal]];
    }
  } else {
    newTabButton = [ToolbarNewTabButton
        toolbarButtonWithImage:[UIImage imageNamed:@"toolbar_new_tab_page"]];
  }

  [newTabButton addTarget:self.actionHandler
                   action:@selector(newTabAction:)
         forControlEvents:UIControlEventTouchUpInside];
  BOOL isIncognito = self.style == INCOGNITO;

  [self configureButton:newTabButton width:kAdaptiveToolbarButtonWidth];

  newTabButton.accessibilityLabel =
      l10n_util::GetNSString(isIncognito ? IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB
                                         : IDS_IOS_TOOLS_MENU_NEW_TAB);

  newTabButton.accessibilityIdentifier = kToolbarNewTabButtonIdentifier;

  newTabButton.visibilityMask =
      self.visibilityConfiguration.newTabButtonVisibility;
  return newTabButton;
}

- (UIButton*)cancelButton {
  UIButton* cancelButton = [UIButton buttonWithType:UIButtonTypeSystem];
  cancelButton.titleLabel.font = [UIFont systemFontOfSize:kLocationBarFontSize];
  cancelButton.tintColor = [UIColor colorNamed:kBlueColor];
  [cancelButton setTitle:l10n_util::GetNSString(IDS_CANCEL)
                forState:UIControlStateNormal];
  [cancelButton setContentHuggingPriority:UILayoutPriorityRequired
                                  forAxis:UILayoutConstraintAxisHorizontal];
  [cancelButton
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];

  // TODO(crbug.com/1418068): Remove after minimum version required is >=
  // iOS 15.
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15_0
  cancelButton.configuration.contentInsets = NSDirectionalEdgeInsetsMake(
      0, kCancelButtonHorizontalInset, 0, kCancelButtonHorizontalInset);
#else
  cancelButton.contentEdgeInsets = UIEdgeInsetsMake(
      0, kCancelButtonHorizontalInset, 0, kCancelButtonHorizontalInset);
#endif  // __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15_0

  cancelButton.hidden = YES;
  [cancelButton addTarget:self.actionHandler
                   action:@selector(cancelOmniboxFocusAction)
         forControlEvents:UIControlEventTouchUpInside];
  cancelButton.accessibilityIdentifier =
      kToolbarCancelOmniboxEditButtonIdentifier;
  return cancelButton;
}

#pragma mark: - VIVALDI
- (ToolbarButton*)panelButton {
  UIImage* panelImage = [UIImage imageNamed:vToolbarPanelButtonIcon];
  ToolbarButton* panelButton = [ToolbarButton
      toolbarButtonWithImage:[panelImage
                                 imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:panelButton width:kAdaptiveToolbarButtonWidth];
  panelButton.accessibilityLabel = GetNSString(IDS_ACCNAME_PANEL);
  [panelButton addTarget:self.actionHandler
                 action:@selector(panelAction)
       forControlEvents:UIControlEventTouchUpInside];
  panelButton.visibilityMask =
      self.visibilityConfiguration.toolsMenuButtonVisibility;
  return panelButton;
}

// Vivaldi search button -> Visible only on new tab page.
- (ToolbarButton*)vivaldiSearchButton {
  UIImage* searchImage = [UIImage imageNamed:@"toolbar_search"];
  ToolbarButton* searchButton = [ToolbarButton
      toolbarButtonWithImage:[searchImage
                                 imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:searchButton width:kAdaptiveToolbarButtonWidth];
  searchButton.accessibilityLabel = GetNSString(IDS_ACCNAME_SEARCH);
  [searchButton addTarget:self.actionHandler
                 action:@selector(vivaldiSearchAction)
       forControlEvents:UIControlEventTouchUpInside];
  searchButton.visibilityMask =
      self.visibilityConfiguration.backButtonVisibility;
  return searchButton;
}

- (ToolbarButton*)shieldButton {
  UIImage* shieldImage = [UIImage imageNamed:vATBShield];
  ToolbarButton* shieldButton =
    [ToolbarButton
      toolbarButtonWithImage:[shieldImage
                              imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:shieldButton width:kAdaptiveToolbarButtonWidth];
  shieldButton.accessibilityLabel = GetNSString(IDS_ACCNAME_ATB);
  [shieldButton addTarget:self.actionHandler
                   action:@selector(showTrackerBlockerManager)
         forControlEvents:UIControlEventTouchUpInside];
  shieldButton.visibilityMask =
    self.visibilityConfiguration.toolsMenuButtonVisibility;
  return shieldButton;
}

// More button -> Visible only in iPhone landscape mode.
- (ToolbarButton*)vivaldiMoreButton {
  UIImage* moreImage = [UIImage imageNamed:@"toolbar_more"];
  ToolbarButton* moreButton =
    [ToolbarButton
      toolbarButtonWithImage:[moreImage
                              imageFlippedForRightToLeftLayoutDirection]];
  [self configureButton:moreButton width:kAdaptiveToolbarButtonWidth];
  moreButton.accessibilityLabel = GetNSString(IDS_ACCNAME_MORE);
  moreButton.visibilityMask =
    self.visibilityConfiguration.toolsMenuButtonVisibility;
  moreButton.menu = [self contextMenuForMoreWithAllButtons:NO];
  moreButton.showsMenuAsPrimaryAction = YES;
  return moreButton;
}

/// Returns the more button options based on the browsing state.
- (UIMenu*)contextMenuForMoreWithAllButtons:(BOOL)allButtons {

  NSString* atbTitle =
      GetNSString(IDS_IOS_PREFS_VIVALDI_AD_AND_TRACKER_BLOCKER);
  UIAction* atbAction =
      [UIAction actionWithTitle:atbTitle
                          image:[UIImage imageNamed:vATBShield]
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler showTrackerBlockerManager];
      }];
  atbAction.accessibilityLabel = atbTitle;

  NSString* shareTitle = GetNSString(IDS_IOS_TOOLBAR_VIVALDI_SHARE);
  UIAction* shareAction =
      [UIAction actionWithTitle:shareTitle
                          image:[UIImage imageNamed:@"toolbar_share"]
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler shareAction];
      }];
  shareAction.accessibilityLabel = shareTitle;

  NSString* panelTitle = GetNSString(IDS_IOS_TOOLBAR_VIVALDI_PANEL);
  UIAction* panelAction =
      [UIAction actionWithTitle:panelTitle
                          image:[UIImage imageNamed:vToolbarPanelButtonIcon]
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler panelAction];
      }];
  panelAction.accessibilityLabel = panelTitle;

  NSArray* moreActions = [[NSArray alloc] initWithObjects:@[], nil];

  if (allButtons) {
    moreActions = @[atbAction, shareAction, panelAction];
  } else {
    moreActions = @[panelAction];
  }

  UIMenu* menu = [UIMenu menuWithTitle:@"" children:moreActions];
  return menu;
}
// End Vivaldi

#pragma mark - Helpers

// Sets the `button` width to `width` with a priority of
// UILayoutPriorityRequired - 1. If the priority is `UILayoutPriorityRequired`,
// there is a conflict when the buttons are hidden as the stack view is setting
// their width to 0. Setting the priority to UILayoutPriorityDefaultHigh doesn't
// work as they would have a lower priority than other elements.
- (void)configureButton:(ToolbarButton*)button width:(CGFloat)width {
  NSLayoutConstraint* constraint =
      [button.widthAnchor constraintEqualToConstant:width];
  constraint.priority = UILayoutPriorityRequired - 1;
  constraint.active = YES;
  button.toolbarConfiguration = self.toolbarConfiguration;
  button.exclusiveTouch = YES;
  button.pointerInteractionEnabled = YES;
  button.pointerStyleProvider =
      ^UIPointerStyle*(UIButton* uiButton, UIPointerEffect* proposedEffect,
                       UIPointerShape* proposedShape) {
    // This gets rid of a thin border on a spotlighted bookmarks button.
    // This is applied to all toolbar buttons for consistency.
    CGRect rect = CGRectInset(uiButton.frame, 1, 1);
    UIPointerShape* shape = [UIPointerShape shapeWithRoundedRect:rect];
    return [UIPointerStyle styleWithEffect:proposedEffect shape:shape];
  };
}

@end
