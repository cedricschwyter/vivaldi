// Copyright (c) 2016-2018 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/tabs/tabs_private_api.h"

#include <math.h>
#include <memory>
#include <utility>
#include <vector>

#include "app/vivaldi_apptools.h"
#include "app/vivaldi_constants.h"
#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "browser/vivaldi_browser_finder.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/permissions/permission_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/zoom/chrome_zoom_level_prefs.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/common/extensions/command.h"
#include "components/prefs/pref_service.h"
#include "components/zoom/zoom_controller.h"
#include "content/browser/renderer_host/render_view_host_delegate_view.h"
#include "content/browser/renderer_host/render_widget_host_view_child_frame.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/drop_data.h"
#include "extensions/api/access_keys/access_keys_api.h"
#include "extensions/api/extension_action_utils/extension_action_utils_api.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/schema/tabs_private.h"
#include "prefs/vivaldi_gen_prefs.h"
#include "prefs/vivaldi_pref_names.h"
#include "prefs/vivaldi_tab_zoom_pref.h"
#include "renderer/vivaldi_render_messages.h"
#include "third_party/blink/renderer/platform/keyboard_codes.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/content/vivaldi_event_hooks.h"
#include "ui/content/vivaldi_tab_check.h"
#include "ui/display/screen.h"
#include "ui/display/win/dpi.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/views/drag_utils.h"
#if defined(OS_WIN)
#include "ui/display/win/screen_win.h"
#endif  // defined(OS_WIN)
#include "ui/vivaldi_browser_window.h"
#include "ui/vivaldi_ui_utils.h"
#include "ui/strings/grit/ui_strings.h"

using content::WebContents;

namespace extensions {

const int& VivaldiPrivateTabObserver::kUserDataKey =
    VivaldiTabCheck::kVivaldiTabObserverContextKey;

namespace tabs_private = vivaldi::tabs_private;

struct MouseGestures {
  // To avoid depending on platform's focus policy store the id of the window
  // where the gesture was initiated and send the gesture events towards it and
  // not to the focused window, see VB-47721. Similarly, pass the initial
  // pointer coordinates relative to root to apply the gesture to the tab over
  // which the gesture has started, see VB-48232.
  int window_id = 0;
  blink::WebFloatPoint initial_client_pos;

  // Gesture started with the Alt key
  bool with_alt = false;

  bool recording = false;
  float last_x = 0.0f;
  float last_y = 0.0f;
  float min_move_squared = 0.0f;

  // The string of uniqe gesture directions that is send to JS.
  std::string directions;
  int last_direction = -1;
};

struct WheelGestures {
  bool active = 0;
  int window_id = 0;
};

struct RockerGestures {
  bool eat_next_left_mouseup = false;
  bool eat_next_right_mouseup = false;
};

class TabsPrivateAPIPrivate : public EventRouter::Observer,
                              public TabStripModelObserver {
 public:
  explicit TabsPrivateAPIPrivate(content::BrowserContext* context);
  ~TabsPrivateAPIPrivate() override;

  // Helper to actually dispatch an event to extension listeners.
  void DispatchEvent(events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args);

  // EventRouter::Observer implementation.
  void OnListenerAdded(const EventListenerInfo& details) override;

  // TabStripModelObserver implementation
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  bool HandleKeyboardMouseGesture(const content::NativeWebKeyboardEvent& event);

  Profile* const profile_;
  std::unique_ptr<MouseGestures> mouse_gestures_;
  WheelGestures wheel_gestures_;
  RockerGestures rocker_gestures_;

  DISALLOW_COPY_AND_ASSIGN(TabsPrivateAPIPrivate);
};

class VivaldiEventHooksImpl : public VivaldiEventHooks {
 public:
  VivaldiEventHooksImpl(WebContents* web_contents)
      : web_contents_(web_contents) {}

  // VivaldiEventHooks implementation.
  bool HandleMouseEvent(content::RenderWidgetHostViewBase* root_view,
                        const blink::WebMouseEvent& event) override;

  bool HandleWheelEvent(content::RenderWidgetHostViewBase* root_view,
                        const blink::WebMouseWheelEvent& event,
                        const ui::LatencyInfo& latency) override;

  bool ShouldCopyWheelEventToRoot(
      content::RenderWidgetHostViewBase* view,
      const blink::WebMouseWheelEvent& event) override;

  bool HandleDragEnd(blink::WebDragOperation operation,
                     bool cancelled,
                     int screen_x,
                     int screen_y) override;

 private:
  TabsPrivateAPIPrivate* GetTabsAPIPriv() {
    DCHECK(::vivaldi::IsVivaldiRunning());
    TabsPrivateAPI* api =
        TabsPrivateAPI::FromBrowserContext(web_contents_->GetBrowserContext());
    DCHECK(api);
    if (!api)
      return nullptr;
    return api->priv_.get();
  }

   WebContents* const web_contents_;
};

TabsPrivateAPIPrivate::TabsPrivateAPIPrivate(
    content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)) {
  EventRouter::Get(profile_)->RegisterObserver(
      this, tabs_private::OnDragEnd::kEventName);
}

TabsPrivateAPIPrivate::~TabsPrivateAPIPrivate() {
}

TabsPrivateAPI::TabsPrivateAPI(content::BrowserContext* context)
    : priv_(std::make_unique<TabsPrivateAPIPrivate>(context)) {
}

TabsPrivateAPI::~TabsPrivateAPI() {}

TabStripModelObserver* TabsPrivateAPI::GetTabStripModelObserver() {
  return priv_.get();
}

