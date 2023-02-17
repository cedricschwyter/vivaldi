// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_SETTINGS_VIEW_CONTROLLER_SERIVCE_DELEGATE_H_
#define IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_SETTINGS_VIEW_CONTROLLER_SERIVCE_DELEGATE_H_

#import <UIKit/UIKit.h>

#import "components/sync/base/user_selectable_type.h"

@protocol VivaldiSyncSettingsViewControllerServiceDelegate

- (void)backupEncryptionKeyButtonPressed;
- (void)clearSyncDataWithNoWarning;
- (void)logOutButtonPressed;
- (void)startSyncingAllButtonPressed;
- (void)syncAllOptionChanged:(BOOL)syncAll;
- (void)updateChosenTypes:(syncer::UserSelectableType)type isOn:(BOOL)isOn;

@end

#endif  // IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_SETTINGS_VIEW_CONTROLLER_SERIVCE_DELEGATE_H_
