// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_NTP_CELLS_VIVALDI_SPEED_DIAL_REGULAR_CELL_H_
#define IOS_CHROME_BROWSER_UI_NTP_CELLS_VIVALDI_SPEED_DIAL_REGULAR_CELL_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/vivaldi_speed_dial_item.h"
#import "ios/chrome/common/ui/favicon/favicon_attributes.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_layout_style.h"

// The cell that renders the speed dial URL items for layout style 'Large' and
// 'Medium'
@interface VivaldiSpeedDialRegularCell : UICollectionViewCell

// INITIALIZER
- (instancetype)initWithFrame:(CGRect)rect;

// SETTERS
- (void)configureCellWith:(const VivaldiSpeedDialItem*)item
              layoutStyle:(VivaldiStartPageLayoutStyle)style;
- (void)configureCellWithAttributes:(const FaviconAttributes*)attributes;
- (void)configurePreview;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_CELLS_VIVALDI_SPEED_DIAL_REGULAR_CELL_H_