void TabsPrivateAPI::Shutdown() {
  EventRouter::Get(priv_->profile_)->UnregisterObserver(priv_.get());
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<TabsPrivateAPI> >::
    DestructorAtExit g_factory_tabs = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<TabsPrivateAPI>*
TabsPrivateAPI::GetFactoryInstance() {
  return g_factory_tabs.Pointer();
}

// static
TabsPrivateAPI* TabsPrivateAPI::FromBrowserContext(
    content::BrowserContext* browser_context) {
  return GetFactoryInstance()->Get(browser_context);
}

// static
void TabsPrivateAPI::SetupWebContents(content::WebContents* web_contents) {
  DCHECK(!web_contents->GetUserData(VivaldiEventHooks::UserDataKey()));
  web_contents->SetUserData(
      VivaldiEventHooks::UserDataKey(),
      std::make_unique<VivaldiEventHooksImpl>(web_contents));
}

void TabsPrivateAPIPrivate::OnListenerAdded(const EventListenerInfo& details) {
  EventRouter::Get(profile_)->UnregisterObserver(this);
}

namespace {
static tabs_private::MediaType ConvertTabAlertState(TabAlertState status) {
  switch (status) {
  case TabAlertState::NONE:
    return tabs_private::MediaType::MEDIA_TYPE_EMPTY;
  case TabAlertState::MEDIA_RECORDING:
    return tabs_private::MediaType::MEDIA_TYPE_RECORDING;
  case TabAlertState::TAB_CAPTURING:
    return tabs_private::MediaType::MEDIA_TYPE_CAPTURING;
  case TabAlertState::AUDIO_PLAYING:
    return tabs_private::MediaType::MEDIA_TYPE_PLAYING;
  case TabAlertState::AUDIO_MUTING:
    return tabs_private::MediaType::MEDIA_TYPE_MUTING;
  case TabAlertState::BLUETOOTH_CONNECTED:
    return tabs_private::MediaType::MEDIA_TYPE_BLUETOOTH;
  case TabAlertState::USB_CONNECTED:
    return tabs_private::MediaType::MEDIA_TYPE_USB;
  case TabAlertState::PIP_PLAYING:
    return tabs_private::MediaType::MEDIA_TYPE_PIP;
  case TabAlertState::DESKTOP_CAPTURING:
    return tabs_private::MediaType::MEDIA_TYPE_DESKTOP_CAPTURING;
  case TabAlertState::VR_PRESENTING_IN_HEADSET:
    return tabs_private::MediaType::MEDIA_TYPE_VR_PRESENTING_IN_HEADSET;
  }
  NOTREACHED() << "Unknown TabAlertState Status.";
  return tabs_private::MediaType::MEDIA_TYPE_NONE;
}
}  // namespace

void TabsPrivateAPIPrivate::TabChangedAt(content::WebContents* contents,
                                         int index,
                                         TabChangeType change_type) {
  tabs_private::MediaType type =
      ConvertTabAlertState(chrome::GetTabAlertStateForContents(contents));

  std::unique_ptr<base::ListValue> args =
      tabs_private::OnMediaStateChanged::Create(
          extensions::ExtensionTabUtil::GetTabId(contents), type);

  VivaldiPrivateTabObserver::BroadcastEvent(
      tabs_private::OnMediaStateChanged::kEventName, std::move(args),
      profile_);
}

namespace {

#if defined(OS_MACOSX)

base::string16 KeyCodeToName(ui::KeyboardCode key_code) {
  int string_id = 0;
  switch (key_code) {
    case ui::VKEY_TAB:
      string_id = IDS_APP_TAB_KEY;
      break;
    case ui::VKEY_RETURN:
      string_id = IDS_APP_ENTER_KEY;
      break;
    case ui::VKEY_SPACE:
      string_id = IDS_APP_SPACE_KEY;
      break;
    case ui::VKEY_PRIOR:
      string_id = IDS_APP_PAGEUP_KEY;
      break;
    case ui::VKEY_NEXT:
      string_id = IDS_APP_PAGEDOWN_KEY;
      break;
    case ui::VKEY_END:
      string_id = IDS_APP_END_KEY;
      break;
    case ui::VKEY_HOME:
      string_id = IDS_APP_HOME_KEY;
      break;
    case ui::VKEY_INSERT:
      string_id = IDS_APP_INSERT_KEY;
      break;
    case ui::VKEY_DELETE:
      string_id = IDS_APP_DELETE_KEY;
      break;
    case ui::VKEY_LEFT:
      string_id = IDS_APP_LEFT_ARROW_KEY;
      break;
    case ui::VKEY_RIGHT:
      string_id =IDS_APP_RIGHT_ARROW_KEY;
      break;
    case ui::VKEY_UP:
      string_id = IDS_APP_UP_ARROW_KEY;
      break;
    case ui::VKEY_DOWN:
      string_id = IDS_APP_DOWN_ARROW_KEY;
      break;
    case ui::VKEY_ESCAPE:
      string_id = IDS_APP_ESC_KEY;
      break;
    case ui::VKEY_BACK:
      string_id = IDS_APP_BACKSPACE_KEY;
      break;
    case ui::VKEY_F1:
      string_id = IDS_APP_F1_KEY;
      break;
    case ui::VKEY_F11:
      string_id = IDS_APP_F11_KEY;
      break;
    case ui::VKEY_OEM_COMMA:
      string_id = IDS_APP_COMMA_KEY;
      break;
    case ui::VKEY_OEM_PERIOD:
      string_id = IDS_APP_PERIOD_KEY;
      break;
    case ui::VKEY_MEDIA_NEXT_TRACK:
      string_id = IDS_APP_MEDIA_NEXT_TRACK_KEY;
      break;
    case ui::VKEY_MEDIA_PLAY_PAUSE:
      string_id = IDS_APP_MEDIA_PLAY_PAUSE_KEY;
      break;
    case ui::VKEY_MEDIA_PREV_TRACK:
      string_id = IDS_APP_MEDIA_PREV_TRACK_KEY;
      break;
    case ui::VKEY_MEDIA_STOP:
      string_id = IDS_APP_MEDIA_STOP_KEY;
      break;
    default:
      break;
  }
  return string_id ? l10n_util::GetStringUTF16(string_id) : base::string16();
}

#endif // OS_MACOSX

std::string ShortcutText(const content::NativeWebKeyboardEvent& event) {
  // We'd just use Accelerator::GetShortcutText to get the shortcut text but
  // it translates the modifiers when the system language is set to
  // non-English (since it's used for display). We can't match something
  // like 'Strg+G' however, so we do the modifiers manually.
  //
  // AcceleratorToString gets the shortcut text, but doesn't localize
  // like Accelerator::GetShortcutText() does, so it's suitable for us.
  // It doesn't handle all keys, however, and doesn't work with ctrl+alt
  // shortcuts so we're left with doing a little tweaking.
  ui::KeyboardCode key_code =
      static_cast<ui::KeyboardCode>(event.windows_key_code);
  ui::Accelerator accelerator =
      ui::Accelerator(key_code, 0, ui::Accelerator::KeyState::PRESSED);

  std::string shortcutText = "";
  if (event.GetModifiers() & content::NativeWebKeyboardEvent::kControlKey) {
    shortcutText += "Ctrl+";
  }
  if (event.GetModifiers() & content::NativeWebKeyboardEvent::kShiftKey) {
    shortcutText += "Shift+";
  }
  if (event.GetModifiers() & content::NativeWebKeyboardEvent::kAltKey) {
    shortcutText += "Alt+";
  }
  if (event.GetModifiers() & content::NativeWebKeyboardEvent::kMetaKey) {
    shortcutText += "Meta+";
  }

  std::string key_from_accelerator =
      extensions::Command::AcceleratorToString(accelerator);
  if (!key_from_accelerator.empty()) {
    shortcutText += key_from_accelerator;
  } else if (event.windows_key_code >= ui::VKEY_F1 &&
             event.windows_key_code <= ui::VKEY_F24) {
    char buf[4];
    base::snprintf(buf, 4, "F%d", event.windows_key_code - ui::VKEY_F1 + 1);
    shortcutText += buf;

  } else if (event.windows_key_code >= ui::VKEY_NUMPAD0 &&
             event.windows_key_code <= ui::VKEY_NUMPAD9) {
    char buf[8];
    base::snprintf(buf, 8, "Numpad%d",
                   event.windows_key_code - ui::VKEY_NUMPAD0);
    shortcutText += buf;

  // Enter is somehow not covered anywhere else.
  } else if (event.windows_key_code == ui::VKEY_RETURN) {
    shortcutText += "Enter";
  // GetShortcutText doesn't translate numbers and digits but
  // 'does' translate backspace
  } else if (event.windows_key_code == ui::VKEY_BACK) {
    shortcutText += "Backspace";
  // Escape was being translated as well in some languages
  } else if (event.windows_key_code == ui::VKEY_ESCAPE) {
    shortcutText += "Esc";
  } else {
#if defined(OS_MACOSX)
  // This is equivalent to js event.code and deals with a few
  // MacOS keyboard shortcuts like cmd+alt+n that fall through
  // in some languages, i.e. AcceleratorToString returns a blank.
  // Cmd+Alt shortcuts seem to be the only case where this fallback
  // is required.
  if (event.GetModifiers() & content::NativeWebKeyboardEvent::kAltKey &&
      event.GetModifiers() & content::NativeWebKeyboardEvent::kMetaKey) {
    shortcutText += ui::DomCodeToUsLayoutCharacter(
        static_cast<ui::DomCode>(event.dom_code), 0);
  } else {
    // With chrome 67 accelerator.GetShortcutText() will return Mac specific
    // symbols (like '⎋' for escape). All is private so we bypass that by
    // testing with KeyCodeToName first.
    base::string16 shortcut = KeyCodeToName(key_code);
    if (shortcut.empty())
      shortcut = accelerator.GetShortcutText();
    shortcutText += base::UTF16ToUTF8(shortcut);
  }
#else
    shortcutText += base::UTF16ToUTF8(accelerator.GetShortcutText());
#endif // OS_MACOSX
  }
  return shortcutText;
}

}  // namespace

bool TabsPrivateAPI::SendKeyboardShortcutEvent(
    const content::NativeWebKeyboardEvent& event,
    bool is_auto_repeat) {
  bool after_gesture = priv_->HandleKeyboardMouseGesture(event);
  bool down = false;
  bool send_keyboard_changed = false;
  if (event.GetType() == blink::WebInputEvent::kRawKeyDown) {
    down = true;
    send_keyboard_changed = true;
  } else if (event.GetType() == blink::WebInputEvent::kKeyUp) {
    send_keyboard_changed = true;
  }
  if (send_keyboard_changed) {
    std::unique_ptr<base::ListValue> args =
        vivaldi::tabs_private::OnKeyboardChanged::Create(
            down, event.GetModifiers(), event.windows_key_code, after_gesture);
    priv_->DispatchEvent(
        extensions::events::VIVALDI_EXTENSION_EVENT,
        extensions::vivaldi::tabs_private::OnKeyboardChanged::kEventName,
        std::move(args));
  }

  if (after_gesture)
    return true;

  // Return here as we don't allow AltGr keyboard shortcuts
  if (event.GetModifiers() & blink::WebInputEvent::kAltGrKey) {
    return false;
  }

  // Don't send if event contains only modifiers.
  int key_code = event.windows_key_code;
  if (key_code != ui::VKEY_CONTROL && key_code != ui::VKEY_SHIFT &&
      key_code != ui::VKEY_MENU) {
    if (event.GetType() == blink::WebInputEvent::kKeyUp) {
      return false;
    }
    std::string shortcut_text = ShortcutText(event);
    // If the event wasn't prevented we'll get a rawKeyDown event. In some
    // exceptional cases we'll never get that, so we let these through
    // unconditionally
    std::vector<std::string> exceptions =
        {"Up", "Down", "Shift+Delete", "Meta+Shift+V", "Esc"};
    bool is_exception = std::find(exceptions.begin(), exceptions.end(),
                                  shortcut_text) != exceptions.end();
    if (event.GetType() == blink::WebInputEvent::kRawKeyDown || is_exception) {
      std::unique_ptr<base::ListValue> args =
          vivaldi::tabs_private::OnKeyboardShortcut::Create(shortcut_text,
                                                            is_auto_repeat);
      priv_->DispatchEvent(
          extensions::events::VIVALDI_EXTENSION_EVENT,
          extensions::vivaldi::tabs_private::OnKeyboardShortcut::kEventName,
          std::move(args));
    }
  }
  return false;
}

namespace {

bool IsMouseGesturesEnabled(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(vivaldiprefs::kMouseGesturesEnabled);
}

bool IsRockerGesturesEnabled(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(
      vivaldiprefs::kMouseGesturesRockerGesturesEnabled);
}

bool IsMouseAltGesturesEnabled(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(
      vivaldiprefs::kMouseGesturesAltGesturesEnabled);
}

float GetMouseGestureStrokeTolerance(Profile* profile) {
  return static_cast<float>(profile->GetPrefs()->GetDouble(
      vivaldiprefs::kMouseGesturesStrokeTolerance));
}

bool IsLoneAltKeyPressed(int modifiers) {
  using blink::WebInputEvent;
  return (modifiers & WebInputEvent::kKeyModifiers) == WebInputEvent::kAltKey;
}

bool IsGestureMouseMove(const blink::WebMouseEvent& mouse_event) {
  using blink::WebInputEvent;
  using blink::WebMouseEvent;
  DCHECK(mouse_event.GetType() == WebInputEvent::kMouseMove);
  return (mouse_event.button == WebMouseEvent::Button::kRight &&
      !(mouse_event.GetModifiers() & WebInputEvent::kLeftButtonDown));
}

bool IsGestureAltMouseMove(const blink::WebMouseEvent& mouse_event) {
  DCHECK(mouse_event.GetType() == blink::WebInputEvent::kMouseMove);
  return IsLoneAltKeyPressed(mouse_event.GetModifiers());
}

blink::WebFloatPoint TransformToRootCoordinates(
    WebContents* web_contents, blink::WebFloatPoint p) {
  // Account for the zoom factor in the UI.
  zoom::ZoomController* zoom_controller =
    zoom::ZoomController::FromWebContents(web_contents);
  if (!zoom_controller)
    return p;
  double zoom_factor =
      content::ZoomLevelToZoomFactor(zoom_controller->GetZoomLevel());
  return blink::WebFloatPoint(p.x / zoom_factor, p.y / zoom_factor);
}

int GetWindowId(WebContents* web_contents) {
  Browser* browser = ::vivaldi::FindBrowserForEmbedderWebContents(web_contents);
  // browser is null for DevTools
  if (!browser)
    return 0;
  return browser->session_id().id();
}

void StartMouseGestureDetection(
    TabsPrivateAPIPrivate* priv, WebContents* web_contents,
    const blink::WebMouseEvent& mouse_event, bool with_alt) {
  DCHECK(!priv->mouse_gestures_);

  // Ignore any gesture after the wheel scroll with the Alt key or right button
  // pressed but before the key or button was released.
  if (priv->wheel_gestures_.active)
    return;
  if (with_alt) {
    if (!IsMouseAltGesturesEnabled(priv->profile_))
      return;
  } else {
    if (!IsMouseGesturesEnabled(priv->profile_))
      return;
  }

  int window_id = GetWindowId(web_contents);
  priv->mouse_gestures_ = std::make_unique<MouseGestures>();
  priv->mouse_gestures_->window_id = window_id;
  priv->mouse_gestures_->initial_client_pos =
      TransformToRootCoordinates(web_contents, mouse_event.PositionInWidget());
  priv->mouse_gestures_->with_alt = with_alt;
  priv->mouse_gestures_->last_x = mouse_event.PositionInScreen().x;
  priv->mouse_gestures_->last_y = mouse_event.PositionInScreen().y;

  std::unique_ptr<base::ListValue> args =
      vivaldi::tabs_private::OnMouseGestureDetection::Create(window_id);
  priv->DispatchEvent(
      extensions::events::VIVALDI_EXTENSION_EVENT,
      extensions::vivaldi::tabs_private::OnMouseGestureDetection::kEventName,
      std::move(args));
}

// The distance the mouse pointer has to travel in logical pixels before we
// start recording a gesture and eat the following pointer move events.
constexpr float MOUSE_GESTURE_THRESHOLD = 5.0f;

void HandleMouseGestureMove(const blink::WebMouseEvent& mouse_event,
                            Profile* profile,
                            MouseGestures& mouse_gestures,
                            bool& eat_event) {
  DCHECK(mouse_event.GetType() == blink::WebInputEvent::kMouseMove);
  float x = mouse_event.PositionInScreen().x;
  float y = mouse_event.PositionInScreen().y;

  // We do not need to account for HiDPI screens when comparing dx and dy with
  // threshould and tolerance. The values are in logical pixels adjusted from
  // real ones according to RenderWidgetHostViewBase::GetDeviceScaleFactor().
  float dx = x - mouse_gestures.last_x;
  float dy = y - mouse_gestures.last_y;
  if (!mouse_gestures.recording) {
    if (fabs(dx) < MOUSE_GESTURE_THRESHOLD
        && fabs(dy) < MOUSE_GESTURE_THRESHOLD) {
      return;
    }
    // The recording flag persists if we go under the threshold by moving the
    // mouse into the original location, which is expected.
    mouse_gestures.recording = true;

    // tolerance = movement in pixels before gesture move initiates.
    // For min_move we devide the preference by two as we require at least two
    // mouse move events in the same direction to acount as a gesture move.
    float tolerance = GetMouseGestureStrokeTolerance(profile);
    mouse_gestures.min_move_squared = (tolerance / 2.0f) * (tolerance / 2.0f);
  }

  // Do not propagate this mouse move as we are in the recording phase.
  eat_event = true;

  float sqDist = dx * dx + dy * dy;
  if (sqDist <= mouse_gestures.min_move_squared)
    return;

  mouse_gestures.last_x = x;
  mouse_gestures.last_y = y;

  // Detect if the direction of movement is into one of 4 sectors,
  // -45° .. 45°, 45° .. 135°, 135° .. 225°, 225° .. 315°.
  unsigned sector;
  if (fabs(dx) >= fabs(dy)) {
    sector = (dx >= 0) ? 0 : 2;
  } else {
    sector = (dy >= 0) ? 1 : 3;
  }

  // Encode the sector as '0' - '2' - '4' - '6' characters.
  char direction = static_cast<char>('0' + (sector * 2));

  // We only record moves that repeats at least twice with the same value
  // and for repeated values we only record the first one.
  if (mouse_gestures.last_direction != direction) {
    mouse_gestures.last_direction = direction;
  } else if (mouse_gestures.directions.size() == 0 ||
             mouse_gestures.directions.back() != direction) {
    mouse_gestures.directions.push_back(direction);
  }
}

void FinishMouseOrWheelGesture(TabsPrivateAPIPrivate* priv,
                               bool with_alt,
                               bool& after_gesture) {
  if (priv->wheel_gestures_.active) {
    DCHECK(!priv->mouse_gestures_);
    after_gesture = true;
    std::unique_ptr<base::ListValue> args =
        vivaldi::tabs_private::OnTabSwitchEnd::Create(
            priv->wheel_gestures_.window_id);
    priv->DispatchEvent(
        extensions::events::VIVALDI_EXTENSION_EVENT,
        extensions::vivaldi::tabs_private::OnTabSwitchEnd::kEventName,
        std::move(args));
    priv->wheel_gestures_.active = false;
    priv->wheel_gestures_.window_id = 0;
  }
  if (!priv->mouse_gestures_)
    return;

  // Alt gestures can only be finished with the keyboard and pure mouse gestures
  // can only be finished with the mouse.
  if (with_alt != priv->mouse_gestures_->with_alt)
    return;

  // Do not send a gesture event and eat the pointer/keyboard up when we got no
  // gesture moves. This allows context menu to work on pointer up when on a
  // touchpad fingers can easly move more then MOUSE_GESTURE_THRESHOLD pixels,
  // see VB-48846.
  if (!priv->mouse_gestures_->directions.empty()) {
    after_gesture = true;

    blink::WebFloatPoint p = priv->mouse_gestures_->initial_client_pos;
    std::unique_ptr<base::ListValue> args =
        vivaldi::tabs_private::OnMouseGesture::Create(
            priv->mouse_gestures_->window_id, p.x, p.y,
            priv->mouse_gestures_->directions);
    priv->DispatchEvent(
        extensions::events::VIVALDI_EXTENSION_EVENT,
        extensions::vivaldi::tabs_private::OnMouseGesture::kEventName,
        std::move(args));
  }
  priv->mouse_gestures_.reset();
}

void CheckMouseGesture(
    TabsPrivateAPIPrivate* priv, WebContents* web_contents,
    const blink::WebMouseEvent& mouse_event, bool& eat_event) {
  using blink::WebInputEvent;
  using blink::WebMouseEvent;
  using blink::WebMouseWheelEvent;

  // We should not have both wheel and mouse gestures running.
  DCHECK(!priv->wheel_gestures_.active || !priv->mouse_gestures_);
  switch (mouse_event.GetType()) {
    default:
      break;
    case WebInputEvent::kMouseDown: {
      if (!priv->mouse_gestures_) {
        if (mouse_event.button == WebMouseEvent::Button::kRight &&
            !(mouse_event.GetModifiers() & WebInputEvent::kLeftButtonDown)) {
          StartMouseGestureDetection(priv, web_contents, mouse_event, false);
        }
      }
      break;
    }
    case WebInputEvent::kMouseMove: {
      if (!priv->mouse_gestures_) {
        bool gesture = false;
        bool with_alt = false;
        if (IsGestureMouseMove(mouse_event)) {
          gesture = true;
        } else if (IsGestureAltMouseMove(mouse_event)) {
          gesture = true;
          with_alt = true;
        }
        if (gesture) {
          // Handle the right button pressed outside the window before entering
          // the window.
          StartMouseGestureDetection(priv, web_contents, mouse_event, with_alt);
        }
        break;
      }
      bool gesture = false;
      if (priv->mouse_gestures_->with_alt) {
        gesture = IsGestureAltMouseMove(mouse_event);
      } else {
        gesture = IsGestureMouseMove(mouse_event);
      }
      if (gesture) {
        HandleMouseGestureMove(mouse_event, priv->profile_,
                               *priv->mouse_gestures_, eat_event);
        break;
      }
      // This happens when the right mouse button is released outside of webview
      // or the alt key was released when the window lost input focus.
      priv->mouse_gestures_.reset();
      break;
    }
    case WebInputEvent::kMouseUp: {
      FinishMouseOrWheelGesture(priv, false, eat_event);
      break;
    }
  }
}

void CheckRockerGesture(TabsPrivateAPIPrivate* priv,
                        content::WebContents* web_contents,
                        const blink::WebMouseEvent& mouse_event,
                        bool& eat_event) {
  using blink::WebInputEvent;
  using blink::WebMouseEvent;
  if (mouse_event.GetType() == WebInputEvent::kMouseDown) {
    enum { ROCKER_NONE, ROCKER_LEFT, ROCKER_RIGHT } rocker_action = ROCKER_NONE;
    if (mouse_event.button == WebMouseEvent::Button::kLeft) {
      if (mouse_event.GetModifiers() & WebInputEvent::kRightButtonDown) {
        rocker_action = ROCKER_LEFT;
      } else {
        // The eat flags can be true if buttons were released outside of the
        // window.
        priv->rocker_gestures_.eat_next_right_mouseup = false;
      }
    } else if (mouse_event.button == WebMouseEvent::Button::kRight) {
      if (mouse_event.GetModifiers() & WebInputEvent::kLeftButtonDown) {
        rocker_action = ROCKER_RIGHT;
      } else {
        priv->rocker_gestures_.eat_next_left_mouseup = false;
      }
    }
    // Check if rocker gestures are enabled only after we detected them to avoid
    // preference checks on each mouse down.
    if (rocker_action != ROCKER_NONE &&
        IsRockerGesturesEnabled(priv->profile_)) {
      // We got a rocker gesture. Follow Opera's implementation and consume
      // the last event which is a mouse down from either the left or the
      // right button and consume both the future left and right mouse up to
      // prevent clicks, menus or similar page actions.
      eat_event = true;
      priv->rocker_gestures_.eat_next_left_mouseup = true;
      priv->rocker_gestures_.eat_next_right_mouseup = true;

      // Stop any mouse gesture if any.
      priv->mouse_gestures_.reset();
      bool is_left = (rocker_action == ROCKER_LEFT);

      // TODO(igor@vivaldi.com): This broadcats the event to all windows and
      // extensions forcing our JS code to check using async API if the current
      // frame is active. Find a way to send this only to Vivaldi JS is a
      // specific window.
      int window_id = GetWindowId(web_contents);
      std::unique_ptr<base::ListValue> args =
          vivaldi::tabs_private::OnRockerGesture::Create(window_id, is_left);
      priv->DispatchEvent(
          extensions::events::VIVALDI_EXTENSION_EVENT,
          extensions::vivaldi::tabs_private::OnRockerGesture::kEventName,
          std::move(args));
    }
  } else if (mouse_event.GetType() == WebInputEvent::kMouseUp) {
    if (priv->rocker_gestures_.eat_next_left_mouseup) {
      if (mouse_event.button == WebMouseEvent::Button::kLeft) {
        priv->rocker_gestures_.eat_next_left_mouseup = false;
        eat_event = true;
      } else if (!(mouse_event.GetModifiers() &
                   WebInputEvent::kLeftButtonDown)) {
        // Missing mouse up when mouse was released outside the window etc.
        priv->rocker_gestures_.eat_next_left_mouseup = false;
      }
    }
    if (priv->rocker_gestures_.eat_next_right_mouseup) {
      if (mouse_event.button == WebMouseEvent::Button::kRight) {
        priv->rocker_gestures_.eat_next_right_mouseup = false;
        eat_event = true;
      } else if (!(mouse_event.GetModifiers() &
                   WebInputEvent::kRightButtonDown)) {
        priv->rocker_gestures_.eat_next_right_mouseup = false;
      }
    }
  }
}

// Notify Vivaldi UI about clicks into webviews to properly track focused tabs
// and to dismiss our popup controls and other GUI elements that cover
// web views, see VB-48000.
//
// Current implementation sends the extension event for any click inside Vivaldi
// window including clicks into UI outside webviews. Chromium API for locating
// views from the point are exteremely heavy, see code in
// RenderWidgetHostInputEventRouter::FindViewAtLocation(), and it is simpler to
// filter out clicks outside the webviews in the handler for the extension event
// using document.elementFromPoint().
void CheckWebviewClick(TabsPrivateAPIPrivate* priv,
                       content::WebContents* web_contents,
                       const blink::WebMouseEvent& mouse_event) {
  using blink::WebInputEvent;
  using Button = blink::WebPointerProperties::Button;
  WebInputEvent::Type type = mouse_event.GetType();
  if (type != WebInputEvent::kMouseDown && type != WebInputEvent::kMouseUp)
    return;

  bool mousedown = (type == WebInputEvent::kMouseDown);
  int button = 0;
  if (mouse_event.button == Button::kMiddle) {
    button = 1;
  } else if (mouse_event.button == Button::kRight) {
    button = 2;
  }
  int window_id = GetWindowId(web_contents);
  blink::WebFloatPoint p =
    TransformToRootCoordinates(web_contents, mouse_event.PositionInWidget());
  std::unique_ptr<base::ListValue> args =
      vivaldi::tabs_private::OnWebviewClickCheck::Create(
          window_id, mousedown, button, p.x, p.y);
  priv->DispatchEvent(events::VIVALDI_EXTENSION_EVENT,
                      vivaldi::tabs_private::OnWebviewClickCheck::kEventName,
                      std::move(args));
}

}  // namespace

bool VivaldiEventHooksImpl::HandleMouseEvent(
    content::RenderWidgetHostViewBase* root_view,
    const blink::WebMouseEvent& event) {
  bool eat_event = false;
  if (TabsPrivateAPIPrivate* priv = GetTabsAPIPriv()) {
    // Rocker gestures take priority over any other mouse gestures.
    CheckRockerGesture(priv, web_contents_, event, eat_event);
    if (!eat_event) {
      CheckMouseGesture(priv, web_contents_, event, eat_event);
      if (!eat_event) {
        CheckWebviewClick(priv, web_contents_, event);
      }
    }
  }
  return eat_event;
}

bool VivaldiEventHooksImpl::HandleWheelEvent(
    content::RenderWidgetHostViewBase* root_view,
    const blink::WebMouseWheelEvent& wheel_event,
    const ui::LatencyInfo& latency) {
  using blink::WebInputEvent;
  using blink::WebMouseWheelEvent;
  DCHECK(::vivaldi::IsVivaldiRunning());

  int modifiers = wheel_event.GetModifiers();
  constexpr int left = WebInputEvent::kLeftButtonDown;
  constexpr int right = WebInputEvent::kRightButtonDown;
  bool only_right = (modifiers & (left | right)) == right;
  bool wheel_gesture_event = only_right || IsLoneAltKeyPressed(modifiers);
  if (!wheel_gesture_event)
    return false;

  TabsPrivateAPIPrivate* priv = GetTabsAPIPriv();
  if (!priv)
    return false;

  // We should not have both wheel and mouse gestures running.
  DCHECK(!priv->wheel_gestures_.active || !priv->mouse_gestures_);

  if (!priv->profile_->GetPrefs()->GetBoolean(
          vivaldiprefs::kMouseWheelTabSwitch))
    return false;

  if (!priv->wheel_gestures_.active) {
    // The event starts a new wheel gesture sequence canceling any mouse
    // gesture detection unless the wheel phase is:
    //
    // kPhaseEnded - with the inertial scrolling we can receive this with
    // modifiers indicating a pressed button after the user stopped
    // rotating the wheel and after the browser received the mouse up event.
    //
    // kPhaseCancelled - when the user presses touchpad with two fingers we
    // may receive kPhaseMayBegin with no modifiers, then kMouseDown with
    // kRightButtonDown then kPhaseCancelled with kRightButtonDown.
    constexpr int unwanted_phases =
        WebMouseWheelEvent::kPhaseEnded | WebMouseWheelEvent::kPhaseCancelled;
    if (!(wheel_event.phase & unwanted_phases)) {
      priv->mouse_gestures_.reset();
      priv->wheel_gestures_.active = true;
      priv->wheel_gestures_.window_id = GetWindowId(web_contents_);
    }
  }
  root_view->ProcessMouseWheelEvent(wheel_event, latency);
  return true;
}

bool VivaldiEventHooksImpl::ShouldCopyWheelEventToRoot(
    content::RenderWidgetHostViewBase* view,
    const blink::WebMouseWheelEvent& event) {
  using blink::WebInputEvent;
  using blink::WebMouseWheelEvent;
  DCHECK(view->IsRenderWidgetHostViewChildFrame());
  constexpr int zoom_modifier =
#if defined(OS_MACOSX)
      WebInputEvent::kMetaKey
#else
      WebInputEvent::kControlKey
#endif
      ;
  int modifiers = event.GetModifiers();
  if ((modifiers & WebInputEvent::kKeyModifiers) != zoom_modifier)
    return false;

  // PDF views implement their own zoom.
  if (view->IsRenderWidgetHostViewGuest())
    return false;

  TabsPrivateAPIPrivate* priv = GetTabsAPIPriv();
  return priv && priv->profile_->GetPrefs()->GetBoolean(
                     vivaldiprefs::kMouseWheelPageZoom);
}

bool TabsPrivateAPIPrivate::HandleKeyboardMouseGesture(
    const content::NativeWebKeyboardEvent& event) {
  bool after_gesture = false;
  // Check for Alt aka Menu release
  bool alt_up = event.GetType() == blink::WebInputEvent::kKeyUp &&
                event.windows_key_code == blink::VKEY_MENU;
  if (alt_up) {
    FinishMouseOrWheelGesture(this, true, after_gesture);
  }
  return after_gesture;
}

bool VivaldiEventHooksImpl::HandleDragEnd(blink::WebDragOperation operation,
                                          bool cancelled,
                                          int screen_x,
                                          int screen_y) {
  if (!::vivaldi::IsTabDragInProgress())
    return false;
  ::vivaldi::SetTabDragInProgress(false);

  TabsPrivateAPIPrivate* priv = GetTabsAPIPriv();
  if (!priv)
    return false;

  bool outside = ::vivaldi::ui_tools::IsOutsideAppWindow(screen_x, screen_y,
                                                         priv->profile_);
  if (!outside && operation == blink::WebDragOperation::kWebDragOperationNone) {
    // None of browser windows accepted the drag and we do not moving tabs out.
    cancelled = true;
  }

  std::unique_ptr<base::ListValue> args =
      vivaldi::tabs_private::OnDragEnd::Create(cancelled, outside, screen_x,
                                               screen_y);

  priv->DispatchEvent(extensions::events::VIVALDI_EXTENSION_EVENT,
                      vivaldi::tabs_private::OnDragEnd::kEventName,
                      std::move(args));

  return outside;
}

void TabsPrivateAPIPrivate::DispatchEvent(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  EventRouter* event_router = EventRouter::Get(profile_);
  if (!event_router)
    return;

  std::unique_ptr<Event> event(
      new Event(histogram_value, event_name, std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

/*static*/
void VivaldiPrivateTabObserver::BroadcastEvent(
    const std::string& eventname,
    std::unique_ptr<base::ListValue> args,
    content::BrowserContext* context) {
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::VIVALDI_EXTENSION_EVENT, eventname, std::move(args)));
  EventRouter* event_router = EventRouter::Get(context);
  if (event_router) {
    event_router->BroadcastEvent(std::move(event));
  }
}

VivaldiPrivateTabObserver::VivaldiPrivateTabObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents), weak_ptr_factory_(this) {
  auto* zoom_controller = zoom::ZoomController::FromWebContents(web_contents);
  if (zoom_controller) {
    zoom_controller->AddObserver(this);
  }
  prefs_registrar_.Init(
      Profile::FromBrowserContext(web_contents->GetBrowserContext())
          ->GetPrefs());

  prefs_registrar_.Add(vivaldiprefs::kWebpagesFocusTrap,
                       base::Bind(&VivaldiPrivateTabObserver::OnPrefsChanged,
                                  weak_ptr_factory_.GetWeakPtr()));
  prefs_registrar_.Add(vivaldiprefs::kWebpagesAccessKeys,
                       base::Bind(&VivaldiPrivateTabObserver::OnPrefsChanged,
                                  weak_ptr_factory_.GetWeakPtr()));
}

VivaldiPrivateTabObserver::~VivaldiPrivateTabObserver() {}

void VivaldiPrivateTabObserver::WebContentsDestroyed() {
}

void VivaldiPrivateTabObserver::OnPrefsChanged(const std::string& path) {
  if (path == vivaldiprefs::kWebpagesFocusTrap) {
    UpdateAllowTabCycleIntoUI();
    CommitSettings();
  } else if (path == vivaldiprefs::kWebpagesAccessKeys) {
    UpdateAllowAccessKeys();
    CommitSettings();
  }

}

void VivaldiPrivateTabObserver::BroadcastTabInfo() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());

  tabs_private::UpdateTabInfo info;
  info.show_images.reset(new bool(show_images()));
  info.load_from_cache_only.reset(new bool(load_from_cache_only()));
  info.enable_plugins.reset(new bool(enable_plugins()));
  info.mime_type.reset(new std::string(contents_mime_type()));
  int id = SessionTabHelper::IdForTab(web_contents()).id();

  std::unique_ptr<base::ListValue> args =
      tabs_private::OnTabUpdated::Create(id, info);
  BroadcastEvent(tabs_private::OnTabUpdated::kEventName, std::move(args),
                 profile);
}


