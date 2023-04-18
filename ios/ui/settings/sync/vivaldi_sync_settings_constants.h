// Copyright 2022-2023 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_SETTINGS_CONSTANTS_H_
#define IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_SETTINGS_CONSTANTS_H_

#import <UIKit/UIKit.h>

// Colors for sync status component (sorted by foreground value)
extern const int kSyncStatusGreenColor;
extern const int kSyncStatusGreenBackgroundColor;
extern const int kSyncStatusBlueColor;
extern const int kSyncStatusBlueBackgroundColor;
extern const int kSyncStatusRedColor;
extern const int kSyncStatusRedBackgroundColor;
extern const int kSyncStatusYellowColor;
extern const int kSyncStatusYellowBackgroundColor;

extern const CGFloat kFooterSectionHeight;

extern NSString* const kShowPasswordIcon;
extern NSString* const kHidePasswordIcon;
extern NSString* const kVivaldiIcon;

// Vivaldi URLs
extern NSString* const vVivaldiBrowserPrivacyUrl;
extern NSString* const vVivaldiCommunityPrivacyUrl;
extern NSString* const vVivaldiSyncActivateAccountUrl;
extern NSString* const vVivaldiSyncRegistrationUrl;
extern NSString* const vVivaldiSyncReSendActivationCodeUrl;
extern NSString* const vVivaldiSyncValidateFieldUrl;
extern NSString* const vVivaldiTermsOfServiceUrl;

// Vivaldi.net account creation validation
extern const int vUserMinimumValidAge;
extern const int vUserMaximumValidAge;
extern const unsigned long vUserMinimumPasswordLength;
extern const unsigned long vActivationCodeLength;

extern NSString* const vHttpMethod;
extern NSString* const vRequestHeader;
extern NSString* const vRequestValue;
extern const double vConnectionTimeout;

// Server error codes
extern NSString* const vErrorCode1004;
extern NSString* const vErrorCode1006;
extern NSString* const vErrorCode1007;
extern NSString* const vErrorCode2001;
extern NSString* const vErrorCode2002;
extern NSString* const vErrorCode2003;
extern NSString* const vErrorCode4001;
extern NSString* const vErrorCode4002;
extern NSString* const vErrorCode4003;
extern NSString* const vErrorCodeOther;

// Create account parameter names
extern const char vParamUsername[];
extern const char vParamPassword[];
extern const char vParamEmailAddress[];
extern const char vParamAge[];
extern const char vParamLanguage[];
extern const char vParamDisableNonce[];
extern const char vParamSubscribeNewletter[];
extern const char vParamActivationCode[];
// It is currently decided to disable Nonce and Captcha on android
extern const bool vDisableNonceValue;

extern const char vSuccessKey[];
extern const char vErrorKey[];
extern const char vAttemptsRemainingKey[];
extern const char vParamField[];
extern const char vParamValue[];
extern const char vParamEmailFieldName[];

extern const char kUsernameKey[];
extern const char kPasswordKey[];
extern const char kRecoveryEmailKey[];

#endif // IOS_UI_SETTINGS_SYNC_VIVALDI_SYNC_SETTINGS_CONSTANTS_H_