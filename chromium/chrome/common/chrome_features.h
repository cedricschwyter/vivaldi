// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the chrome
// module.

#ifndef CHROME_COMMON_CHROME_FEATURES_H_
#define CHROME_COMMON_CHROME_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "build/chromeos_buildflags.h"
#include "chrome/common/buildflags.h"
#include "device/vr/buildflags/buildflags.h"
#include "ppapi/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "ui/base/buildflags.h"

namespace features {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kActivityReportingSessionType);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAdaptiveScreenBrightnessLogging);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAllowDisableTouchpadHapticFeedback);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAllowTouchpadHapticClickSettings);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAnonymousUpdateChecks);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAppDeduplicationService);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAppDiscoveryForOobe);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAppManagementAppDetails);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAppPreloadService);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAppProvisioningStatic);
#endif

#if BUILDFLAG(IS_MAC)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAppShimRemoteCocoa);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAppShimNewCloseBehavior);
#endif  // BUILDFLAG(IS_MAC)

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAsyncDns);

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_FUCHSIA)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAutofillAddressSurvey);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAutofillCardSurvey);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kAutofillPasswordSurvey);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kBackgroundModeAllowRestart);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kBlockMigratedDefaultChromeAppSync);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kBorealis);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kBrowserAppInstanceTracking);
#endif  // BUILDFLAG(IS_CHROMEOS)

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kChangePictureVideoMode);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kClientStorageAccessContextAuditing);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCrOSEnableUSMUserService);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kCrosCompUpdates);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kCrostini);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCrostiniAdditionalEnterpriseReporting);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCrostiniAdvancedAccessControls);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCrostiniAnsibleInfrastructure);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCrostiniAnsibleSoftwareManagement);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kCrostiniArcSideload);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kCrostiniForceClose);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCryptohomeDistributedModel);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kCryptohomeUserDataAuth);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kCryptohomeUserDataAuthKillswitch);
#endif

#if BUILDFLAG(IS_CHROMEOS)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kCrosPrivacyHub);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDataLeakPreventionPolicy);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDataLeakPreventionFilesRestriction);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDefaultLinkCapturingInBrowser);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDMServerOAuthForChildUser);
#endif

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kPreinstalledWebAppInstallation);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kPreinstalledWebAppDuplicationFixer);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPWAsDefaultOfflinePage);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsAdditionalWindowingControls);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsCacheDuringDefaultInstall);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsElidedExtensionsMenu);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsEnforceWebAppSettingsPolicy);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsFlashAppNameInsteadOfOrigin);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsIconHealthChecks);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsRunOnOsLogin);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDesktopPWAsTabStripSettings);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kDesktopPWAsWebBundles);

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_FUCHSIA)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kChromeAppsDeprecation);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kKeepForceInstalledPreinstalledApps);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kDisruptiveNotificationPermissionRevocation);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kDnsOverHttps);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<bool> kDnsOverHttpsFallbackParam;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<bool> kDnsOverHttpsShowUiParam;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kDnsOverHttpsTemplatesParam;
COMPONENT_EXPORT(CHROME_FEATURES)

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kDnsProxyEnableDOH);
#endif

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kEarlyLibraryLoad);
#endif

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kElidePrioritizationOfPreNativeBootstrapTasks);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kEnableAmbientAuthenticationInGuestSession);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kEnableAmbientAuthenticationInIncognito);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kEnableRestrictedWebApis);

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kEnableWebHidOnExtensionServiceWorker);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kEnableWebUsbOnExtensionServiceWorker);

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kExtensionDeferredIndividualSettings);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kExtensionWorkflowJustification);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kEnterpriseReportingInChromeOS);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kExternalExtensionDefaultButtonControl);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kFileTransferEnterpriseConnector);
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kFlashDeprecationWarning);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kFocusMode);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kGeoLanguage);

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopDemo);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopPrivacySandbox);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kHappinessTrackingSurveysForDesktopPrivacySandboxTime;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopSettings);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopSettingsPrivacy);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<bool>
    kHappinessTrackingSurveysForDesktopSettingsPrivacyNoSandbox;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<bool>
    kHappinessTrackingSurveysForDesktopSettingsPrivacyNoGuide;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kHappinessTrackingSurveysForDesktopSettingsPrivacyTime;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopPrivacyGuide);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kHappinessTrackingSurveysForDesktopPrivacyGuideTime;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopNtpModules);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForNtpPhotosOptOut);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSurveysForDesktopWhatsNew);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kHappinessTrackingSurveysForDesktopWhatsNewTime;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHaTSDesktopDevToolsIssuesCOEP);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHaTSDesktopDevToolsIssuesMixedContent);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(
    kHappinessTrackingSurveysForDesktopDevToolsIssuesCookiesSameSite);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHaTSDesktopDevToolsIssuesHeavyAd);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHaTSDesktopDevToolsIssuesCSP);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystem);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemEnt);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemStability);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemPerformance);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemOnboarding);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemUnlock);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemSmartLock);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemArcGames);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemAudio);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingPersonalizationAvatar);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingPersonalizationScreensaver);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingPersonalizationWallpaper);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingMediaAppPdf);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingSystemCameraApp);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingPhotosExperience);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kHappinessTrackingGeneralCamera);

