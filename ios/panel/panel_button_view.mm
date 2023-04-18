// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/panel/panel_button_view.h"

#import "UIKit/UIKit.h"

#import "ios/panel/panel_button_cell.h"
#import "ios/panel/panel_interaction_controller.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// NAMESPACE
namespace {
// Cell Identifier for the top menu CV Cell.
NSString* cellId = @"cellId";
// Collection view line spacing between items.
CGFloat lineSpacing = 12.0;

NSInteger numberOfPages = 4;

CGFloat iconSize = 56.0;
}

@interface PanelButtonView() <UICollectionViewDelegate,
                                    UICollectionViewDataSource> {
    NSInteger activeIndex;
}
@property (weak,nonatomic) UICollectionView *collectionView;
@end

@implementation PanelButtonView

@synthesize collectionView = _collectionView;

#pragma mark - INITIALIZER
- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    [self setUpUI];
  }
  return self;
}


#pragma mark - SET UP UI COMPONENTS
- (void)setUpUI {
  self.backgroundColor = UIColor.clearColor;
  activeIndex = 0;

  // COLLECTION VIEW FLOW LAYOUT
  UICollectionViewFlowLayout *layout= [[UICollectionViewFlowLayout alloc] init];
  UICollectionView* collectionView =
  [[UICollectionView alloc] initWithFrame: self.frame
                     collectionViewLayout:layout];
  _collectionView = collectionView;
  layout.scrollDirection = UICollectionViewScrollDirectionVertical;
  layout.minimumLineSpacing = lineSpacing;
  layout.minimumInteritemSpacing = 0;

  [self.collectionView setDataSource: self];
  [self.collectionView setDelegate: self];
  self.collectionView.showsHorizontalScrollIndicator = false;
  self.collectionView.showsVerticalScrollIndicator = false;
  self.collectionView.contentInsetAdjustmentBehavior =
      UIScrollViewContentInsetAdjustmentNever;

  [self.collectionView registerClass:[PanelButtonCell class]
      forCellWithReuseIdentifier:cellId];
  [self.collectionView setBackgroundColor:[UIColor clearColor]];

  [self addSubview:self.collectionView];
  [self.collectionView fillSuperview];
}

#pragma mark - SETTERS

- (void)selectItemWithIndex:(NSInteger)index {
    activeIndex = index;
    [self scrollToItemWithIndex:index];
}

#pragma mark - PRIVATE METHODS
- (void)scrollToItemWithIndex:(NSInteger)index {
  NSIndexPath *indexPath = [NSIndexPath indexPathForRow:index
                                              inSection:0];
  [self.collectionView
      selectItemAtIndexPath:indexPath
                   animated:true
             scrollPosition:UICollectionViewScrollPositionCenteredHorizontally];
}

#pragma mark - COLLECTIONVIEW DELEGATE, DATA SOURCE & FLOW LAYOUT

- (NSInteger)collectionView:(UICollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section {
    return numberOfPages;
}

- (UICollectionViewCell*)collectionView:(UICollectionView *)collectionView
                           cellForItemAtIndexPath:(NSIndexPath *)indexPath {
  PanelButtonCell *cell =
    [collectionView dequeueReusableCellWithReuseIdentifier:cellId
                                              forIndexPath:indexPath];
  if (indexPath.row == activeIndex)
    [cell configureHighlightedCellWithIndex:indexPath.row];
  else
    [cell configureCellWithIndex:indexPath.row];
  return cell;
}

- (void)collectionView:(UICollectionView *)collectionView
didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
  activeIndex = indexPath.row;
  if (self.delegate) {
    [self.delegate didSelectIndex:indexPath.row];
  }
  [self.collectionView reloadData];
  [self scrollToItemWithIndex:indexPath.row];
}

- (CGSize)collectionView:(UICollectionView *)collectionView
                  layout:(UICollectionViewLayout *)collectionViewLayout
  sizeForItemAtIndexPath:(NSIndexPath *)indexPath {
  return CGSizeMake(iconSize, iconSize);
}

@end
