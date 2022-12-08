// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_PANEL_PANEL_VIEW_CONTROLLER_H_
#define IOS_PANEL_PANEL_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/notes/note_navigation_controller.h"

class Browser;

// The Panel Controller
@interface PanelViewController : UIViewController<UIPageViewControllerDelegate>
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithNibName:(NSString*)name
                         bundle:(NSBundle*)bundle NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;
- (void)setupControllers:(UINavigationController*)nvc
        withBookmarkController:(UINavigationController*)bvc
    andHistoryController:(UINavigationController*)hc;
- (void)panelDismissed;
- (void)setIndexForControl:(int)index;

@property(nonatomic, strong) UISegmentedControl* segmentControl;

@end

#endif  // IOS_PANEL_PANEL_VIEW_CONTROLLER_H_

