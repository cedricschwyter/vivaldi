// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "media" command-line switches.

#ifndef MEDIA_BASE_MEDIA_SWITCHES_H_
#define MEDIA_BASE_MEDIA_SWITCHES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "media/base/media_export.h"
#include "media/media_buildflags.h"

namespace base {
class CommandLine;
}

namespace switches {

MEDIA_EXPORT extern const char kAudioBufferSize[];

#if BUILDFLAG(ENABLE_PLATFORM_DTS_AUDIO) && BUILDFLAG(IS_WIN)
MEDIA_EXPORT extern const char kAudioCodecsFromEDID[];
#endif  // BUILDFLAG(ENABLE_PLATFORM_DTS_AUDIO) && BUILDFLAG(IS_WIN)

MEDIA_EXPORT extern const char kAudioServiceQuitTimeoutMs[];

MEDIA_EXPORT extern const char kAutoplayPolicy[];

MEDIA_EXPORT extern const char kDisableAudioOutput[];
MEDIA_EXPORT extern const char kFailAudioStreamCreation[];

MEDIA_EXPORT extern const char kVideoThreads[];

MEDIA_EXPORT extern const char kDisableBackgroundMediaSuspend[];

MEDIA_EXPORT extern const char kReportVp9AsAnUnsupportedMimeType[];

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_FREEBSD) || \
    BUILDFLAG(IS_SOLARIS)
MEDIA_EXPORT extern const char kAlsaInputDevice[];
MEDIA_EXPORT extern const char kAlsaOutputDevice[];
#endif

#if BUILDFLAG(IS_WIN)
MEDIA_EXPORT extern const char kEnableExclusiveAudio[];
MEDIA_EXPORT extern const char kForceWaveAudio[];
MEDIA_EXPORT extern const char kTrySupportedChannelLayouts[];
MEDIA_EXPORT extern const char kWaveOutBuffers[];
#endif

#if BUILDFLAG(IS_FUCHSIA)
MEDIA_EXPORT extern const char kEnableProtectedVideoBuffers[];
MEDIA_EXPORT extern const char kForceProtectedVideoOutputBuffers[];
MEDIA_EXPORT extern const char kDisableAudioInput[];
MEDIA_EXPORT extern const char kUseOverlaysForVideo[];
MEDIA_EXPORT extern const char kMinVideoDecoderOutputBufferSize[];
MEDIA_EXPORT extern const char kAudioCapturerWithEchoCancellation[];
#endif

#if defined(USE_CRAS)
MEDIA_EXPORT extern const char kUseCras[];
#endif

MEDIA_EXPORT extern const char
    kUnsafelyAllowProtectedMediaIdentifierForDomain[];

MEDIA_EXPORT extern const char kUseFakeDeviceForMediaStream[];
MEDIA_EXPORT extern const char kUseFileForFakeVideoCapture[];
MEDIA_EXPORT extern const char kUseFileForFakeAudioCapture[];
MEDIA_EXPORT extern const char kUseFakeMjpegDecodeAccelerator[];
MEDIA_EXPORT extern const char kDisableAcceleratedMjpegDecode[];

MEDIA_EXPORT extern const char kRequireAudioHardwareForTesting[];
MEDIA_EXPORT extern const char kMuteAudio[];

MEDIA_EXPORT extern const char kVideoUnderflowThresholdMs[];

MEDIA_EXPORT extern const char kDisableRTCSmoothnessAlgorithm[];

MEDIA_EXPORT extern const char kForceVideoOverlays[];

MEDIA_EXPORT extern const char kMSEAudioBufferSizeLimitMb[];
MEDIA_EXPORT extern const char kMSEVideoBufferSizeLimitMb[];

MEDIA_EXPORT extern const char kClearKeyCdmPathForTesting[];
MEDIA_EXPORT extern const char kOverrideEnabledCdmInterfaceVersion[];
MEDIA_EXPORT extern const char kOverrideHardwareSecureCodecsForTesting[];
MEDIA_EXPORT extern const char kEnableLiveCaptionPrefForTesting[];

#if BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT extern const char kLacrosEnablePlatformHevc[];
MEDIA_EXPORT extern const char kLacrosUseChromeosProtectedMedia[];
MEDIA_EXPORT extern const char kLacrosUseChromeosProtectedAv1[];
#endif  // BUILDFLAG(IS_CHROMEOS)

namespace autoplay {

MEDIA_EXPORT extern const char kDocumentUserActivationRequiredPolicy[];
MEDIA_EXPORT extern const char kNoUserGestureRequiredPolicy[];
MEDIA_EXPORT extern const char kUserGestureRequiredPolicy[];

}  // namespace autoplay

#if BUILDFLAG(USE_CHROMEOS_MEDIA_ACCELERATION)
MEDIA_EXPORT extern const char kHardwareVideoDecodeFrameRate[];
MEDIA_EXPORT extern const char kMaxChromeOSDecoderThreads[];
#endif

}  // namespace switches

