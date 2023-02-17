// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/panel/panel_button_cell.h"

#import "ios/panel/panel_interaction_controller.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"

@interface PanelButtonCell() {}
@property(nonatomic, weak) UIImageView* imageView;
@end

@implementation PanelButtonCell

@synthesize imageView = _imageView;

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
  self.imageView.image = nil;
}

#pragma mark - SET UP UI COMPONENTS
- (void)setUpUI {

  // Container to hold the items
  UIView *container = [UIView new];
  [self addSubview:container];
  [container fillSuperview];

  // Icon set up
  UIImageView* imgView = [[UIImageView alloc] init];
  _imageView = imgView;
  _imageView.contentMode = UIViewContentModeCenter;
  _imageView.clipsToBounds = YES;

  [container addSubview:_imageView];
  [_imageView anchorTop:container.topAnchor
                 leading:container.leadingAnchor
                  bottom: nil
                trailing: container.trailingAnchor
                 padding: UIEdgeInsetsZero];
}

- (void)setSelected:(BOOL)selected {
    [super setSelected:selected];
  [self toggleHighlight];
}

- (void)toggleHighlight {
  [UIView transitionWithView: self.imageView
                    duration: 0.25
                     options: UIViewAnimationOptionTransitionCrossDissolve
                  animations:nil
                  completion: nil];
}

#pragma mark - SETTERS
- (void)configureCellWithIndex:(NSInteger)index {
  if (index == PanelPage::BookmarksPage)
      self.imageView.image = [UIImage
                        imageNamed:@"bookmark_panel"];
  else if (index == PanelPage::NotesPage)
      self.imageView.image = [UIImage
                        imageNamed:@"notes_panel"];
  else if (index == PanelPage::HistoryPage)
      self.imageView.image = [UIImage
                        imageNamed:@"history_panel"];
}

- (void)configureHighlightedCellWithIndex:(NSInteger)index {
    if (index == PanelPage::BookmarksPage) {
        self.imageView.image = [UIImage
                                imageNamed:@"bookmark_panel_active"];
    }  else if (index == PanelPage::NotesPage)
      self.imageView.image = [UIImage
                        imageNamed:@"notes_panel_active"];
  else if (index == PanelPage::HistoryPage)
      self.imageView.image = [UIImage
                        imageNamed:@"history_panel_active"];
    [self setNeedsLayout];
}


@end
