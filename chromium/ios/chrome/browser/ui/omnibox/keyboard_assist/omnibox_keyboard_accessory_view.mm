// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/keyboard_assist/omnibox_keyboard_accessory_view.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/flags/system_flags.h"
#import "ios/chrome/browser/search_engines/search_engine_observer_bridge.h"
#import "ios/chrome/browser/search_engines/search_engines_util.h"
#import "ios/chrome/browser/ui/omnibox/keyboard_assist/omnibox_assistive_keyboard_views.h"
#import "ios/chrome/browser/ui/omnibox/keyboard_assist/omnibox_assistive_keyboard_views_utils.h"
#import "ios/chrome/browser/ui/ui_feature_flags.h"
#import "ios/chrome/browser/ui/util/rtl_geometry.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/public/provider/chrome/browser/lens/lens_api.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface OmniboxKeyboardAccessoryView () <SearchEngineObserving>

@property(nonatomic, retain) NSArray<NSString*>* buttonTitles;
@property(nonatomic, weak) id<OmniboxAssistiveKeyboardDelegate> delegate;
@property(nonatomic, weak) id<UIPasteConfigurationSupporting> pasteTarget;

// The shortcut stack view that is displayed by this view.
@property(nonatomic, weak) UIStackView* shortcutStackView;

// The search stack view that is displayed by this view.
@property(nonatomic, weak) UIStackView* searchStackView;

// Called when a keyboard shortcut button is pressed.
- (void)keyboardButtonPressed:(NSString*)title;
// Creates a button shortcut for `title`.
- (UIView*)shortcutButtonWithTitle:(NSString*)title;

@end

@implementation OmniboxKeyboardAccessoryView {
  std::unique_ptr<SearchEngineObserverBridge> _searchEngineObserver;
}

@synthesize buttonTitles = _buttonTitles;
@synthesize delegate = _delegate;

- (instancetype)initWithButtons:(NSArray<NSString*>*)buttonTitles
                       delegate:(id<OmniboxAssistiveKeyboardDelegate>)delegate
                    pasteTarget:(id<UIPasteConfigurationSupporting>)pasteTarget
             templateURLService:(TemplateURLService*)templateURLService {
  self = [super initWithFrame:CGRectZero
               inputViewStyle:UIInputViewStyleKeyboard];
  if (self) {
    _buttonTitles = buttonTitles;
    _delegate = delegate;
    _pasteTarget = pasteTarget;
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.allowsSelfSizing = YES;
    self.templateURLService = templateURLService;
    [self addSubviews];
  }
  return self;
}

- (void)addSubviews {
  if (!self.subviews.count)
    return;

  // Remove any existing stack views from this view.
  [self.shortcutStackView removeFromSuperview];
  [self.searchStackView removeFromSuperview];

  const CGFloat kButtonMinWidth = 36.0;
  const CGFloat kButtonHeight = 36.0;
  const CGFloat kBetweenShortcutButtonSpacing = 5.0;
  const CGFloat kBetweenSearchButtonSpacing = 12.0;
  const CGFloat kHorizontalMargin = 10.0;
  const CGFloat kVerticalMargin = 4.0;

  // Create and add stackview filled with the shortcut buttons.
  UIStackView* shortcutStackView = [[UIStackView alloc] init];
  shortcutStackView.translatesAutoresizingMaskIntoConstraints = NO;
  shortcutStackView.spacing = kBetweenShortcutButtonSpacing;
  shortcutStackView.alignment = UIStackViewAlignmentCenter;
  for (NSString* title in self.buttonTitles) {
    UIView* button = [self shortcutButtonWithTitle:title];
    [button setTranslatesAutoresizingMaskIntoConstraints:NO];
    [button.widthAnchor constraintGreaterThanOrEqualToConstant:kButtonMinWidth]
        .active = YES;
    [button.heightAnchor constraintEqualToConstant:kButtonHeight].active = YES;
    [shortcutStackView addArrangedSubview:button];
  }
  [self addSubview:shortcutStackView];
  self.shortcutStackView = shortcutStackView;

  // Create and add a stackview containing the leading assistive buttons, i.e.
  // Voice search, camera/Lens search and paste search.
  BOOL useLens = ios::provider::IsLensSupported() &&
                 base::FeatureList::IsEnabled(kEnableLensInKeyboard) &&
                 [self isGoogleSearchEngine:self.templateURLService];
  NSArray<UIControl*>* leadingControls =
      OmniboxAssistiveKeyboardLeadingControls(_delegate, self.pasteTarget,
                                              useLens);
  UIStackView* searchStackView = [[UIStackView alloc] init];
  searchStackView.translatesAutoresizingMaskIntoConstraints = NO;
  searchStackView.spacing = kBetweenSearchButtonSpacing;
  for (UIControl* button in leadingControls) {
    [searchStackView addArrangedSubview:button];
  }
  [self addSubview:searchStackView];
  self.searchStackView = searchStackView;

  // Position the stack views.
  id<LayoutGuideProvider> layoutGuide = self.safeAreaLayoutGuide;
  [NSLayoutConstraint activateConstraints:@[
    [searchStackView.leadingAnchor
        constraintEqualToAnchor:layoutGuide.leadingAnchor
                       constant:kHorizontalMargin],
    [shortcutStackView.trailingAnchor
        constraintEqualToAnchor:layoutGuide.trailingAnchor
                       constant:-kHorizontalMargin],
    [searchStackView.trailingAnchor
        constraintLessThanOrEqualToAnchor:shortcutStackView.leadingAnchor],
    [searchStackView.topAnchor constraintEqualToAnchor:layoutGuide.topAnchor
                                              constant:kVerticalMargin],
    [searchStackView.bottomAnchor
        constraintEqualToAnchor:layoutGuide.bottomAnchor
                       constant:-kVerticalMargin],
    [shortcutStackView.topAnchor
        constraintEqualToAnchor:searchStackView.topAnchor],
    [shortcutStackView.bottomAnchor
        constraintEqualToAnchor:searchStackView.bottomAnchor],
  ]];
}

