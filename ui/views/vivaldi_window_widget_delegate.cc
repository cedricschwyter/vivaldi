// Copyright (c) 2017 Vivaldi Technologies AS. All rights reserved.
//
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "ui/views/vivaldi_window_widget_delegate.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/stl_util.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window_state.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "chrome/common/chrome_switches.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "extensions/browser/image_loader.h"
#include "extensions/common/draggable_region.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/compositor/layer.h"
#include "ui/devtools/devtools_connector.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_family.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"
#include "ui/wm/core/easy_resize_window_targeter.h"

#include "browser/vivaldi_browser_finder.h"
#include "extensions/api/vivaldi_utilities/vivaldi_utilities_api.h"
#include "extensions/api/window/window_private_api.h"
#include "ui/views/vivaldi_window_frame_view.h"
#include "ui/vivaldi_browser_window.h"
#include "ui/vivaldi_quit_confirmation_dialog.h"
#include "ui/vivaldi_ui_utils.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"
#include "vivaldi/ui/vector_icons/vector_icons.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "browser/win/vivaldi_utils.h"
#endif  // BUILDFLAG(IS_WIN)

// Colors for the flash screen.
#define DEFAULT_DARK_BACKGROUND_COLOR SkColorSetARGB(0xFF, 0x2d, 0x2d, 0x2d)
#define DEFAULT_LIGHT_BACKGROUND_COLOR SkColorSetARGB(0xFF, 0xd2, 0xd2, 0xd2)

namespace {

// Make sure we answer correctly for ClientView::CanClose to make sure the exit
// sequence is started when closing a BrowserWindow. See comment in
// fast_unload_controller.h.
class VivaldiWindowClientView : public views::ClientView {
 public:
  VivaldiWindowClientView(views::Widget* widget,
                          views::View* contents_view,
                          VivaldiBrowserWindow* window)
      : views::ClientView(widget, contents_view), window_(window) {}
  VivaldiWindowClientView(const VivaldiWindowClientView&) = delete;
  VivaldiWindowClientView& operator=(const VivaldiWindowClientView&) = delete;

  // views::ClientView:
  views::CloseRequestResult OnWindowCloseRequested() override {
    // This is to cach platform closing of windows, Alt+F4
    views::CloseRequestResult result =
        (window_->ConfirmWindowClose()
             ? views::CloseRequestResult::kCanClose
             : views::CloseRequestResult::kCannotClose);

    // If we are not asking before closing a window we must try to move pinned
    // tabs as soon as possible.
    if (!window_->browser()->profile()->GetPrefs()->GetBoolean(
            vivaldiprefs::kWindowsShowWindowCloseConfirmationDialog)) {
      window_->MovePersistentTabsToOtherWindowIfNeeded();
    }
    return result;
  }

 private:
  VivaldiBrowserWindow* const window_;
};

#if !BUILDFLAG(IS_WIN)
const int kLargeIconSizeViv = 256;
const int kSmallIconSizeViv = 16;
#endif

class VivaldiSplashBackground : public views::Background {
 public:
  explicit VivaldiSplashBackground(bool show_logo) : show_logo_(show_logo) {}

  VivaldiSplashBackground(const VivaldiSplashBackground&) = delete;
  VivaldiSplashBackground& operator=(const VivaldiSplashBackground&) = delete;

  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    ui::NativeTheme* theme = view->GetNativeTheme();
    if (theme && theme->GetDefaultSystemColorScheme() ==
                     ui::NativeTheme::ColorScheme::kDark) {
      canvas->DrawColor(DEFAULT_DARK_BACKGROUND_COLOR);
    } else {
      canvas->DrawColor(DEFAULT_LIGHT_BACKGROUND_COLOR);
    }

    if (show_logo_) {
      const gfx::Rect& b = view->GetContentsBounds();
      int size = b.width() * 0.16;
      const ui::ThemedVectorIcon& logo = ui::ThemedVectorIcon(
          &kVivaldiSplashIcon, SkColorSetARGB(0x1A, 0x00, 0x00, 0x00), size);
      canvas->DrawImageInt(logo.GetImageSkia(view->GetColorProvider()),
                           (b.width() - size) / 2, (b.height() - size) / 2);
    }
  }

 private:
  bool show_logo_;
};

}  // namespace

