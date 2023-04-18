// Copyright 2022-2023 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/sync/vivaldi_sync_settings_constants.h"

// Colors for sync status component (sorted by foreground value)
const int kSyncStatusGreenColor = 0x2ACF58;
const int kSyncStatusGreenBackgroundColor = 0xD4F5DE;
const int kSyncStatusBlueColor = 0X4A67FB;
const int kSyncStatusBlueBackgroundColor = 0xDBE1FE;
const int kSyncStatusRedColor = 0xE32351;
const int kSyncStatusRedBackgroundColor = 0xFADCE3;
const int kSyncStatusYellowColor = 0XFFCC48;
const int kSyncStatusYellowBackgroundColor = 0xFFF7E2;

const CGFloat kFooterSectionHeight = 32.0;

NSString* const kShowPasswordIcon = @"infobar_reveal_password_icon";
NSString* const kHidePasswordIcon = @"infobar_hide_password_icon";
NSString* const kVivaldiIcon = @"vivaldi_icon";

// Vivaldi URLs
NSString* const vVivaldiBrowserPrivacyUrl =
    @"https://vivaldi.com/privacy/browser/";
NSString* const vVivaldiCommunityPrivacyUrl =
    @"https://vivaldi.com/privacy/community-privacy-policy";
NSString* const vVivaldiSyncActivateAccountUrl =
    @"https://login.vivaldi.net/profile/activation-code";
NSString* const vVivaldiSyncRegistrationUrl =
    @"https://login.vivaldi.net/profile/sync-registration";
NSString* const vVivaldiSyncReSendActivationCodeUrl =
    @"https://login.vivaldi.net/profile/resend-activation-code";
NSString* const vVivaldiSyncValidateFieldUrl =
    @"https://login.vivaldi.net/profile/validateField";
NSString* const vVivaldiTermsOfServiceUrl =
    @"https://vivaldi.com/privacy/vivaldi-end-user-license-agreement/";

// Vivaldi.net account creation validation
const int vUserMinimumValidAge = 16;
const int vUserMaximumValidAge = 150;
const unsigned long vUserMinimumPasswordLength = 12;
const unsigned long vActivationCodeLength = 6;

NSString* const vHttpMethod = @"POST";
NSString* const vRequestHeader = @"Content-Type";
NSString* const vRequestValue = @"application/json;utf-8";
const double vConnectionTimeout = 60.0;

// Server error codes
NSString* const vErrorCode1004 = @"1004";
NSString* const vErrorCode1006 = @"1006";
NSString* const vErrorCode1007 = @"1007";
NSString* const vErrorCode2001 = @"2001";
NSString* const vErrorCode2002 = @"2002";
NSString* const vErrorCode2003 = @"2003";
NSString* const vErrorCode4001 = @"4001";
NSString* const vErrorCode4002 = @"4002";
NSString* const vErrorCode4003 = @"4003";
NSString* const vErrorCodeOther = @"";

// Create account parameter names
const char vParamUsername[] = "username";
const char vParamPassword[] = "password";
const char vParamEmailAddress[] = "emailaddress";
const char vParamAge[] = "age";
const char vParamLanguage[] = "lang";
const char vParamSubscribeNewletter[] = "newsletter";
const char vParamActivationCode[] = "activation_code";
const char vParamDisableNonce[] = "disableNonce";
// It is currently decided to disable Nonce and Captcha on iOS
const bool vDisableNonceValue = true;

const char vSuccessKey[] = "success";
const char vErrorKey[] = "error";
const char vAttemptsRemainingKey[] = "attempts_remaining";
const char vParamField[] = "field";
const char vParamValue[] = "value";
const char vParamEmailFieldName[] = "email";

const char kUsernameKey[] = "username";
const char kPasswordKey[] = "password";
const char kRecoveryEmailKey[] = "recovery_email";