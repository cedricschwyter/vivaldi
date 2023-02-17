// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_NTP_CELLS_VIVALDI_SPEED_DIAL_FOLDER_LIST_CELL_H_
#define IOS_CHROME_BROWSER_UI_NTP_CELLS_VIVALDI_SPEED_DIAL_FOLDER_LIST_CELL_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/vivaldi_speed_dial_item.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_layout_style.h"

// The cell that renders the speed dial folder item for layout 'List'
@interface VivaldiSpeedDialFolderListCell : UICollectionViewCell

// INITIALIZER
- (instancetype)initWithFrame:(CGRect)rect;

// SETTERS
- (void)configureCellWith:(VivaldiSpeedDialItem*)item
                   addNew:(bool)addNew;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_CELLS_VIVALDI_SPEED_DIAL_FOLDER_LIST_CELL_H_