#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kHideWebAppOriginText);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kHttpsOnlyMode);

#if BUILDFLAG(IS_MAC)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kImmersiveFullscreen);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kImmersiveFullscreenPWAs);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kInSessionPasswordChange);
#endif

#if BUILDFLAG(IS_WIN)
// Only has an effect in branded builds.
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kIncompatibleApplicationsWarning);
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kIncognitoDownloadsWarning);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kIncognitoNtpRevamp);

#if BUILDFLAG(IS_CHROMEOS)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kKioskEnableAppService);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kKAnonymityService);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kKAnonymityServiceAuthServer;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kKAnonymityServiceJoinRelayServer;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kKAnonymityServiceJoinServer;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta> kKAnonymityServiceJoinInterval;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kKAnonymityServiceQueryServer;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kKAnonymityServiceQueryRelayServer;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kKAnonymityServiceQueryInterval;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kKAnonymityServiceOHTTPRequests);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kUpdateHistoryEntryPointsInIncognito);

#if BUILDFLAG(IS_LINUX) && !BUILDFLAG(IS_CHROMEOS)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kLinuxLowMemoryMonitor);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<int> kLinuxLowMemoryMonitorModerateLevel;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<int> kLinuxLowMemoryMonitorCriticalLevel;
#endif  // BUILDFLAG(IS_LINUX) && !BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kListWebAppsSwitch);
#endif

#if BUILDFLAG(IS_MAC)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kMacSystemScreenCapturePermissionCheck);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kMeteredShowToggle);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kShowHiddenNetworkToggle);
#endif

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kMetricsSettingsAndroid);
#endif

#if BUILDFLAG(IS_CHROMEOS)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kMicrosoftOfficeWebAppExperiment);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kMoveWebApp);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kMoveWebAppUninstallStartUrlPrefix;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kMoveWebAppUninstallStartUrlPattern;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string> kMoveWebAppInstallStartUrl;

#if BUILDFLAG(ENABLE_SYSTEM_NOTIFICATIONS)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kNativeNotifications);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSystemNotifications);
#endif

#if BUILDFLAG(IS_MAC)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kNewMacNotificationAPI);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kNoReferrers);

#if BUILDFLAG(IS_WIN)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kNotificationDurationLongForRequireInteraction);
#endif

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kOnConnectNative);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kOobeMarketingAdditionalCountriesSupported);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kOobeMarketingDoubleOptInCountriesSupported);

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kOomIntervention);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kParentAccessCodeForOnlineLogin);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPermissionAuditing);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPermissionPredictions);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double> kPermissionPredictionsHoldbackChance;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kPermissionGeolocationPredictions);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kPermissionGeolocationPredictionsHoldbackChance;

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPluginVm);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kPrerenderFallbackToPreconnect);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPrivacyGuide2);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPrivacyGuideAndroid);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kPushMessagingBackgroundMode);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPwaUpdateDialogForIcon);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kPwaUpdateDialogForName);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kQuietNotificationPrompts);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kRecordWebAppDebugInfo);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kAbusiveNotificationPermissionRevocation);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kRemoveStatusBarInWebApps);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kRemoveSupervisedUsersOnStartup);
#endif

#if BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kRequestDesktopSiteForTablets);
#endif

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kSafetyCheckNotificationPermissions);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<int>
    kSafetyCheckNotificationPermissionsMinEnagementLimit;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<int>
    kSafetyCheckNotificationPermissionsLowEnagementLimit;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kSafetyCheckUnusedSitePermissions);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSchedulerConfiguration);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSCTAuditing);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double> kSCTAuditingSamplingRate;
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSCTAuditingHashdance);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta> kSCTLogExpectedIngestionDelay;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta> kSCTLogMaxIngestionRandomDelay;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kSecurityKeyAttestationPrompt);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSitePerProcess);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSmartDim);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSoundContentSetting);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSysInternals);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kTPMFirmwareUpdate);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kTabMetricsLogging);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSupportTool);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kSupportToolScreenshot);
#endif

