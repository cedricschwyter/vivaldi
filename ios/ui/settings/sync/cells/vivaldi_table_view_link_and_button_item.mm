// Copyright 2023 Vivaldi Technologies. All rights reserved

#import "ios/ui/settings/sync/cells/vivaldi_table_view_link_and_button_item.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/table_view/table_view_cells_constants.h"
#import "ios/chrome/common/ui/util/pointer_interaction_util.h"
#import "ios/chrome/common/ui/util/text_view_util.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Alpha value for the disabled action button.
const CGFloat kDisabledButtonAlpha = 0.5;
// Vertical spacing between stackView and cell contentView.
const CGFloat kStackViewVerticalSpacing = 9.0;
// Horizontal spacing between stackView and cell contentView.
const CGFloat kStackViewHorizontalSpacing = 16.0;
// SubView spacing within stackView.
const CGFloat kStackViewSubViewSpacing = 13.0;
// Horizontal Inset between button contents and edge.
const CGFloat kButtonTitleHorizontalContentInset = 40.0;
// Vertical Inset between button contents and edge.
const CGFloat kButtonTitleVerticalContentInset = 8.0;
// Button corner radius.
const CGFloat kButtonCornerRadius = 8;
// Default Text alignment.
const NSTextAlignment kDefaultTextAlignment = NSTextAlignmentCenter;
}  // namespace

@implementation VivaldiTableViewLinkAndButtonItem
@synthesize buttonAccessibilityIdentifier = _buttonAccessibilityIdentifier;
@synthesize buttonBackgroundColor = _buttonBackgroundColor;
@synthesize buttonText = _buttonText;
@synthesize linkText = _linkText;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [VivaldiTableViewLinkAndButtonCell class];
    _enabled = YES;
    _textAlignment = kDefaultTextAlignment;
  }
  return self;
}

- (void)configureCell:(TableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];
  VivaldiTableViewLinkAndButtonCell* cell =
      base::mac::ObjCCastStrict<VivaldiTableViewLinkAndButtonCell>(tableCell);
  [cell setSelectionStyle:UITableViewCellSelectionStyleNone];

  cell.backgroundColor = [UIColor clearColor];

  cell.label.attributedText = self.linkText;
  // Decide cell.label.textColor in order:
  //   1. styler.cellTitleColor
  //   2. [UIColor colorNamed:kTextSecondaryColor]
  if (styler.cellTitleColor) {
    cell.label.textColor = styler.cellTitleColor;
  } else {
    cell.label.textColor = [UIColor colorNamed:kTextSecondaryColor];
  }
  [cell enableItemSpacing:[self.linkText length]];
  [cell disableButtonIntrinsicWidth:self.disableButtonIntrinsicWidth];
  cell.label.textAlignment = self.textAlignment;

  [cell.button setTitle:self.buttonText forState:UIControlStateNormal];
  [cell disableButtonIntrinsicWidth:self.disableButtonIntrinsicWidth];
  // Decide cell.button titleColor in order:
  //   1. self.buttonTextColor;
  //   2. [UIColor colorNamed:kSolidButtonTextColor]
  if (self.buttonTextColor) {
    [cell.button setTitleColor:self.buttonTextColor
                      forState:UIControlStateNormal];
  } else {
    [cell.button setTitleColor:[UIColor colorNamed:kSolidButtonTextColor]
                      forState:UIControlStateNormal];
  }
  cell.button.accessibilityIdentifier = self.buttonAccessibilityIdentifier;
  // Decide cell.button.backgroundColor in order:
  //   1. self.buttonBackgroundColor
  //   2. [UIColor colorNamed:kBlueColor]
  if (self.buttonBackgroundColor) {
    cell.button.backgroundColor = self.buttonBackgroundColor;
  } else {
    cell.button.backgroundColor = [UIColor colorNamed:kBlueColor];
  }
  cell.button.enabled = self.enabled;
  if (!self.enabled && self.dimBackgroundWhenDisabled) {
    cell.button.backgroundColor = [cell.button.backgroundColor
        colorWithAlphaComponent:kDisabledButtonAlpha];
  }
}

