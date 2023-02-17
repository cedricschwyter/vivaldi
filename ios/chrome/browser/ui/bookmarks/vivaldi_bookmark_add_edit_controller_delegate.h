// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_VIVALDI_BOOKMARK_ADD_EDIT_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_VIVALDI_BOOKMARK_ADD_EDIT_CONTROLLER_DELEGATE_H_

#import <UIKit/UIKit.h>

#import "components/bookmarks/browser/bookmark_model.h"

@protocol VivaldiBookmarkAddEditControllerDelegate
- (void)didCreateNewFolder:(const bookmarks::BookmarkNode*)folder;
@end

#endif  // IOS_CHROME_BROWSER_UI_VIVALDI_BOOKMARK_ADD_EDIT_CONTROLLER_DELEGATE_H_
