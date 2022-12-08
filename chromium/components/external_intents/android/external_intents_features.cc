// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/external_intents/android/external_intents_features.h"

#include <jni.h>
#include <stddef.h>
#include <string>

#include "base/android/jni_string.h"
#include "base/notreached.h"
#include "components/external_intents/android/jni_headers/ExternalIntentsFeatures_jni.h"

namespace external_intents {

namespace {

// Array of features exposed through the Java ExternalIntentsFeatures API.
const base::Feature* kFeaturesExposedToJava[] = {
    &kAutofillAssistantGoogleInitiatorOriginCheck,
    &kBlockExternalFormSubmitWithoutGesture, &kExternalNavigationDebugLogs};

}  // namespace

// Alphabetical:

// Uses the initiator origin to check whether a navigation was started from a
// Google domain.
BASE_FEATURE(kAutofillAssistantGoogleInitiatorOriginCheck,
             "AutofillAssistantGoogleInitiatorOriginCheck",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Kill switch for blocking form submissions redirecting to apps without an
// associated user gesture.
BASE_FEATURE(kBlockExternalFormSubmitWithoutGesture,
             "BlockExternalFormSubmitWithoutGesture",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kExternalNavigationDebugLogs,
             "ExternalNavigationDebugLogs",
             base::FEATURE_DISABLED_BY_DEFAULT);

static jlong JNI_ExternalIntentsFeatures_GetFeature(JNIEnv* env, jint ordinal) {
  return reinterpret_cast<jlong>(kFeaturesExposedToJava[ordinal]);
}

}  // namespace external_intents
