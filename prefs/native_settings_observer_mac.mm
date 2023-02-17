// Copyright (c) 2016 Vivaldi Technologies. All Rights Reserverd.

#include "prefs/native_settings_observer_mac.h"

#import <CoreFoundation/CoreFoundation.h>

#include "components/prefs/pref_service.h"
#include "prefs/native_settings_helper_mac.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"
#include "vivaldi/prefs/vivaldi_gen_pref_enums.h"

namespace vivaldi {

// static
NativeSettingsObserver* NativeSettingsObserver::Create(Profile* profile) {
  return new NativeSettingsObserverMac(profile);
}

void AquaColorChanged(CFNotificationCenterRef center,
                      void* observer,
                      CFStringRef name,
                      const void* object,
                      CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)->SetPref(
      vivaldiprefs::kSystemMacAquaColorVariant,
      getAquaColor());
}

void NoRedisplayChanged(CFNotificationCenterRef center,
                        void* observer,
                        CFStringRef name,
                        const void* object,
                        CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)
      ->SetPref(vivaldiprefs::kSystemMacActionOnDoubleClick,
        getActionOnDoubleClick());
}

void SwipeDirectionChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)
      ->SetPref(vivaldiprefs::kSystemMacSwipeScrollDirection,
        getSwipeDirection());
}

void SystemDarkModeChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)->SetPref(
      vivaldiprefs::kSystemDesktopThemeColor, getSystemDarkMode());
}

void KeyboardUIModeChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)->SetPref(
      vivaldiprefs::kSystemMacKeyboardUiMode, getKeyboardUIMode());
}

void ColorPreferencesChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)
    ->SetPref(vivaldiprefs::kSystemAccentColor, getSystemAccentColor());

  reinterpret_cast<NativeSettingsObserver*>(observer)
    ->SetPref(vivaldiprefs::kSystemHighlightColor, getSystemHighlightColor());
}

void MenubarSettingChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)
      ->SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
        getMenubarVisibleInFullscreen());
  reinterpret_cast<NativeSettingsObserver*>(observer)
      ->SetPref(vivaldiprefs::kSystemMacHideMenubar,
        getHideMenubar());
}

NativeSettingsObserverMac::NativeSettingsObserverMac(Profile* profile)
    : NativeSettingsObserver(profile) {

  // Initialize, in case the values are changed while Vivaldi is not running.
  AquaColorChanged(CFNotificationCenterGetDistributedCenter(), this,
    CFSTR("AppleAquaColorVariantChanged"), nullptr, nullptr);

  SwipeDirectionChanged(CFNotificationCenterGetDistributedCenter(), this,
    CFSTR("SwipeScrollDirectionDidChangeNotification"), nullptr, nullptr);

  NoRedisplayChanged(CFNotificationCenterGetDistributedCenter(), this,
    CFSTR("AppleNoRedisplayAppearancePreferenceChanged"), nullptr, nullptr);

  SystemDarkModeChanged(CFNotificationCenterGetDistributedCenter(), this,
    CFSTR("AppleInterfaceThemeChangedNotification"), nullptr, nullptr);

  SystemDarkModeChanged(CFNotificationCenterGetDistributedCenter(), this,
    CFSTR("KeyboardUIModeDidChangeNotification"), nullptr, nullptr);

  if (@available(macOS 10.14, *)) {
    SystemDarkModeChanged(CFNotificationCenterGetDistributedCenter(), this,
      CFSTR("AppleColorPreferencesChangedNotification"), nullptr, nullptr);
  }

  if (@available(macos 12.0.1, *)) {
    MenubarSettingChanged(CFNotificationCenterGetLocalCenter(), this,
      CFSTR("NSApplicationDidChangeSafeVisibleFrameNotification"), nullptr,
      nullptr);
  }

  // NOTE(tomas@vivaldi.com): fix for VB-39486
  CFNotificationCenterRemoveEveryObserver(
      CFNotificationCenterGetDistributedCenter(), this);


  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, AquaColorChanged,
      CFSTR("AppleAquaColorVariantChanged"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this,
      SwipeDirectionChanged, CFSTR("SwipeScrollDirectionDidChangeNotification"),
      NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, NoRedisplayChanged,
      CFSTR("AppleNoRedisplayAppearancePreferenceChanged"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, SystemDarkModeChanged,
      CFSTR("AppleInterfaceThemeChangedNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, KeyboardUIModeChanged,
      CFSTR("KeyboardUIModeDidChangeNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
  if (@available(macOS 10.14, *)) {
    CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, ColorPreferencesChanged,
      CFSTR("AppleColorPreferencesChangedNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
  }
  if (@available(macos 12.0.1, *)) {
    CFNotificationCenterAddObserver(
      CFNotificationCenterGetLocalCenter(), this, MenubarSettingChanged,
      CFSTR("NSApplicationDidChangeSafeVisibleFrameNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
  }
}

NativeSettingsObserverMac::~NativeSettingsObserverMac() {
  CFNotificationCenterRemoveEveryObserver(
      CFNotificationCenterGetDistributedCenter(), this);
}

}  // namespace vivaldi