namespace media {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.

MEDIA_EXPORT BASE_DECLARE_FEATURE(kAudioFocusDuckFlash);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kAudioFocusLossSuspendMediaSession);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kAutoplayIgnoreWebAudio);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kAutoplayDisableSettings);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kBackgroundVideoPauseOptimization);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kBresenhamCadence);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kCdmHostVerification);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kCdmProcessSiteIsolation);
#if BUILDFLAG(CHROME_WIDE_ECHO_CANCELLATION)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kChromeWideEchoCancellation);
MEDIA_EXPORT extern const base::FeatureParam<int>
    kChromeWideEchoCancellationProcessingFifoSize;
MEDIA_EXPORT extern const base::FeatureParam<bool>
    kChromeWideEchoCancellationMinimizeResampling;
MEDIA_EXPORT extern const base::FeatureParam<double>
    kChromeWideEchoCancellationDynamicMixingTimeout;
MEDIA_EXPORT extern const base::FeatureParam<bool>
    kChromeWideEchoCancellationAllowAllSampleRates;
#endif
MEDIA_EXPORT BASE_DECLARE_FEATURE(kD3D11VideoDecoderUseSharedHandle);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kEnableTabMuting);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kExposeSwDecodersToWebRTC);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kExternalClearKeyForTesting);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kFFmpegDecodeOpaqueVP8);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kFailUrlProvisionFetcherForTesting);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kFallbackAfterDecodeError);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGav1VideoDecoder);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControls);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControlsAutoDismiss);
#if BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControlsForCast);
#endif
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControlsForChromeOS);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControlsPictureInPicture);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControlsSeamlessTransfer);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalMediaControlsModernUI);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kHardwareMediaKeyHandling);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kHardwareSecureDecryption);
MEDIA_EXPORT extern const base::FeatureParam<bool>
    kHardwareSecureDecryptionForceSupportClearLead;
MEDIA_EXPORT BASE_DECLARE_FEATURE(kHardwareSecureDecryptionExperiment);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kHardwareSecureDecryptionFallback);
MEDIA_EXPORT extern const base::FeatureParam<int>
    kHardwareSecureDecryptionFallbackMinDisablingDays;
MEDIA_EXPORT extern const base::FeatureParam<int>
    kHardwareSecureDecryptionFallbackMaxDisablingDays;
MEDIA_EXPORT BASE_DECLARE_FEATURE(kInternalMediaSession);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kKeepRvfcFrameAlive);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kKeyPressMonitoring);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLiveCaption);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLiveCaptionRightClick);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLiveCaptionMultiLanguage);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLiveCaptionSystemWideOnChromeOS);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLiveTranslate);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLowDelayVideoRenderingOnLiveStream);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaCapabilitiesQueryGpuFactories);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaCapabilitiesWithParameters);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaCastOverlayButton);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaEngagementBypassAutoplayPolicies);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaEngagementHTTPSOnly);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaLearningExperiment);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaLearningFramework);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaLearningSmoothnessExperiment);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaOptimizer);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaPowerExperiment);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaSessionWebRTC);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMemoryPressureBasedSourceBufferGC);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMultiPlaneVideoCaptureSharedImages);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kOpenscreenCastStreamingSession);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kOverlayFullscreenVideo);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPictureInPicture);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPlatformAudioEncoder);
#if BUILDFLAG(ENABLE_PLATFORM_HEVC)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPlatformHEVCDecoderSupport);
#endif  // BUILDFLAG(ENABLE_PLATFORM_HEVC)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPlaybackSpeedButton);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPreloadMediaEngagementData);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPreloadMetadataLazyLoad);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPreloadMetadataSuspend);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kRecordMediaEngagementScores);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kRecordWebAudioEngagement);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kResumeBackgroundVideo);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kRevokeMediaSourceObjectURLOnAttach);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kShareThisTabInsteadButtonGetDisplayMedia);
MEDIA_EXPORT BASE_DECLARE_FEATURE(
    kShareThisTabInsteadButtonGetDisplayMediaAudio);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kSpeakerChangeDetection);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kSpecCompliantCanPlayThrough);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kSuspendMutedAudio);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUnifiedAutoplay);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseAndroidOverlayForSecureOnly);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseDecoderStreamForWebRTC);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseFakeDeviceForMediaStream);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseMediaHistoryStore);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseR16Texture);
#if BUILDFLAG(IS_LINUX)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVideoDecodeLinux);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVideoEncodeLinux);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiIgnoreDriverChecks);
#endif  // BUILDFLAG(IS_LINUX)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiAV1Decoder);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiLowPowerEncoderGen9x);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiEnforceVideoMinMaxResolution);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVideoMinResolutionForPerformance);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVP8Encoder);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVP9Encoder);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kGlobalVaapiLock);
#if defined(ARCH_CPU_X86_FAMILY) && BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiH264TemporalLayerHWEncoding);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVp8TemporalLayerHWEncoding);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVaapiVp9kSVCHWEncoding);
#endif  // defined(ARCH_CPU_X86_FAMILY) && BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVideoBlitColorAccuracy);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kVp9kSVCHWDecoding);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kWakeLockOptimisationHiddenMuted);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kWebContentsCaptureHiDpi);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kWebrtcMediaCapabilitiesParameters);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kResolutionBasedDecoderPriority);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kForceHardwareVideoDecoders);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kForceHardwareAudioDecoders);