const int kThemeColorBufferSize = 8;

void VivaldiPrivateTabObserver::DidChangeThemeColor(SkColor theme_color) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  char rgb_buffer[kThemeColorBufferSize];
  base::snprintf(rgb_buffer, kThemeColorBufferSize, "#%02x%02x%02x",
                 SkColorGetR(theme_color), SkColorGetG(theme_color),
                 SkColorGetB(theme_color));
  int tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents());
  std::unique_ptr<base::ListValue> args =
      tabs_private::OnThemeColorChanged::Create(tab_id, rgb_buffer);
  BroadcastEvent(tabs_private::OnThemeColorChanged::kEventName, std::move(args),
                 profile);
}

base::Value DictionaryToJSONString(const base::DictionaryValue& dict) {
  std::string json_string;
  base::JSONWriter::WriteWithOptions(dict, 0, &json_string);
  return base::Value(json_string);
}

void VivaldiPrivateTabObserver::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  if (::vivaldi::isTabZoomEnabled(web_contents())) {
    std::string ext = web_contents()->GetExtData();

    base::JSONParserOptions options = base::JSON_PARSE_RFC;
    std::unique_ptr<base::Value> json = base::JSONReader::Read(ext, options);
    base::DictionaryValue* dict = NULL;
    Profile* profile =
        Profile::FromBrowserContext(web_contents()->GetBrowserContext());
    content::HostZoomMap* host_zoom_map =
        content::HostZoomMap::GetDefaultForBrowserContext(profile);
    double default_zoom_level = host_zoom_map->GetDefaultZoomLevel();

    if (json && json->GetAsDictionary(&dict)) {
      if (dict) {
        if (dict->HasKey("vivaldi_tab_zoom")) {
          dict->GetDouble("vivaldi_tab_zoom", &tab_zoom_level_);
        } else {
          tab_zoom_level_ = default_zoom_level;
        }
      }
    } else {
      tab_zoom_level_ = default_zoom_level;
    }
  }

  SetShowImages(show_images_);
  SetLoadFromCacheOnly(load_from_cache_only_);
  SetEnablePlugins(enable_plugins_);
  UpdateAllowTabCycleIntoUI();
  UpdateAllowAccessKeys();
  CommitSettings();

  const GURL& site = render_view_host->GetSiteInstance()->GetSiteURL();
  std::string renderviewhost = site.host();
  if (::vivaldi::IsVivaldiApp(renderviewhost)) {
    auto* security_policy = content::ChildProcessSecurityPolicy::GetInstance();
    int process_id = render_view_host->GetProcess()->GetID();
    security_policy->GrantRequestScheme(process_id, url::kFileScheme);
    security_policy->GrantRequestScheme(process_id, content::kViewSourceScheme);
  }
}

