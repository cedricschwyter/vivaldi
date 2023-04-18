// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_PANEL_SIDEBAR_PANEL_VIEW_CONTROLLER_H_
#define IOS_PANEL_SIDEBAR_PANEL_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/notes/note_navigation_controller.h"

class Browser;

@interface SidebarPanelViewController :
    UIViewController<UIPageViewControllerDelegate>
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithNibName:(NSString*)name
                         bundle:(NSBundle*)bundle NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;
- (void)setupControllers:(UINavigationController*)nvc
        withBookmarkController:(UINavigationController*)bvc
        andReadinglistController:(UINavigationController*)rvc
    andHistoryController:(UINavigationController*)hc;
- (void)panelDismissed;
- (void)setIndexForControl:(int)index;

@property(nonatomic, strong) UISegmentedControl* segmentControl;

@end

#endif  // IOS_PANEL_SIDEBAR_PANEL_VIEW_CONTROLLER_H_