#if BUILDFLAG(IS_ANDROID)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kAllowNonSecureOverlays);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaControlsExpandGesture);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaDrmPersistentLicense);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaDrmPreprovisioning);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaDrmPreprovisioningAtStartup);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kCanPlayHls);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPictureInPictureAPI);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kHlsPlayer);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kRequestSystemAudioFocus);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseAudioLatencyFromHAL);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUsePooledSharedImageVideoProvider);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseRealColorSpaceForAndroidVideo);
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(USE_CHROMEOS_MEDIA_ACCELERATION)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kChromeOSHWVBREncoding);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseChromeOSDirectVideoDecoder);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kLimitConcurrentDecoderInstances);
#if defined(ARCH_CPU_ARM_FAMILY)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPreferLibYuvImageProcessor);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kPreferGLImageProcessor);
#endif  // defined(ARCH_CPU_ARM_FAMILY)
#if BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseAlternateVideoDecoderImplementation);
#endif  // BUILDFLAG(IS_CHROMEOS)
#endif  // BUILDFLAG(USE_CHROMEOS_MEDIA_ACCELERATION)

#if BUILDFLAG(IS_WIN)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kDelayCopyNV12Textures);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kDirectShowGetPhotoState);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kIncludeIRCamerasInDeviceEnumeration);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationAV1Encoding);

// For feature check of kMediaFoundationH264CbpEncoding at runtime,
// please use IsMediaFoundationH264CbpEncodingEnabled() instead.
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationH264CbpEncoding);

MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationVP9Encoding);

MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationVideoCapture);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationVP8Decoding);

// For feature check of kMediaFoundationD3D11VideoCapture at runtime,
// please use IsMediaFoundationD3D11VideoCaptureEnabled() instead.
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationD3D11VideoCapture);

MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationClearPlayback);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kAllowMediaFoundationFrameServerMode);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kWasapiRawAudioCapture);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseFakeAudioCaptureTimestamps);
MEDIA_EXPORT BASE_DECLARE_FEATURE(kD3D11Vp9kSVCHWDecoding);

// Strategy affecting how Media Foundation Renderer determines its rendering
// mode when used with clear video media. This strategy does not impact
// protected media which must always use Direct Composition mode.
enum class MediaFoundationClearRenderingStrategy {
  // The renderer will operate in Direct Composition mode (e.g. windowless
  // swapchain).
  kDirectComposition,
  // The renderer will operate in Frame Server mode.
  kFrameServer,
  // The renderer is allowed to switch between Direct Composition & Frame Server
  // mode at its discretion.
  kDynamic,
};

// Under this feature, a given MediaFoundationClearRenderingStrategy param is
// used by the Media Foundation Renderer for Clear content scenarios.
MEDIA_EXPORT BASE_DECLARE_FEATURE(kMediaFoundationClearRendering);
MEDIA_EXPORT extern const base::FeatureParam<
    MediaFoundationClearRenderingStrategy>
    kMediaFoundationClearRenderingStrategyParam;
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kDeprecateLowUsageCodecs);
#endif

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseOutOfProcessVideoDecoding);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
MEDIA_EXPORT BASE_DECLARE_FEATURE(kUseOutOfProcessVideoEncoding);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

// Based on a |command_line| and the current platform, returns the effective
// autoplay policy. In other words, it will take into account the default policy
// if none is specified via the command line and options passed for testing.
// Returns one of the possible autoplay policy switches from the
// switches::autoplay namespace.
MEDIA_EXPORT std::string GetEffectiveAutoplayPolicy(
    const base::CommandLine& command_line);

MEDIA_EXPORT bool IsChromeWideEchoCancellationEnabled();
MEDIA_EXPORT bool IsHardwareSecureDecryptionEnabled();
MEDIA_EXPORT bool IsVideoCaptureAcceleratedJpegDecodingEnabled();

#if BUILDFLAG(IS_WIN)
MEDIA_EXPORT bool IsMediaFoundationH264CbpEncodingEnabled();
MEDIA_EXPORT bool IsMediaFoundationD3D11VideoCaptureEnabled();
#endif

enum class kCrosGlobalMediaControlsPinOptions {
  kPin,
  kNotPin,
  kHeuristic,
};

// Feature param used to force default pin/unpin for global media controls in
// CrOS.
MEDIA_EXPORT extern const base::FeatureParam<kCrosGlobalMediaControlsPinOptions>
    kCrosGlobalMediaControlsPinParam;

}  // namespace media

#endif  // MEDIA_BASE_MEDIA_SWITCHES_H_