void VivaldiPrivateTabObserver::SaveZoomLevelToExtData(double zoom_level) {
  std::string ext = web_contents()->GetExtData();

  base::JSONParserOptions options = base::JSON_PARSE_RFC;
  std::unique_ptr<base::Value> json = base::JSONReader::Read(ext, options);
  base::DictionaryValue* dict = NULL;
  if (json && json->GetAsDictionary(&dict)) {
    if (dict) {
      dict->SetDouble("vivaldi_tab_zoom", zoom_level);
      base::Value st = DictionaryToJSONString(*dict);
      web_contents()->SetExtData(st.GetString());
    }
  }
}

void VivaldiPrivateTabObserver::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  if (::vivaldi::isTabZoomEnabled(web_contents())) {
    int render_view_id = new_host->GetRoutingID();
    int process_id = new_host->GetProcess()->GetID();

    content::HostZoomMap* host_zoom_map_ =
        content::HostZoomMap::GetForWebContents(web_contents());

    host_zoom_map_->SetTemporaryZoomLevel(process_id, render_view_id,
                                          tab_zoom_level_);
  }

  // Set the setting on the new RenderViewHost too.
  SetShowImages(show_images_);
  SetLoadFromCacheOnly(load_from_cache_only_);
  SetEnablePlugins(enable_plugins_);
  UpdateAllowTabCycleIntoUI();
  UpdateAllowAccessKeys();
  CommitSettings();
}