- (UIView*)shortcutButtonWithTitle:(NSString*)title {
  const CGFloat kHorizontalEdgeInset = 8;
  const CGFloat kButtonTitleFontSize = 16.0;
  UIColor* kTitleColorStateNormal = [UIColor colorWithWhite:0.0 alpha:1.0];
  UIColor* kTitleColorStateHighlighted = [UIColor colorWithWhite:0.0 alpha:0.3];

  UIButton* button = [UIButton buttonWithType:UIButtonTypeCustom];
  [button setTitleColor:kTitleColorStateNormal forState:UIControlStateNormal];
  [button setTitleColor:kTitleColorStateHighlighted
               forState:UIControlStateHighlighted];

  [button setTitle:title forState:UIControlStateNormal];
  [button setTitleColor:[UIColor colorNamed:kTextPrimaryColor]
               forState:UIControlStateNormal];
  // TODO(crbug.com/1418068): Remove after minimum version required is >=
  // iOS 15.
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15_0
  button.configuration.contentInsets = NSDirectionalEdgeInsetsMake(
      0, kHorizontalEdgeInset, 0, kHorizontalEdgeInset);
#else
  button.contentEdgeInsets =
      UIEdgeInsetsMake(0, kHorizontalEdgeInset, 0, kHorizontalEdgeInset);
#endif  // __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15_0
  button.clipsToBounds = YES;
  [button.titleLabel setFont:[UIFont systemFontOfSize:kButtonTitleFontSize
                                               weight:UIFontWeightMedium]];

  [button addTarget:self
                action:@selector(keyboardButtonPressed:)
      forControlEvents:UIControlEventTouchUpInside];
  button.isAccessibilityElement = YES;
  [button setAccessibilityLabel:title];
  return button;
}

- (BOOL)enableInputClicksWhenVisible {
  return YES;
}

- (void)keyboardButtonPressed:(id)sender {
  UIButton* button = base::mac::ObjCCastStrict<UIButton>(sender);
  [[UIDevice currentDevice] playInputClick];
  [_delegate keyPressed:[button currentTitle]];
}

#pragma mark - Setters

- (void)setTemplateURLService:(TemplateURLService*)templateURLService {
  _templateURLService = templateURLService;
  if (_templateURLService) {
    _searchEngineObserver =
        std::make_unique<SearchEngineObserverBridge>(self, templateURLService);
  } else {
    _searchEngineObserver.reset();
  }
}

#pragma mark - SearchEngineObserving

- (void)searchEngineChanged {
  // Regenerate the shortcut buttons depending on the new search engine.
  [self addSubviews];
}

#pragma mark - Private

- (BOOL)isGoogleSearchEngine:(TemplateURLService*)service {
  DCHECK(service);
  const TemplateURL* defaultURL = service->GetDefaultSearchProvider();
  return defaultURL &&
         defaultURL->GetEngineType(service->search_terms_data()) ==
             SEARCH_ENGINE_GOOGLE;
}

@end
