// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved.

// This is the cross platform code for Razer Chroma, it will call the
// platform specific parts.

#ifndef COMPONENTS_LIGHTS_RAZER_CHROMA_HANDLER_H_
#define COMPONENTS_LIGHTS_RAZER_CHROMA_HANDLER_H_

#include <vector>
#include "third_party/skia/include/core/SkColor.h"

typedef std::vector<SkColor> RazerChromaColors;
class PrefService;

class RazerChromaPlatformDriver {
 public:
  explicit RazerChromaPlatformDriver(PrefService* pref_service) {}
  virtual ~RazerChromaPlatformDriver() = default;

  // Initialize the platform layer, return false if Razer Chroma is
  // is not available or it could otherwise not initialize.
  virtual bool Initialize() = 0;

  // Called before exiting to allow the driver to clean up used
  // resources.
  virtual void Shutdown() = 0;

  // Sets the given colors for the configured devices.
  virtual void SetColors(RazerChromaColors& colors) = 0;

  // Implemented by the platform to initialize the Razer api, if
  // available.
  static RazerChromaPlatformDriver* CreateRazerChromaPlatformDriver(
      PrefService* pref_service);
};

class RazerChromaHandler {
 public:
  explicit RazerChromaHandler(PrefService* pref_service);
  ~RazerChromaHandler();

  // Initialize the Razer Chroma support and the underlying platform
  // layer.  Returns false on errors.
  bool Initialize();
  void Shutdown();

  bool IsAvailable() { return platform_driver_ != nullptr; }

  void SetColors(RazerChromaColors& colors);

 private:
  bool initialized_ = false;
  std::unique_ptr<RazerChromaPlatformDriver> platform_driver_;
};

#endif  // COMPONENTS_LIGHTS_RAZER_CHROMA_HANDLER_H_
