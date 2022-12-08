// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/speech/tts_external_platform_delegate_impl_lacros.h"

#include "base/no_destructor.h"
#include "chrome/browser/speech/tts_client_lacros.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/tts_controller.h"
#include "content/public/browser/tts_utterance.h"
#include "url/gurl.h"

// static
ExternalPlatformDelegateImplLacros*
ExternalPlatformDelegateImplLacros::GetInstance() {
  static base::NoDestructor<ExternalPlatformDelegateImplLacros>
      external_delegate;
  return external_delegate.get();
}

ExternalPlatformDelegateImplLacros::ExternalPlatformDelegateImplLacros() =
    default;

ExternalPlatformDelegateImplLacros::~ExternalPlatformDelegateImplLacros() =
    default;

void ExternalPlatformDelegateImplLacros::GetVoicesForBrowserContext(
    content::BrowserContext* browser_context,
    const GURL& source_url,
    std::vector<content::VoiceData>* out_voices) {
  TtsClientLacros::GetForBrowserContext(browser_context)
      ->GetAllVoices(out_voices);
}

void ExternalPlatformDelegateImplLacros::Enqueue(
    std::unique_ptr<content::TtsUtterance> utterance) {
  TtsClientLacros::GetForBrowserContext(utterance->GetBrowserContext())
      ->SpeakOrEnqueue(std::move(utterance));
}
