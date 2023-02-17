// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/ui/table_view/cells/vivaldi_table_view_illustrated_item.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/util/text_view_util.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The insets of the View content and additional margin for some of its items.
const CGFloat kStackMargin = 32.0;
const CGFloat kItemMargin = 16.0;
// Spacing within stackView.
const CGFloat kStackViewSpacing = 13.0;
// Height of the image.
const CGFloat kImageViewHeight = 80.0;
}

#pragma mark - VivaldiTableViewIllustratedItem

@implementation VivaldiTableViewIllustratedItem

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [VivaldiTableViewIllustratedCell class];
  }
  return self;
}

- (void)configureCell:(TableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];
  VivaldiTableViewIllustratedCell* cell =
      base::mac::ObjCCastStrict<VivaldiTableViewIllustratedCell>(tableCell);
  if ([self.accessibilityIdentifier length]) {
    cell.accessibilityIdentifier = self.accessibilityIdentifier;
  }
  [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
  if (self.image) {
    cell.illustratedImageView.image = self.image;
  } else {
    cell.illustratedImageView.hidden = YES;
  }
  if ([self.title length]) {
    cell.titleLabel.text = self.title;
  } else {
    cell.titleLabel.hidden = YES;
  }
  if ([self.subtitle length]) {
    cell.subtitleLabel.attributedText = self.subtitle;
  } else {
    cell.subtitleLabel.hidden = YES;
  }
  cell.backgroundColor = nil;

  if (styler.cellTitleColor) {
    cell.titleLabel.textColor = styler.cellTitleColor;
  }
}

@end

#pragma mark - VivaldiTableViewIllustratedCell

@implementation VivaldiTableViewIllustratedCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    _illustratedImageView = [[UIImageView alloc] initWithImage:nil];
    _illustratedImageView.contentMode = UIViewContentModeScaleAspectFit;
    _illustratedImageView.clipsToBounds = YES;

    _titleLabel = [[UILabel alloc] init];
    _titleLabel.textColor = [UIColor colorNamed:kTextPrimaryColor];
    _titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleTitle1];
    _titleLabel.textAlignment = NSTextAlignmentCenter;
    _titleLabel.numberOfLines = 0;

    UIStackView* stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
      _illustratedImageView, _titleLabel, self.subtitleLabel
    ]];
    stackView.axis = UILayoutConstraintAxisVertical;
    stackView.alignment = UIStackViewAlignmentCenter;
    stackView.spacing = kStackViewSpacing;
    [self.contentView addSubview:stackView];

    // Layout
    [_illustratedImageView anchorTop:nil
           leading:stackView.leadingAnchor
            bottom:nil
          trailing:stackView.trailingAnchor
              size:CGSizeMake(0,kImageViewHeight)];

    [_titleLabel anchorTop:nil
           leading:stackView.leadingAnchor
            bottom:nil
          trailing:stackView.trailingAnchor];

    [self.subtitleLabel anchorTop:nil
           leading:stackView.leadingAnchor
            bottom:nil
          trailing:stackView.trailingAnchor
          padding:UIEdgeInsetsMake(0,kItemMargin,0,kItemMargin)];

    [stackView anchorTop:self.contentView.topAnchor
           leading:self.contentView.leadingAnchor
            bottom:self.contentView.bottomAnchor
          trailing:self.contentView.trailingAnchor
           padding:UIEdgeInsetsMake(0,kStackMargin,0,kStackMargin)];
  }
  return self;
}

- (UITextView*)subtitleLabel {
  if (!_subtitleLabel) {
    _subtitleLabel = CreateUITextViewWithTextKit1();
    _subtitleLabel.textColor = [UIColor colorNamed:kTextSecondaryColor];
    _subtitleLabel.scrollEnabled = NO;
    _subtitleLabel.editable = NO;
    _subtitleLabel.backgroundColor = [UIColor clearColor];
    _subtitleLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
    _subtitleLabel.textAlignment = NSTextAlignmentCenter;
    _subtitleLabel.textContainerInset = UIEdgeInsetsZero;
    _subtitleLabel.linkTextAttributes =
        @{NSForegroundColorAttributeName : [UIColor colorNamed:kBlueColor]};
  }
  return _subtitleLabel;
}

- (void)prepareForReuse {
  [super prepareForReuse];

  self.illustratedImageView.hidden = NO;
  self.titleLabel.hidden = NO;
  self.subtitleLabel.hidden = NO;
  self.subtitleLabel.delegate = nil;
}

@end
