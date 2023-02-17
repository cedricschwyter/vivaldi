// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/chrome/browser/ui/ntp/cells/vivaldi_speed_dial_list_cell.h"

#import "Foundation/Foundation.h"

#import "ios/chrome/browser/ui/ntp/vivaldi_ntp_constants.h"
#import "ios/chrome/browser/ui/ntp/vivaldi_speed_dial_constants.h"
#import "ios/ui/helpers/vivaldi_colors_helper.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"
#import "ios/ui/helpers/vivaldi_uiview_style_helper.h"

namespace {
// Padding for labels. In order - Top, Left, Bottom, Right
const UIEdgeInsets faviconPadding =
    UIEdgeInsetsMake(0.f, 12.f, 0.f, 0.f);
const UIEdgeInsets labelStackPadding =
    UIEdgeInsetsMake(4.f, 12.f, 4.f, 8.f);
const UIEdgeInsets titleLabelMaskViewPadding =
    UIEdgeInsetsMake(6.f, 10.f, 0.f, 40.f);
const UIEdgeInsets urlLabelMaskViewPadding =
    UIEdgeInsetsMake(6.f, 0.f, 4.f, 62.f);
}

@interface VivaldiSpeedDialListCell()
// The title label for the speed dial.
@property(nonatomic,weak) UILabel* titleLabel;
// The title label mask for preview purpose
@property(nonatomic,weak) UIView* titleLabelMaskView;
// The url label for the speed dial.
@property(nonatomic,weak) UILabel* urlLabel;
// The url label mask for preview purpose
@property(nonatomic,weak) UIView* urlLabelMaskView;
// Imageview for the favicon.
@property(nonatomic,weak) UIImageView* faviconView;
@end

@implementation VivaldiSpeedDialListCell

@synthesize titleLabel = _titleLabel;
@synthesize titleLabelMaskView = _titleLabelMaskView;
@synthesize urlLabel = _urlLabel;
@synthesize urlLabelMaskView = _urlLabelMaskView;
@synthesize faviconView = _faviconView;

#pragma mark - INITIALIZER
- (id)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setUpUI];
  }
  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.titleLabel.text = nil;
  self.urlLabel.text = nil;
  self.faviconView.image = nil;
  self.titleLabelMaskView.hidden = YES;
  self.urlLabelMaskView.hidden = YES;
}