void VivaldiPrivateTabObserver::UpdateAllowTabCycleIntoUI() {
  content::RendererPreferences* render_prefs =
    web_contents()->GetMutableRendererPrefs();
  DCHECK(render_prefs);
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());

  if (::vivaldi::IsVivaldiRunning()) {
    render_prefs->allow_tab_cycle_from_webpage_into_ui =
        !profile->GetPrefs()->GetBoolean(vivaldiprefs::kWebpagesFocusTrap);
  } else {
    // Avoid breaking tests.
    render_prefs->allow_tab_cycle_from_webpage_into_ui = true;
  }
}

void VivaldiPrivateTabObserver::UpdateAllowAccessKeys() {
  content::RendererPreferences* render_prefs =
    web_contents()->GetMutableRendererPrefs();
  DCHECK(render_prefs);
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  render_prefs->allow_access_keys =
      profile->GetPrefs()->GetBoolean(vivaldiprefs::kWebpagesAccessKeys);
}

void VivaldiPrivateTabObserver::SetShowImages(bool show_images) {
  show_images_ = show_images;

  content::RendererPreferences* render_prefs =
      web_contents()->GetMutableRendererPrefs();
  DCHECK(render_prefs);
  render_prefs->should_show_images = show_images;
}

void VivaldiPrivateTabObserver::SetLoadFromCacheOnly(
    bool load_from_cache_only) {
  load_from_cache_only_ = load_from_cache_only;

  content::RendererPreferences* render_prefs =
      web_contents()->GetMutableRendererPrefs();
  DCHECK(render_prefs);
  render_prefs->serve_resources_only_from_cache = load_from_cache_only;
}

