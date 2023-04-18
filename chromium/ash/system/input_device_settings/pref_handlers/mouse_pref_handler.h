// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_INPUT_DEVICE_SETTINGS_PREF_HANDLERS_MOUSE_PREF_HANDLER_H_
#define ASH_SYSTEM_INPUT_DEVICE_SETTINGS_PREF_HANDLERS_MOUSE_PREF_HANDLER_H_

#include "ash/ash_export.h"
#include "ash/public/mojom/input_device_settings.mojom-forward.h"

class PrefService;

namespace ash {

// Handles reading and updating prefs that store mouse settings.
class ASH_EXPORT MousePrefHandler {
 public:
  // Initializes device settings in prefs and update the `settings` member of
  // the `mojom::Mouse` object.
  virtual void InitializeMouseSettings(PrefService* pref_service,
                                       mojom::Mouse* mouse) = 0;

  // Updates device settings stored in prefs to match the values in
  // `mouse.settings`.
  virtual void UpdateMouseSettings(PrefService* pref_service,
                                   const mojom::Mouse& mouse) = 0;

 protected:
  virtual ~MousePrefHandler() = default;
};

}  // namespace ash

#endif  // ASH_SYSTEM_INPUT_DEVICE_SETTINGS_PREF_HANDLERS_MOUSE_PREF_HANDLER_H_