#if BUILDFLAG(IS_WIN)
// Only has an effect in branded builds.
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kThirdPartyModulesBlocking);
#endif

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kTreatUnsafeDownloadsAsActive);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kTrustSafetySentimentSurvey);
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kTrustSafetySentimentSurveyMinTimeToPrompt;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kTrustSafetySentimentSurveyMaxTimeToPrompt;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<int>
    kTrustSafetySentimentSurveyNtpVisitsMinRange;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<int>
    kTrustSafetySentimentSurveyNtpVisitsMaxRange;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySettingsProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyTrustedSurfaceProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyTransactionsProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySandbox3ConsentAcceptProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySandbox3ConsentDeclineProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeDismissProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeOkProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeSettingsProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<double>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeLearnMoreProbability;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySettingsTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyTrustedSurfaceTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyTransactionsTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySandbox3ConsentAcceptTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySandbox3ConsentDeclineTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeDismissTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeOkTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeSettingsTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<std::string>
    kTrustSafetySentimentSurveyPrivacySandbox3NoticeLearnMoreTriggerId;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kTrustSafetySentimentSurveyPrivacySettingsTime;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kTrustSafetySentimentSurveyTrustedSurfaceTime;
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<base::TimeDelta>
    kTrustSafetySentimentSurveyTransactionsPasswordManagerTime;

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kUploadZippedSystemLogs);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kUserActivityEventLogging);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kUserTypeByDeviceTypeMetricsProvider);
#endif

// Android expects this string from Java code, so it is always needed.
// TODO(crbug.com/731802): Use #if BUILDFLAG(ENABLE_VR_BROWSING) instead.
#if BUILDFLAG(ENABLE_VR) || BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kVrBrowsing);
#endif
#if BUILDFLAG(ENABLE_VR)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kVrBrowsingExperimentalFeatures);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kVrBrowsingExperimentalRendering);
#endif  // ENABLE_VR

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kWebAppManifestIconUpdating);
#endif  // !BUILDFLAG(IS_ANDROID)

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kWebAppManifestPolicyAppIdentityUpdate);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kWebAppsCrosapi);

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kChromeKioskEnableLacros);

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kWebKioskEnableLacros);
#endif

#if !BUILDFLAG(IS_ANDROID)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kWebRtcRemoteEventLog);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kWebRtcRemoteEventLogGzipped);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_MAC)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kWebShare);
#endif

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kWebUIDarkMode);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kUmaStorageDimensions);
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kWilcoDtc);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_WIN)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kWin10AcceleratedDefaultBrowserFlow);
#endif  // BUILDFLAG(IS_WIN)

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kWriteBasicSystemProfileToPersistentHistogramsFile);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
bool IsParentAccessCodeForOnlineLoginEnabled();
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// This flag is used for enabling Omnibox triggered prerendering and
// blink::WebRuntimeFeatures::Prerender2RelatedFeatures that enables Prerender2
// related web exposed features. This flag takes effect only when
// blink::features::Prerender2 is enabled. See crbug.com/1166085 for more
// details of Omnibox triggered prerendering.
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kOmniboxTriggerForPrerender2);

// This flag controls whether to trigger prerendering when the default search
// engine suggests to prerender a search result. It also enables
// Prerender2-related features on the blink side. This flag takes effect only
// when blink::features::Prerender2 is enabled.
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kSupportSearchSuggestionForPrerender2);
enum class SearchSuggestionPrerenderImplementationType {
  kUsePrefetch,
  kIgnorePrefetch,
};
COMPONENT_EXPORT(CHROME_FEATURES)
extern const base::FeatureParam<SearchSuggestionPrerenderImplementationType>
    kSearchSuggestionPrerenderImplementationTypeParam;

COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kOmniboxTriggerForNoStatePrefetch);

#if BUILDFLAG(IS_CHROMEOS_ASH)
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kSupportsRtcWakeOver24Hours);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// This flag is used to toggle the reading of data from the web_app DB instead
// of the ExternallyInstalledWebAppPrefs. Data will be written to both storages,
// and this will be removed in the future once we move to using the web_app DB
// completely.
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kUseWebAppDBInsteadOfExternalPrefs);

bool PrefServiceEnabled();

// DON'T ADD RANDOM STUFF HERE. Put it in the main section above in
// alphabetical order, or in one of the ifdefs (also in order in each section).

}  // namespace features

#endif  // CHROME_COMMON_CHROME_FEATURES_H_