void VivaldiPrivateTabObserver::SetEnablePlugins(bool enable_plugins) {
  enable_plugins_ = enable_plugins;
  content::RendererPreferences* render_prefs =
      web_contents()->GetMutableRendererPrefs();
  DCHECK(render_prefs);
  render_prefs->should_enable_plugin_content = enable_plugins;
}

void VivaldiPrivateTabObserver::CommitSettings() {
  content::RendererPreferences* render_prefs =
      web_contents()->GetMutableRendererPrefs();
  DCHECK(render_prefs);

  // We must update from system settings otherwise many settings would fallback
  // to default values when syncing below.
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  renderer_preferences_util::UpdateFromSystemSettings(render_prefs, profile);

  if (load_from_cache_only_ && enable_plugins_) {
    render_prefs->should_ask_plugin_content = true;
  } else {
    render_prefs->should_ask_plugin_content = false;
  }
  web_contents()->GetRenderViewHost()->SyncRendererPrefs();
}

void VivaldiPrivateTabObserver::OnZoomChanged(
    const zoom::ZoomController::ZoomChangedEventData& data) {
  content::WebContents* web_contents = data.web_contents;
  content::StoragePartition* current_partition =
      content::BrowserContext::GetStoragePartition(
          web_contents->GetBrowserContext(), web_contents->GetSiteInstance(),
          false);
  if (current_partition &&
      current_partition == content::BrowserContext::GetDefaultStoragePartition(
                               web_contents->GetBrowserContext()))
    SetZoomLevelForTab(data.new_zoom_level);
}