VivaldiWindowWidgetDelegate::VivaldiWindowWidgetDelegate(
    VivaldiBrowserWindow* window)
    : window_(window) {}

VivaldiWindowWidgetDelegate::~VivaldiWindowWidgetDelegate() = default;

// WidgetDelegate implementation.

views::Widget* VivaldiWindowWidgetDelegate::GetWidget() {
  return window_->GetWidget();
}

const views::Widget* VivaldiWindowWidgetDelegate::GetWidget() const {
  return window_->GetWidget();
}

std::unique_ptr<views::NonClientFrameView>
VivaldiWindowWidgetDelegate::CreateNonClientFrameView(views::Widget* widget) {
  DCHECK_EQ(widget, window_->GetWidget());
#if defined(USE_AURA)
  // On Mac Vivaldi Frame view handles both frameless and with-native-frame
  // cases.
  if (window_->with_native_frame())
    return views::WidgetDelegate::CreateNonClientFrameView(widget);
#endif
  return CreateVivaldiWindowFrameView(window_);
}

ui::ImageModel VivaldiWindowWidgetDelegate::GetWindowAppIcon() {
  if (window_->browser()->is_type_popup()) {
    content::WebContents* web_contents =
        window_->browser()->tab_strip_model()->GetActiveWebContents();
    if (web_contents) {
      favicon::FaviconDriver* favicon_driver =
          favicon::ContentFaviconDriver::FromWebContents(web_contents);
      gfx::Image app_icon = favicon_driver->GetFavicon();
      if (!app_icon.IsEmpty())
        return ui::ImageModel::FromImage(app_icon);
    }
  }
  // NOTE(pettern@vivaldi.com): Returning empty icons on Windows will make OS
  // grab the icons from the resource section instead, fixing VB-34191.
#if !BUILDFLAG(IS_WIN)
  if (!window_->icon_family_.empty()) {
    const gfx::Image* img =
        window_->icon_family_.GetBest(kLargeIconSizeViv, kLargeIconSizeViv);
    if (img)
      return ui::ImageModel::FromImage(*img);
  }
#endif
  return ui::ImageModel();
}

ui::ImageModel VivaldiWindowWidgetDelegate::GetWindowIcon() {
  // See comments in GetWindowAppIcon()
#if !BUILDFLAG(IS_WIN)
  if (!window_->icon_family_.empty()) {
    const gfx::Image* img =
        window_->icon_family_.GetBest(kSmallIconSizeViv, kSmallIconSizeViv);
    if (img)
      return ui::ImageModel::FromImage(*img);
  }
#endif
  return ui::ImageModel();
}

views::ClientView* VivaldiWindowWidgetDelegate::CreateClientView(
    views::Widget* widget) {
  DCHECK_EQ(widget, window_->GetWidget());
  content::WebContents* contents = window_->web_contents();
  auto web_view =
      std::make_unique<views::WebView>(contents->GetBrowserContext());

  // Events in the webview are handled in VivaldiEventHooks::HandleXXX.
  web_view->SetCanProcessEventsWithinSubtree(false);
  web_view->SetWebContents(contents);

  bool show_logo = window_->browser()->is_type_normal();
  web_view->SetBackground(std::make_unique<VivaldiSplashBackground>(show_logo));

  // ClientView manages the lifetime of its contents view manually.
  return new VivaldiWindowClientView(widget, web_view.release(), window_);
}

std::string VivaldiWindowWidgetDelegate::GetWindowName() const {
  if (window_->browser()) {
    return chrome::GetWindowName(window_->browser());
  }
  return std::string();
}

bool VivaldiWindowWidgetDelegate::WidgetHasHitTestMask() const {
  return false;
}

void VivaldiWindowWidgetDelegate::GetWidgetHitTestMask(SkPath* mask) const {
  NOTREACHED();
}

void VivaldiWindowWidgetDelegate::OnWidgetMove() {
  window_->OnNativeWindowChanged(true);
}

views::View* VivaldiWindowWidgetDelegate::GetInitiallyFocusedView() {
  return window_->GetWebView();
}

bool VivaldiWindowWidgetDelegate::CanMaximize() const {
  return true;
}

bool VivaldiWindowWidgetDelegate::CanMinimize() const {
  return true;
}

