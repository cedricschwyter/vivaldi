// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines features for media/webrtc.

#ifndef MEDIA_WEBRTC_WEBRTC_FEATURES_H_
#define MEDIA_WEBRTC_WEBRTC_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace features {

COMPONENT_EXPORT(MEDIA_WEBRTC)
BASE_DECLARE_FEATURE(kWebRtcAllowWgcDesktopCapturer);

COMPONENT_EXPORT(MEDIA_WEBRTC)
BASE_DECLARE_FEATURE(kWebRtcAllow48kHzProcessingOnArm);

COMPONENT_EXPORT(MEDIA_WEBRTC) BASE_DECLARE_FEATURE(kWebRtcHybridAgc);

COMPONENT_EXPORT(MEDIA_WEBRTC)
BASE_DECLARE_FEATURE(kWebRtcAnalogAgcClippingControl);

COMPONENT_EXPORT(MEDIA_WEBRTC)
BASE_DECLARE_FEATURE(kWebRtcAnalogAgcStartupMinVolume);

}  // namespace features

#endif  // MEDIA_WEBRTC_WEBRTC_FEATURES_H_