void VivaldiPrivateTabObserver::SetZoomLevelForTab(double level) {
  if (level != tab_zoom_level_) {
    tab_zoom_level_ = level;
    SaveZoomLevelToExtData(level);
  }
}

bool VivaldiPrivateTabObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(VivaldiPrivateTabObserver, message)
  IPC_MESSAGE_HANDLER(VivaldiViewHostMsg_RequestThumbnailForFrame_ACK,
                      OnRequestThumbnailForFrameResponse)
  IPC_MESSAGE_HANDLER(VivaldiViewHostMsg_GetAccessKeysForPage_ACK,
                      OnGetAccessKeysForPageResponse)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void VivaldiPrivateTabObserver::DocumentAvailableInMainFrame() {
  VivaldiPrivateTabObserver* tab_api =
      VivaldiPrivateTabObserver::FromWebContents(web_contents());
  DCHECK(tab_api);
  if (tab_api) {
    tab_api->SetContentsMimeType(web_contents()->GetContentsMimeType());
    tab_api->BroadcastTabInfo();
  }
}

void VivaldiPrivateTabObserver::CaptureTab(
    gfx::Size size,
    bool full_page,
    const CaptureTabDoneCallback& callback) {
  VivaldiViewMsg_RequestThumbnailForFrame_Params param;
  gfx::Rect rect = web_contents()->GetContainerBounds();

  param.callback_id = SessionTabHelper::IdForTab(web_contents()).id();
  if (full_page) {
    param.size = size;
  } else {
    param.size = rect.size();
  }
  capture_callback_ = callback;
  param.full_page = full_page;

  web_contents()->GetRenderViewHost()->Send(
      new VivaldiViewMsg_RequestThumbnailForFrame(
          web_contents()->GetRenderViewHost()->GetRoutingID(), param));
}

bool VivaldiPrivateTabObserver::IsCapturing() {
  return capture_callback_.is_null() ? false : true;
}

void VivaldiPrivateTabObserver::OnRequestThumbnailForFrameResponse(
    base::SharedMemoryHandle handle,
    const gfx::Size image_size,
    int callback_id,
    bool success) {
  if (!capture_callback_.is_null()) {
    capture_callback_.Run(handle, image_size, callback_id, success);
    capture_callback_.Reset();
  }
}

void VivaldiPrivateTabObserver::GetAccessKeys(
    content::WebContents* web_contents,
    AccessKeysCallback callback) {
  access_keys_callback_ = std::move(callback);
  web_contents->GetRenderViewHost()->Send(
      new VivaldiViewMsg_GetAccessKeysForPage(
          web_contents->GetRenderViewHost()->GetRoutingID()));
}

void VivaldiPrivateTabObserver::OnGetAccessKeysForPageResponse(
  std::vector<VivaldiViewMsg_AccessKeyDefinition> access_keys) {
  if (!access_keys_callback_.is_null()) {
    std::move(access_keys_callback_).Run(std::move(access_keys));
  }
}

void VivaldiPrivateTabObserver::AccessKeyAction(
    content::WebContents* web_contents,
    std::string access_key) {
  web_contents->GetRenderViewHost()->Send(new VivaldiViewMsg_AccessKeyAction(
      web_contents->GetRenderViewHost()->GetRoutingID(), access_key));
}

void VivaldiPrivateTabObserver::OnPermissionAccessed(
    ContentSettingsType content_settings_type, std::string origin,
    ContentSetting content_setting) {
  int tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents());

  std::string type_name = base::ToLowerASCII(
      PermissionUtil::GetPermissionString(content_settings_type));

  std::string setting;
  switch (content_setting) {
  case CONTENT_SETTING_ALLOW:
    setting = "allow";
    break;
  case CONTENT_SETTING_ASK:
    setting = "ask";
    break;
  case CONTENT_SETTING_BLOCK:
    setting = "block";
    break;
  case CONTENT_SETTING_DEFAULT:
  default:
    setting = "default";
    break;
  }

  std::unique_ptr<base::ListValue> args =
      vivaldi::tabs_private::OnPermissionAccessed::Create(tab_id, type_name,
                                                          origin, setting);

  Profile *profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  BroadcastEvent(tabs_private::OnPermissionAccessed::kEventName,
                 std::move(args), profile);
}

void VivaldiPrivateTabObserver::WebContentsDidDetach(
    const content::WebContents* embedder_contents) {
  int tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents());
  std::unique_ptr<base::ListValue> args =
      vivaldi::tabs_private::OnTabIsDetached::Create(
          tab_id, ExtensionTabUtil::GetWindowIdOfTab(web_contents()));
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  BroadcastEvent(tabs_private::OnTabIsDetached::kEventName,
                 std::move(args), profile);
}

void VivaldiPrivateTabObserver::WebContentsDidAttach(
    const content::WebContents* embedder_contents) {
  int tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents());
  std::unique_ptr<base::ListValue> args =
      vivaldi::tabs_private::OnTabIsAttached::Create(
          tab_id, ExtensionTabUtil::GetWindowIdOfTab(web_contents()),
          ConvertTabAlertState(chrome::GetTabAlertStateForContents(
              web_contents())));
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  BroadcastEvent(tabs_private::OnTabIsAttached::kEventName, std::move(args),
                 profile);
}