#pragma mark - SET UP UI COMPONENTS
- (void)setUpUI {

  self.layer.cornerRadius = vSpeedDialItemCornerRadius;
  self.clipsToBounds = true;
  self.layer.borderColor = UIColor.clearColor.CGColor;
  // Add drop shadow to parent
  [self
    addShadowWithBackground: [UIColor colorNamed:
                              vNTPSpeedDialCellBackgroundColor]
                     offset: vSpeedDialItemShadowOffset
                shadowColor: [UIColor colorNamed:vSpeedDialItemShadowColor]
                     radius: vSpeedDialItemShadowRadius
                    opacity: vSpeedDialItemShadowOpacity
  ];
  self.backgroundColor =
    [UIColor colorNamed: vNTPSpeedDialCellBackgroundColor];

  // Container view to hold the labels and image views.
  UIView *container = [UIView new];
  container.backgroundColor =
    [UIColor colorNamed: vNTPSpeedDialCellBackgroundColor];
  container.layer.cornerRadius = vSpeedDialItemCornerRadius;
  container.clipsToBounds = true;
  [self addSubview:container];
  [container fillSuperview];

  // Favicon view
  UIImageView* faviconView = [[UIImageView alloc] initWithImage:nil];
  _faviconView = faviconView;
  faviconView.contentMode = UIViewContentModeScaleAspectFit;
  faviconView.backgroundColor = UIColor.clearColor;
  faviconView.layer.cornerRadius = vSpeedDialFaviconCornerRadius;
  faviconView.clipsToBounds = true;
  [container addSubview: faviconView];
  [faviconView anchorTop: nil
                 leading: container.leadingAnchor
                  bottom: nil
                trailing: nil
                 padding: faviconPadding
                    size: vSpeedDialItemFaviconSizeListLayout];
  [faviconView centerYInSuperview];

  // Website title label
  UILabel* titleLabel = [[UILabel alloc] init];
  _titleLabel = titleLabel;
  titleLabel.textColor = [UIColor colorNamed:vNTPSpeedDialDomainTextColor];
  titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  titleLabel.numberOfLines = 1;
  titleLabel.textAlignment = NSTextAlignmentLeft;

  // Website url label
  UILabel* urlLabel = [[UILabel alloc] init];
  _urlLabel = urlLabel;
  urlLabel.textColor = UIColor.secondaryLabelColor;
  urlLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
  urlLabel.numberOfLines = 1;
  urlLabel.textAlignment = NSTextAlignmentLeft;

  // Website title and url label stack
  UIStackView *labelStack = [[UIStackView alloc] initWithArrangedSubviews:@[
    titleLabel, urlLabel
  ]];
  labelStack.axis = UILayoutConstraintAxisVertical;
  labelStack.distribution = UIStackViewDistributionFillEqually;
  labelStack.spacing = 0;

  [container addSubview: labelStack];
  [labelStack anchorTop: nil
                leading: faviconView.trailingAnchor
                 bottom: nil
               trailing: container.trailingAnchor
                padding: labelStackPadding];
  [labelStack centerYInSuperview];


  // Title label mask, used for preview only
  UIView* titleLabelMaskView = [UIView new];
  _titleLabelMaskView = titleLabelMaskView;
  titleLabelMaskView.backgroundColor = UIColor.clearColor;
  titleLabelMaskView.layer.cornerRadius = vSpeedDialFaviconCornerRadius;
  titleLabelMaskView.clipsToBounds = true;

  [container addSubview:titleLabelMaskView];
  [titleLabelMaskView anchorTop: faviconView.topAnchor
                        leading: faviconView.trailingAnchor
                         bottom: nil
                       trailing: container.trailingAnchor
                        padding: titleLabelMaskViewPadding];
  titleLabelMaskView.hidden = YES;

  // URL label mask, used for preview only
  UIView* urlLabelMaskView = [UIView new];
  _urlLabelMaskView = urlLabelMaskView;
  urlLabelMaskView.backgroundColor = UIColor.clearColor;
  urlLabelMaskView.layer.cornerRadius = vSpeedDialFaviconCornerRadius;
  urlLabelMaskView.clipsToBounds = true;

  [container addSubview:urlLabelMaskView];
  [urlLabelMaskView anchorTop: titleLabelMaskView.bottomAnchor
                      leading: titleLabelMaskView.leadingAnchor
                       bottom: faviconView.bottomAnchor
                     trailing: titleLabelMaskView.trailingAnchor
                      padding: urlLabelMaskViewPadding];
  urlLabelMaskView.hidden = YES;

  [titleLabelMaskView.heightAnchor
    constraintEqualToAnchor:urlLabelMaskView.heightAnchor].active = YES;
}


#pragma mark - SETTERS

- (void)configureCellWith:(VivaldiSpeedDialItem*)item {
  self.titleLabel.text = item.title;
  self.urlLabel.text = item.urlString;
}

- (void)configureCellWithAttributes:(FaviconAttributes*)attributes {
  if (!attributes) {
    self.faviconView.backgroundColor = UIColor.grayColor;
    return;
  }

  if (attributes.faviconImage) {
    self.faviconView.image = attributes.faviconImage;
  } else {
    // Do something for fallback
  }
}

- (void)configurePreview {
  self.faviconView.backgroundColor = UIColor.vSystemGray03;
  self.titleLabelMaskView.hidden = NO;
  self.titleLabelMaskView.backgroundColor = UIColor.vSystemGray03;
  self.urlLabelMaskView.hidden = NO;
  self.urlLabelMaskView.backgroundColor =
    [UIColor.vSystemGray03 colorWithAlphaComponent:0.4];
}
@end
