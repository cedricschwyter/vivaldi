// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FEATURES_H_
#define DEVICE_FIDO_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"

namespace url {
class Origin;
}

namespace device {

#if BUILDFLAG(IS_WIN)
// Controls whether on Windows, U2F/CTAP2 requests are forwarded to the
// native WebAuthentication API, where available.
COMPONENT_EXPORT(DEVICE_FIDO) BASE_DECLARE_FEATURE(kWebAuthUseNativeWinApi);
#endif  // BUILDFLAG(IS_WIN)

// Support the caBLE extension in assertion requests from any origin.
COMPONENT_EXPORT(DEVICE_FIDO)
BASE_DECLARE_FEATURE(kWebAuthCableExtensionAnywhere);

#if BUILDFLAG(IS_CHROMEOS)
// Enable a ChromeOS platform authenticator
COMPONENT_EXPORT(DEVICE_FIDO)
BASE_DECLARE_FEATURE(kWebAuthCrosPlatformAuthenticator);
#endif  // BUILDFLAG(IS_CHROMEOS)

COMPONENT_EXPORT(DEVICE_FIDO) BASE_DECLARE_FEATURE(kU2fPermissionPrompt);

// Feature flag for the Google-internal
// `WebAuthenticationAllowGoogleCorpRemoteRequestProxying` enterprise policy.
COMPONENT_EXPORT(DEVICE_FIDO)
BASE_DECLARE_FEATURE(kWebAuthnGoogleCorpRemoteDesktopClientPrivilege);

// Enable some experimental UI changes
COMPONENT_EXPORT(DEVICE_FIDO) BASE_DECLARE_FEATURE(kWebAuthPasskeysUI);

// Reshuffle WebAuthn request UI to put account selection for discoverable
// credentials on platform authenticators first, where applicable.
COMPONENT_EXPORT(DEVICE_FIDO)
BASE_DECLARE_FEATURE(kWebAuthnNewDiscoverableCredentialsUi);

// Don't send empty displayName values to security keys when creating
// credentials.
BASE_DECLARE_FEATURE(kWebAuthnNoEmptyDisplayNameCBOR);

// Include an indication for non-discoverable makeCredential calls in caBLE QR
// codes.
BASE_DECLARE_FEATURE(kWebAuthnNonDiscoverableMakeCredentialQRFlag);

}  // namespace device

#endif  // DEVICE_FIDO_FEATURES_H_
