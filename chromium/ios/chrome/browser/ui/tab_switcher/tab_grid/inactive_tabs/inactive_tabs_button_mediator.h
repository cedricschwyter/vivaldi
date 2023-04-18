// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_INACTIVE_TABS_INACTIVE_TABS_BUTTON_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_INACTIVE_TABS_INACTIVE_TABS_BUTTON_MEDIATOR_H_

#import <Foundation/Foundation.h>

#import "base/memory/weak_ptr.h"

@protocol InactiveTabsCountConsumer;
class WebStateList;

// This mediator updates the button in the regular Tab Grid showing the presence
// and number of inactive tabs.
@interface InactiveTabsButtonMediator : NSObject

// Initializer with `consumer` as the receiver of `webStateList` count updates.
- (instancetype)initWithConsumer:(id<InactiveTabsCountConsumer>)consumer
                    webStateList:(base::WeakPtr<WebStateList>)webStateList
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_INACTIVE_TABS_INACTIVE_TABS_BUTTON_MEDIATOR_H_
