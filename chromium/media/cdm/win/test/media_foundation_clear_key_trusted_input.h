// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_WIN_TEST_MEDIA_FOUNDATION_CLEAR_KEY_TRUSTED_INPUT_H_
#define MEDIA_CDM_WIN_TEST_MEDIA_FOUNDATION_CLEAR_KEY_TRUSTED_INPUT_H_

#include <mfidl.h>
#include <wrl/implements.h>

namespace media {

class MediaFoundationClearKeyTrustedInput final
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IMFTrustedInput,
          Microsoft::WRL::FtmBase> {
 public:
  MediaFoundationClearKeyTrustedInput();
  ~MediaFoundationClearKeyTrustedInput() override;
  MediaFoundationClearKeyTrustedInput(
      const MediaFoundationClearKeyTrustedInput&) = delete;
  MediaFoundationClearKeyTrustedInput& operator=(
      const MediaFoundationClearKeyTrustedInput&) = delete;

  HRESULT RuntimeClassInitialize();

  // IMFTrustedInput
  STDMETHODIMP GetInputTrustAuthority(
      _In_ DWORD stream_id,
      _In_ REFIID riid,
      _COM_Outptr_ IUnknown** authority) override;
};

}  // namespace media

#endif  // MEDIA_CDM_WIN_TEST_MEDIA_FOUNDATION_CLEAR_KEY_TRUSTED_INPUT_H_