std::u16string VivaldiWindowWidgetDelegate::GetWindowTitle() const {
  return window_->GetTitle();
}

bool VivaldiWindowWidgetDelegate::ShouldShowWindowTitle() const {
  return true;
}

void VivaldiWindowWidgetDelegate::SaveWindowPlacement(
    const gfx::Rect& bounds,
    ui::WindowShowState show_state) {
  if (window_->browser() &&
      chrome::ShouldSaveWindowPlacement(window_->browser()) &&
      // If IsFullscreen() is true, we've just changed into fullscreen mode,
      // and we're catching the going-into-fullscreen sizing and positioning
      // calls, which we want to ignore.
      !window_->IsFullscreen() &&
      // VB-35145: Don't save placement after Hide() in
      // VivaldiBrowserWindow::ConfirmWindowClose() unmaximizes.
      !window_->is_hidden()) {
    WidgetDelegate::SaveWindowPlacement(bounds, show_state);
    chrome::SaveWindowPlacement(window_->browser(), bounds, show_state);
  }
  window_->OnNativeWindowChanged();
}

bool VivaldiWindowWidgetDelegate::GetSavedWindowPlacement(
    const views::Widget* widget,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  chrome::GetSavedWindowBoundsAndShowState(window_->browser(), bounds,
                                           show_state);

  if (chrome::SavedBoundsAreContentBounds(window_->browser())) {
    // This is normal non-app popup window. The value passed in |bounds|
    // represents two pieces of information:
    // - the position of the window, in screen coordinates (outer position).
    // - the size of the content area (inner size).
    // We need to use these values to determine the appropriate size and
    // position of the resulting window.
    gfx::Rect window_rect =
        GetWidget()->non_client_view()->GetWindowBoundsForClientBounds(*bounds);
    window_rect.set_origin(bounds->origin());

    // When we are given x/y coordinates of 0 on a created popup window,
    // assume none were given by the window.open() command.
    if (window_rect.x() == 0 && window_rect.y() == 0) {
      gfx::Size size = window_rect.size();
      window_rect.set_origin(WindowSizer::GetDefaultPopupOrigin(size));
    }
    *bounds = window_rect;
    *show_state = ui::SHOW_STATE_NORMAL;
  }
  // We return true because we can _always_ locate reasonable bounds using the
  // WindowSizer, and we don't want to trigger the Window's built-in "size to
  // default" handling because the browser window has no default preferred
  // size.
  return true;
}

bool VivaldiWindowWidgetDelegate::ShouldDescendIntoChildForEventHandling(
    gfx::NativeView child,
    const gfx::Point& location) {
  // TODO(igor@vivaldi.com): Figure out why it is not done on Mac and comment
  // about it.
  bool retval = true;
#if defined(USE_AURA)
  if (child->Contains(window_->web_contents()->GetNativeView())) {
    // App window should claim mouse events that fall within the draggable
    // region.
    const SkRegion* draggable = window_->draggable_region();
    retval = !draggable || !draggable->contains(location.x(), location.y());
  }
#endif
  return retval;
}

void VivaldiWindowWidgetDelegate::HandleKeyboardCode(ui::KeyboardCode code) {
  Browser* browser = window_->browser();
  if (!browser)
    return;
  extensions::WebViewGuest* current_webviewguest =
      vivaldi::ui_tools::GetActiveWebGuestFromBrowser(browser);
  if (current_webviewguest) {
    content::NativeWebKeyboardEvent synth_event(
        blink::WebInputEvent::Type::kRawKeyDown,
        blink::WebInputEvent::kNoModifiers, ui::EventTimeForNow());
    synth_event.windows_key_code = code;
    current_webviewguest->web_contents()->GetDelegate()->HandleKeyboardEvent(
        window_->web_contents(), synth_event);
  }
}

bool VivaldiWindowWidgetDelegate::ExecuteWindowsCommand(int command_id) {
#if BUILDFLAG(IS_WIN)
  // Windows-specific, see BrowserView::ExecuteWindowsCommand()
  int command_id_from_app_command =
      window_->GetCommandIDForAppCommandID(command_id);
  if (command_id_from_app_command != -1) {
    command_id = command_id_from_app_command;
  }
#endif

  return chrome::ExecuteCommand(window_->browser(), command_id);
}
