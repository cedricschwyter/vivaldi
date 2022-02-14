// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved.

#include "ui/lights/razer_chroma_handler.h"
#include "base/logging.h"

#ifndef OS_WIN
// static
RazerChromaPlatformDriver*
RazerChromaPlatformDriver::CreateRazerChromaPlatformDriver(
    PrefService* pref_service) {
  // Only Windows has a Chroma SDK.
  return nullptr;
}
#endif  // OS_WIN

RazerChromaHandler::RazerChromaHandler(PrefService* pref_service) {
  platform_driver_.reset(
      RazerChromaPlatformDriver::CreateRazerChromaPlatformDriver(pref_service));
}

RazerChromaHandler::~RazerChromaHandler() {

}

bool RazerChromaHandler::Initialize() {
  if (initialized_) {
    NOTREACHED();
    return false;
  }
  if (platform_driver_) {
    initialized_ = platform_driver_->Initialize();
  }
  return initialized_;
}

void RazerChromaHandler::Shutdown() {
  platform_driver_->Shutdown();
}

void RazerChromaHandler::SetColors(RazerChromaColors& colors) {
  if (!initialized_) {
    NOTREACHED();
    return;
  }
  DCHECK(platform_driver_);

  platform_driver_->SetColors(colors);
}