@end

@interface VivaldiTableViewLinkAndButtonCell ()
// StackView that contains the cell's Button and Label.
@property(nonatomic, strong) UIStackView* horizontalStackView;
// Constraints used to match the Button width to the StackView.
@property(nonatomic, strong) NSArray* expandedButtonWidthConstraints;
@end

@implementation VivaldiTableViewLinkAndButtonCell
@synthesize button = _button;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    // Create button.
    self.button = [UIButton buttonWithType:UIButtonTypeSystem];
    self.button.translatesAutoresizingMaskIntoConstraints = NO;
    [self.button.titleLabel
        setFont:[UIFont preferredFontForTextStyle:UIFontTextStyleBody]];
    self.button.titleLabel.numberOfLines = 0;
    self.button.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
    self.button.titleLabel.textAlignment = NSTextAlignmentCenter;
    self.button.layer.cornerRadius = kButtonCornerRadius;
    self.button.clipsToBounds = YES;
    self.button.contentEdgeInsets = UIEdgeInsetsMake(
        kButtonTitleVerticalContentInset, kButtonTitleHorizontalContentInset,
        kButtonTitleVerticalContentInset, kButtonTitleHorizontalContentInset);

    self.button.pointerInteractionEnabled = YES;
    // This button's background color is configured whenever the cell is
    // reused. The pointer style provider used here dynamically provides the
    // appropriate style based on the background color at runtime.
    self.button.pointerStyleProvider =
        CreateOpaqueOrTransparentButtonPointerStyleProvider();

    // Horizontal stackView to hold label and button.
    self.horizontalStackView = [[UIStackView alloc]
        initWithArrangedSubviews:@[ self.label, self.button ]];
    self.horizontalStackView.alignment = UIStackViewAlignmentCenter;
    self.horizontalStackView.axis = UILayoutConstraintAxisHorizontal;
    self.horizontalStackView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.contentView addSubview:self.horizontalStackView];

    // Add constraints for stackView
    [self.horizontalStackView fillSuperviewWithPadding:
      UIEdgeInsetsMake(kStackViewVerticalSpacing,
                       kStackViewHorizontalSpacing,
                       kStackViewVerticalSpacing,
                       kStackViewHorizontalSpacing)];

    self.expandedButtonWidthConstraints = @[
      [self.button.leadingAnchor
          constraintEqualToAnchor:self.horizontalStackView.centerXAnchor],
      [self.button.trailingAnchor
          constraintEqualToAnchor:self.horizontalStackView.trailingAnchor],
    ];
  }
  return self;
}

#pragma mark - Public Methods

- (void)enableItemSpacing:(BOOL)enable {
  self.horizontalStackView.spacing = enable ? kStackViewSubViewSpacing : 0;
}

- (void)disableButtonIntrinsicWidth:(BOOL)disable {
  if (disable) {
    [NSLayoutConstraint
        activateConstraints:self.expandedButtonWidthConstraints];
  } else {
    [NSLayoutConstraint
        deactivateConstraints:self.expandedButtonWidthConstraints];
  }
}

- (UITextView*)label {
  if (!_label) {
    _label = CreateUITextViewWithTextKit1();
    _label.textColor = [UIColor colorNamed:kTextSecondaryColor];
    _label.scrollEnabled = NO;
    _label.editable = NO;
    _label.backgroundColor = [UIColor clearColor];
    _label.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
    _label.textAlignment = NSTextAlignmentCenter;
    _label.textContainerInset = UIEdgeInsetsZero;
    _label.linkTextAttributes =
        @{NSForegroundColorAttributeName : [UIColor colorNamed:kBlueColor]};
  }
  return _label;
}

#pragma mark - UITableViewCell

- (void)prepareForReuse {
  [super prepareForReuse];
  [self.button setTitleColor:[UIColor colorNamed:kSolidButtonTextColor]
                    forState:UIControlStateNormal];
  self.label.textAlignment = kDefaultTextAlignment;
  [self disableButtonIntrinsicWidth:NO];
}

@end