////////////////////////////////////////////////////////////////////////////////

TabsPrivateUpdateFunction::TabsPrivateUpdateFunction() {}

TabsPrivateUpdateFunction::~TabsPrivateUpdateFunction() {}

bool TabsPrivateUpdateFunction::RunAsync() {
  std::unique_ptr<tabs_private::Update::Params> params(
      tabs_private::Update::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  tabs_private::UpdateTabInfo* info = &params->tab_info;
  int tab_id = params->tab_id;

  content::WebContents* tabstrip_contents =
      ::vivaldi::ui_tools::GetWebContentsFromTabStrip(tab_id, GetProfile());
  if (tabstrip_contents) {
    VivaldiPrivateTabObserver* tab_api =
        VivaldiPrivateTabObserver::FromWebContents(tabstrip_contents);
    DCHECK(tab_api);
    if (tab_api) {
      if (info->show_images) {
        tab_api->SetShowImages(*info->show_images.get());
      }
      if (info->load_from_cache_only) {
        tab_api->SetLoadFromCacheOnly(*info->load_from_cache_only.get());
      }
      if (info->enable_plugins) {
        tab_api->SetEnablePlugins(*info->enable_plugins.get());
      }
      tab_api->CommitSettings();
      tab_api->BroadcastTabInfo();
    }
  }
  SendResponse(true);
  return true;
}

TabsPrivateGetFunction::TabsPrivateGetFunction() {}

TabsPrivateGetFunction::~TabsPrivateGetFunction() {}

bool TabsPrivateGetFunction::RunAsync() {
  std::unique_ptr<tabs_private::Get::Params> params(
      tabs_private::Get::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = params->tab_id;
  tabs_private::UpdateTabInfo info;

  content::WebContents* tabstrip_contents =
      ::vivaldi::ui_tools::GetWebContentsFromTabStrip(tab_id, GetProfile());
  if (tabstrip_contents) {
    VivaldiPrivateTabObserver* tab_api =
        VivaldiPrivateTabObserver::FromWebContents(tabstrip_contents);
    DCHECK(tab_api);
    if (tab_api) {
      info.show_images.reset(new bool(tab_api->show_images()));
      info.load_from_cache_only.reset(
          new bool(tab_api->load_from_cache_only()));
      info.enable_plugins.reset(new bool(tab_api->enable_plugins()));
      results_ = tabs_private::Get::Results::Create(info);
      SendResponse(true);
      return true;
    }
  }
  SendResponse(false);
  return false;
}

TabsPrivateInsertTextFunction::TabsPrivateInsertTextFunction() {}

TabsPrivateInsertTextFunction::~TabsPrivateInsertTextFunction() {}

bool TabsPrivateInsertTextFunction::RunAsync() {
  std::unique_ptr<tabs_private::InsertText::Params> params(
      tabs_private::InsertText::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = params->tab_id;
  bool success = false;

  base::string16 text = base::UTF8ToUTF16(params->text);

  content::WebContents* tabstrip_contents =
      ::vivaldi::ui_tools::GetWebContentsFromTabStrip(tab_id, GetProfile());

  content::RenderFrameHost* focused_frame =
      tabstrip_contents->GetFocusedFrame();

  if (focused_frame) {
    success = true;
    tabstrip_contents->GetRenderViewHost()->Send(new VivaldiMsg_InsertText(
        tabstrip_contents->GetRenderViewHost()->GetRoutingID(), text));
  }

  SendResponse(success);
  return success;
}

TabsPrivateStartDragFunction::TabsPrivateStartDragFunction() {}

TabsPrivateStartDragFunction::~TabsPrivateStartDragFunction() {}

bool TabsPrivateStartDragFunction::RunAsync() {
  std::unique_ptr<tabs_private::StartDrag::Params> params(
      tabs_private::StartDrag::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  SkBitmap bitmap;
  gfx::Vector2d image_offset;
  if (params->drag_image.get()) {
    std::string string_data;
    if (base::Base64Decode(params->drag_image->image, &string_data)) {
      const unsigned char* data =
          reinterpret_cast<const unsigned char*>(string_data.c_str());
      // Try PNG first.
      if (!gfx::PNGCodec::Decode(data, string_data.length(), &bitmap)) {
        // Try JPEG.
        std::unique_ptr<SkBitmap> decoded_jpeg(
            gfx::JPEGCodec::Decode(data, string_data.length()));
        if (decoded_jpeg) {
          bitmap = *decoded_jpeg;
        } else {
          LOG(WARNING) << "Error decoding png or jpg image data";
        }
      }
    } else {
      LOG(WARNING) << "Error decoding base64 image data";
    }
    image_offset.set_x(params->drag_image->cursor_x);
    image_offset.set_y(params->drag_image->cursor_y);
  }
  Browser* browser = BrowserList::GetInstance()->GetLastActive();
  DCHECK(browser);
  VivaldiBrowserWindow* window =
      static_cast<VivaldiBrowserWindow*>(browser->window());
  DCHECK(window);
  content::RenderViewHostImpl* rvh = static_cast<content::RenderViewHostImpl*>(
      window->web_contents()->GetRenderViewHost());
  DCHECK(rvh);
  content::RenderViewHostDelegateView* view =
      rvh->GetDelegate()->GetDelegateView();

  content::DropData drop_data;
  const base::string16 identifier(
      base::UTF8ToUTF16(params->drag_data.mime_type));
  const base::string16 custom_data(
      base::UTF8ToUTF16(params->drag_data.custom_data));

  drop_data.custom_data.insert(std::make_pair(identifier, custom_data));

  drop_data.url = GURL(params->drag_data.url);
  drop_data.url_title = base::UTF8ToUTF16(params->drag_data.title);

  blink::WebDragOperationsMask allowed_ops =
      static_cast<blink::WebDragOperationsMask>(
          blink::WebDragOperation::kWebDragOperationMove);

  gfx::ImageSkia image(gfx::ImageSkiaRep(bitmap, 1));
  content::DragEventSourceInfo event_info;

  event_info.event_source =
      params->is_from_touch.get() && *params->is_from_touch.get()
          ? ui::DragDropTypes::DRAG_EVENT_SOURCE_TOUCH
          : ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE;
  event_info.event_location =
      display::Screen::GetScreen()->GetCursorScreenPoint();

  ::vivaldi::SetTabDragInProgress(true);
  view->StartDragging(drop_data, allowed_ops, image, image_offset, event_info,
                      rvh->GetWidget());
  SendResponse(true);
  return true;
}

TabsPrivateScrollPageFunction::TabsPrivateScrollPageFunction() {}

TabsPrivateScrollPageFunction::~TabsPrivateScrollPageFunction() {}

bool TabsPrivateScrollPageFunction::RunAsync() {
  std::unique_ptr<tabs_private::ScrollPage::Params> params(
      tabs_private::ScrollPage::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = params->tab_id;
  std::string scroll_type = params->scroll_type;

  content::WebContents* tabstrip_contents =
      ::vivaldi::ui_tools::GetWebContentsFromTabStrip(tab_id, GetProfile());
  if (!tabstrip_contents) {
    SendResponse(false);
    return false;
  }
  content::RenderFrameHost* focused_frame =
      tabstrip_contents->GetFocusedFrame();
  if (!focused_frame) {
    SendResponse(false);
    return false;
  }
  content::RenderViewHost *rvh = tabstrip_contents->GetRenderViewHost();
  if (!rvh) {
    SendResponse(false);
    return false;
  }
  rvh->Send(new VivaldiViewMsg_ScrollPage(
      tabstrip_contents->GetRenderViewHost()->GetRoutingID(), scroll_type));

  SendResponse(true);
  return true;
}

}  // namespace extensions
