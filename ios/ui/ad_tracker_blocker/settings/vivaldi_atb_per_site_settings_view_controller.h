// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_AD_TRACKER_BLOCKER_SETTINGS_VIVALDI_ATB_PER_SITE_SETTINGS_VIEW_CONTROLLER_H_
#define IOS_UI_AD_TRACKER_BLOCKER_SETTINGS_VIVALDI_ATB_PER_SITE_SETTINGS_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/table_view/chrome_table_view_controller.h"

@interface VivaldiATBPerSiteSettingsViewController :
  ChromeTableViewController <UIAdaptivePresentationControllerDelegate>

// INITIALIZER
- (instancetype)initWithTitle:(NSString*)title;

@end

#endif  // IOS_UI_AD_TRACKER_BLOCKER_SETTINGS_VIVALDI_ATB_PER_SITE_SETTINGS_VIEW_CONTROLLER_H_
