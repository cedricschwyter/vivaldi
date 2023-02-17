// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_COORDINATOR_H_
#define IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

namespace syncer {
  class SyncSetupInProgressHandle;
}

@class VivaldiSyncCoordinator;
@protocol ApplicationCommands;

// Delegate for VivaldiSyncCoordinator.
@protocol VivaldiSyncCoordinatorDelegate <NSObject>

@property(nonatomic, weak) id<ApplicationCommands> applicationCommandsHandler;

// Called when the view controller is removed from its parent.
- (void)vivaldiSyncCoordinatorWasRemoved:
    (VivaldiSyncCoordinator*)coordinator;

@end

@interface VivaldiSyncCoordinator : ChromeCoordinator

- (instancetype)initWithBaseNavigationController:
                    (UINavigationController*)navigationController
                                         browser:(Browser*)browser
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser NS_UNAVAILABLE;

@property(nonatomic, weak) id<VivaldiSyncCoordinatorDelegate> delegate;

@end

#endif  // IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_COORDINATOR_H_
