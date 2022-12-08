// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/common/password_manager_features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace password_manager::features {
// NOTE: It is strongly recommended to use UpperCamelCase style for feature
//       names, e.g. "MyGreatFeature".

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
// Enables biometric authentication before form filling.
BASE_FEATURE(kBiometricAuthenticationForFilling,
             "BiometricAuthenticationForFilling",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

#if BUILDFLAG(IS_MAC)
// Enables biometric authentication in settings.
BASE_FEATURE(kBiometricAuthenticationInSettings,
             "BiometricAuthenticationInSettings",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enables Biometrics for the Touch To Fill feature. This only effects Android.
BASE_FEATURE(kBiometricTouchToFill,
             "BiometricTouchToFill",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables submission detection for forms dynamically cleared but not removed
// from the page.
BASE_FEATURE(kDetectFormSubmissionOnFormClear,
             "DetectFormSubmissionOnFormClear",
#if BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

// Force enables password change capabilities for every domain, regardless of
// the server response. The flag is meant for end-to-end testing purposes only.
BASE_FEATURE(kForceEnablePasswordDomainCapabilities,
             "ForceEnablePasswordDomainCapabilities",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables favicons in Password Manager.
BASE_FEATURE(kEnableFaviconForPasswords,
             "EnableFaviconForPasswords",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables the overwriting of prefilled username fields if the server predicted
// the field to contain a placeholder value.
BASE_FEATURE(kEnableOverwritingPlaceholderUsernames,
             "EnableOverwritingPlaceholderUsernames",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables a second, Gaia-account-scoped password store for users who are signed
// in but not syncing.
BASE_FEATURE(kEnablePasswordsAccountStorage,
             "EnablePasswordsAccountStorage",
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kEnablePasswordGenerationForClearTextFields,
             "EnablePasswordGenerationForClearTextFields",
             base::FEATURE_ENABLED_BY_DEFAULT);

// By default, Password Manager is disabled in fenced frames for now.
// TODO(crbug.com/1294378): Remove once launched.
BASE_FEATURE(kEnablePasswordManagerWithinFencedFrame,
             "EnablePasswordManagerWithinFencedFrame",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables filling password on a website when there is saved password on
// affiliated website.
BASE_FEATURE(kFillingAcrossAffiliatedWebsites,
             "FillingAcrossAffiliatedWebsites",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables the experiment for the password manager to only fill on account
// selection, rather than autofilling on page load, with highlighting of fields.
BASE_FEATURE(kFillOnAccountSelect,
             "fill-on-account-select",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_LINUX)
// When enabled, initial sync will be forced during startup if the password
// store has encryption service failures.
BASE_FEATURE(kForceInitialSyncWhenDecryptionFails,
             "ForceInitialSyncWhenDecryptionFails",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enables finding a confirmation password field during saving by inspecting the
// values of the fields. Used as a kill switch.
// TODO(crbug.com/1164861): Remove once confirmed to be safe (around M92 or so).
BASE_FEATURE(kInferConfirmationPasswordField,
             "InferConfirmationPasswordField",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Feature flag that updates icons, strings, and views for Google Password
// Manager.
BASE_FEATURE(kIOSEnablePasswordManagerBrandingUpdate,
             "IOSEnablePasswordManagerBrandingUpdate",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_IOS)
// Removes the list of passwords from the Settings UI and adds a separate
// Password Manager view.
BASE_FEATURE(kIOSPasswordUISplit,
             "IOSPasswordUISplit",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables password saving and filling in cross-origin iframes on IOS.
BASE_FEATURE(kIOSPasswordManagerCrossOriginIframeSupport,
             "IOSPasswordManagerCrossOriginIframeSupport",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // IS_IOS

// Enables (un)muting compromised passwords from bulk leak check in settings.
BASE_FEATURE(kMuteCompromisedPasswords,
             "MuteCompromisedPasswords",
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

// Enables the new password viewing subpage.
BASE_FEATURE(kPasswordViewPageInSettings,
             "PasswordViewPageInSettings",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables sending credentials from the settings UI.
BASE_FEATURE(kSendPasswords,
             "SendPasswords",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables password leak detection for unauthenticated users.
BASE_FEATURE(kLeakDetectionUnauthenticated,
             "LeakDetectionUnauthenticated",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables automatic password change flow from leaked password dialog.
BASE_FEATURE(kPasswordChange,
             "PasswordChange",
#if BUILDFLAG(IS_ANDROID)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enables password change flow from bulk leak check in settings.
BASE_FEATURE(kPasswordChangeInSettings,
             "PasswordChangeInSettings",
#if BUILDFLAG(IS_ANDROID)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enables .well-known based password change flow from leaked password dialog.
BASE_FEATURE(kPasswordChangeWellKnown,
             "PasswordChangeWellKnown",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables fetching credentials capabilities from server for the
// |PasswordChangeInSettings| and |PasswordChange| features.
BASE_FEATURE(kPasswordDomainCapabilitiesFetching,
             "PasswordDomainCapabilitiesFetching",
#if BUILDFLAG(IS_ANDROID)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Controls the ability to import passwords from Chrome's settings page.
BASE_FEATURE(kPasswordImport,
             "PasswordImport",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
BASE_FEATURE(kPasswordManagerRedesign,
             "PasswordManagerRedesign",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enables password reuse detection.
BASE_FEATURE(kPasswordReuseDetectionEnabled,
             "PasswordReuseDetectionEnabled",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables password scripts fetching for the |PasswordChangeInSettings| feature.
BASE_FEATURE(kPasswordScriptsFetching,
             "PasswordScriptsFetching",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables requesting and saving passwords grouping information from the
// affiliation service.
// TODO(crbug.com/1359392): Remove once launched.
BASE_FEATURE(kPasswordsGrouping,
             "PasswordsGrouping",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables showing UI which allows users to easily revert their choice to
// never save passwords on a certain website.
BASE_FEATURE(kRecoverFromNeverSaveAndroid,
             "RecoverFromNeverSaveAndroid",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables the password strength indicator.
BASE_FEATURE(kPasswordStrengthIndicator,
             "PasswordStrengthIndicator",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// Displays at least the decryptable and never saved logins in the password
// manager
BASE_FEATURE(kSkipUndecryptablePasswords,
             "SkipUndecryptablePasswords",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

#if BUILDFLAG(IS_LINUX)
// When enabled, all undecryptable passwords are deleted from the local database
// during initial sync flow.
BASE_FEATURE(kSyncUndecryptablePasswordsLinux,
             "SyncUndecryptablePasswordsLinux",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

#if BUILDFLAG(IS_ANDROID)
BASE_FEATURE(kPasswordEditDialogWithDetails,
             "PasswordEditDialogWithDetails",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kShowUPMErrorNotification,
             "ShowUpmErrorNotification",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables the experiment to automatically submit a form after filling by
// TouchToFill
// TODO(crbug/1283004): Clean up the flag once the feature is completely landed
// in Stable.
BASE_FEATURE(kTouchToFillPasswordSubmission,
             "TouchToFillPasswordSubmission",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables the intent fetching for the credential manager in Google Mobile
// Services. It does not enable launching the credential manager.
BASE_FEATURE(kUnifiedCredentialManagerDryRun,
             "UnifiedCredentialManagerDryRun",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables use of Google Mobile Services for password storage. Chrome's local
// database will be unused but kept in sync for local passwords.
BASE_FEATURE(kUnifiedPasswordManagerAndroid,
             "UnifiedPasswordManagerAndroid",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables showing contextual error messages when UPM encounters an auth error.
BASE_FEATURE(kUnifiedPasswordManagerErrorMessages,
             "UnifiedPasswordManagerErrorMessages",
             base::FEATURE_DISABLED_BY_DEFAULT);

// If enabled, the built-in sync functionality in PasswordSyncBridge becomes
// unused, meaning that SyncService/SyncEngine will no longer download or
// upload changes to/from the Sync server. Instead, an external Android-specific
// backend will be used to achieve similar behavior.
BASE_FEATURE(kUnifiedPasswordManagerSyncUsingAndroidBackendOnly,
             "UnifiedPasswordManagerSyncUsingAndroidBackendOnly",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables automatic reenrollment into the Unified Password Manager for clients
// that were previously evicted after experiencing errors.
BASE_FEATURE(kUnifiedPasswordManagerReenrollment,
             "UnifiedPasswordManagerReenrollment",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables all UI branding changes related to Unified Password Manager:
// the strings containing 'Password Manager' and the password manager
// icon.
BASE_FEATURE(kUnifiedPasswordManagerAndroidBranding,
             "UnifiedPasswordManagerAndroidBranding",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enables support of sending additional votes on username first flow. The votes
// are sent on single password forms and contain information about preceding
// single username forms.
// TODO(crbug.com/959776): Clean up if the main crowdsourcing is good enough and
// we don't need additional signals.
BASE_FEATURE(kUsernameFirstFlowFallbackCrowdsourcing,
             "UsernameFirstFlowFallbackCrowdsourcing",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_ANDROID)
// Current migration version to Google Mobile Services. If version saved in pref
// is lower than 'kMigrationVersion' passwords will be re-uploaded.
extern const base::FeatureParam<int> kMigrationVersion = {
    &kUnifiedPasswordManagerAndroid, "migration_version", 1};

// The maximum possible number of reenrollments into the UPM. Needed to avoid a
// patchy experience for users who experience errors in communication with
// Google Mobile Services on a regular basis.
extern const base::FeatureParam<int> kMaxUPMReenrollments = {
    &kUnifiedPasswordManagerReenrollment, "max_reenrollments", 0};

// The maximum possible number of reenrollment migration attempts. Needed to
// avoid wasting resources of users who have persistent errors.
extern const base::FeatureParam<int> kMaxUPMReenrollmentAttempts = {
    &kUnifiedPasswordManagerReenrollment, "max_reenrollment_attempts", 0};

// Whether to ignore the 24h timeout in between auth error messages as
// well as the 30 mins distance to sync error messages.
extern const base::FeatureParam<bool> kIgnoreAuthErrorMessageTimeouts = {
    &kUnifiedPasswordManagerErrorMessages, "ignore_auth_error_message_timeouts",
    false};

// The maximum number of authentication error UI messages to show before
// considering auth errors as unrecoverable and unenrolling the user from UPM.
// If this param is set, unenrollment will happen even if the auth error is in
// the ignore list.
// By default, there is no limit to how many errors will be shown.
extern const base::FeatureParam<int> kMaxShownUPMErrorsBeforeEviction = {
    &kUnifiedPasswordManagerErrorMessages,
    "max_shown_auth_errors_before_eviction", -1};
#endif

// Field trial identifier for password generation requirements.
const char kGenerationRequirementsFieldTrial[] =
    "PasswordGenerationRequirements";

// The file version number of password requirements files. If the prefix length
// changes, this version number needs to be updated.
// Default to 0 in order to get an empty requirements file.
const char kGenerationRequirementsVersion[] = "version";

// Length of a hash prefix of domain names. This is used to shard domains
// across multiple files.
// Default to 0 in order to put all domain names into the same shard.
const char kGenerationRequirementsPrefixLength[] = "prefix_length";

// Timeout (in milliseconds) for password requirements lookups. As this is a
// network request in the background that does not block the UI, the impact of
// high values is not strong.
// Default to 5000 ms.
const char kGenerationRequirementsTimeout[] = "timeout";

// Enables showing leaked dialog after every successful form submission.
const char kPasswordChangeWithForcedDialogAfterEverySuccessfulSubmission[] =
    "should_force_dialog_after_every_sucessful_form_submission";

// Enables showing leaked warning for every site while doing bulk leak check in
// settings.
const char kPasswordChangeInSettingsWithForcedWarningForEverySite[] =
    "should_force_warning_for_every_site_in_settings";

#if BUILDFLAG(IS_ANDROID)
// Enables using conservative heuristics to calculate submission readiness.
const char kTouchToFillPasswordSubmissionWithConservativeHeuristics[] =
    "should_use_conservative_heuristics";
#endif  // IS_ANDROID

bool IsPasswordScriptsFetchingEnabled() {
  return base::FeatureList::IsEnabled(kPasswordScriptsFetching) ||
         base::FeatureList::IsEnabled(kPasswordDomainCapabilitiesFetching);
}

bool IsAutomatedPasswordChangeEnabled() {
  return base::FeatureList::IsEnabled(kPasswordChangeInSettings) ||
         base::FeatureList::IsEnabled(kPasswordChange);
}

#if BUILDFLAG(IS_ANDROID)
bool UsesUnifiedPasswordManagerUi() {
  if (!base::FeatureList::IsEnabled(kUnifiedPasswordManagerAndroid))
    return false;
  UpmExperimentVariation variation = kUpmExperimentVariationParam.Get();
  switch (variation) {
    case UpmExperimentVariation::kEnableForSyncingUsers:
    case UpmExperimentVariation::kEnableForAllUsers:
      return true;
    case UpmExperimentVariation::kShadowSyncingUsers:
    case UpmExperimentVariation::kEnableOnlyBackendForSyncingUsers:
      return false;
  }
  NOTREACHED() << "Define explicitly whether UI is required!";
  return false;
}

bool UsesUnifiedPasswordManagerBranding() {
  return (UsesUnifiedPasswordManagerUi() ||
          base::FeatureList::IsEnabled(kUnifiedPasswordManagerAndroidBranding));
}

bool RequiresMigrationForUnifiedPasswordManager() {
  if (!base::FeatureList::IsEnabled(kUnifiedPasswordManagerAndroid))
    return false;
  UpmExperimentVariation variation = kUpmExperimentVariationParam.Get();
  switch (variation) {
    case UpmExperimentVariation::kEnableForSyncingUsers:
    case UpmExperimentVariation::kEnableOnlyBackendForSyncingUsers:
    case UpmExperimentVariation::kEnableForAllUsers:
      return true;
    case UpmExperimentVariation::kShadowSyncingUsers:
      return false;
  }
  NOTREACHED() << "Define explicitly whether migration is required!";
  return false;
}

bool ManagesLocalPasswordsInUnifiedPasswordManager() {
  if (!base::FeatureList::IsEnabled(kUnifiedPasswordManagerAndroid))
    return false;
  UpmExperimentVariation variation = kUpmExperimentVariationParam.Get();
  switch (variation) {
    case UpmExperimentVariation::kEnableForSyncingUsers:
    case UpmExperimentVariation::kShadowSyncingUsers:
    case UpmExperimentVariation::kEnableOnlyBackendForSyncingUsers:
      return false;
    case UpmExperimentVariation::kEnableForAllUsers:
      return true;
  }
  NOTREACHED()
      << "Define explicitly whether local password management is supported!";
  return false;
}
#endif  // IS_ANDROID

}  // namespace password_manager::features
