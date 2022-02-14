// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright (c) 2015 Vivaldi Technologies AS. All rights reserved.
//

#include "ui/views/vivaldi_context_menu_views.h"

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "browser/menus/vivaldi_bookmark_context_menu.h"
#include "browser/vivaldi_browser_finder.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_menu_controller_views.h"
#include "chrome/browser/ui/views/renderer_context_menu/render_view_context_menu_views.h"
#include "chrome/common/chrome_switches.h"
#include "components/renderer_context_menu/views/toolkit_delegate_views.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "extensions/tools/vivaldi_tools.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/widget/widget.h"

namespace vivaldi {
VivaldiContextMenu* CreateVivaldiContextMenu(
    content::WebContents* web_contents,
    ui::SimpleMenuModel* menu_model,
    const content::ContextMenuParams& params) {
  return new VivaldiContextMenuViews(web_contents, menu_model, params);
}

VivaldiBookmarkMenu* CreateVivaldiBookmarkMenu(
    content::WebContents* web_contents,
    const bookmarks::BookmarkNode* node,
    int offset,
    vivaldi::BookmarkSorter::SortField sort_field,
    vivaldi::BookmarkSorter::SortOrder sort_order,
    bool folder_group,
    const gfx::Rect& button_rect) {
  return new VivaldiBookmarkMenuViews(web_contents, node, offset,
                                      sort_field, sort_order, folder_group,
                                      button_rect);
}
}  // vivialdi

VivaldiContextMenuViews::VivaldiContextMenuViews(
    content::WebContents* web_contents,
    ui::SimpleMenuModel* menu_model,
    const content::ContextMenuParams& params)
    : web_contents_(web_contents),
      menu_model_(menu_model),
      params_(params) {
  toolkit_delegate_.reset(new ToolkitDelegateViews);
  menu_view_ = toolkit_delegate_->VivaldiInit(menu_model_);
}

VivaldiContextMenuViews::~VivaldiContextMenuViews() {}

void VivaldiContextMenuViews::RunMenuAt(views::Widget* parent,
                                        const gfx::Point& point,
                                        ui::MenuSourceType type) {
  static_cast<ToolkitDelegateViews*>(toolkit_delegate_.get())
      ->RunMenuAt(parent, point, type);
}

void VivaldiContextMenuViews::Show() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode))
    return;

  // Menus need a Widget to work. If we're not the active tab we won't
  // necessarily be in a widget.
  views::Widget* top_level_widget = GetTopLevelWidget();
  if (!top_level_widget)
    return;

  // Don't show empty menus.
  if (menu_model_->GetItemCount() == 0)
    return;

  // Convert from target window coordinates to root window coordinates.
  gfx::Point screen_point(params_.x, params_.y);
  aura::Window* target_window = GetActiveNativeView();
  aura::Window* root_window = target_window->GetRootWindow();
  aura::client::ScreenPositionClient* screen_position_client =
      aura::client::GetScreenPositionClient(root_window);
  if (screen_position_client) {
    screen_position_client->ConvertPointToScreen(target_window, &screen_point);
  }
  // Enable recursive tasks on the message loop so we can get updates while
  // the context menu is being displayed.
  base::MessageLoopCurrent::ScopedNestableTaskAllower allow;
  RunMenuAt(top_level_widget, screen_point, params_.source_type);
}

views::Widget* VivaldiContextMenuViews::GetTopLevelWidget() {
  return views::Widget::GetTopLevelWidgetForNativeView(GetActiveNativeView());
}

aura::Window* VivaldiContextMenuViews::GetActiveNativeView() {
  return web_contents_->GetFullscreenRenderWidgetHostView()
             ? web_contents_->GetFullscreenRenderWidgetHostView()
                   ->GetNativeView()
             : web_contents_->GetNativeView();
}

void VivaldiContextMenuViews::SetIcon(const gfx::Image& icon, int id) {
  if (menu_view_->GetMenuItemByID(id)) {
    menu_view_->SetIcon(*icon.ToImageSkia(), id);
  }
}

void VivaldiContextMenuViews::SetSelectedItem(int id) {
  views::MenuItemView* item = menu_view_->GetMenuItemByID(id);
  if (item && views::MenuController::GetActiveInstance()) {
    views::MenuController::GetActiveInstance()->SetSelection(
        item, views::MenuController::SELECTION_OPEN_SUBMENU |
                  views::MenuController::SELECTION_UPDATE_IMMEDIATELY);
  }
}

void VivaldiContextMenuViews::UpdateMenu(ui::SimpleMenuModel* menu_model,
                                         int id) {
  views::MenuItemView* view = menu_view_->GetMenuItemByID(id);
  if (view)
    toolkit_delegate_->VivaldiUpdateMenu(view, menu_model);
}


VivaldiBookmarkMenuViews::VivaldiBookmarkMenuViews(
    content::WebContents* web_contents,
    const bookmarks::BookmarkNode* node,
    int offset,
    vivaldi::BookmarkSorter::SortField sort_field,
    vivaldi::BookmarkSorter::SortOrder sort_order,
    bool folder_group,
    const gfx::Rect& button_rect)
    : web_contents_(web_contents),
      button_rect_(button_rect),
      controller_(nullptr),
      observer_(nullptr) {
  Browser* browser = GetBrowser();
  if (browser) {
    vivaldi::SetBookmarkSortProperties(sort_field, sort_order, folder_group);
    controller_ = new BookmarkMenuController(browser, web_contents_,
        GetTopLevelWidget(), node, offset, false);
    controller_->set_observer(this);
  }
}

VivaldiBookmarkMenuViews::~VivaldiBookmarkMenuViews() {
  if (controller_) {
    controller_->set_observer(nullptr);
  }
}

void VivaldiBookmarkMenuViews::set_observer(
    vivaldi::VivaldiBookmarkMenuObserver* observer) {
  observer_ = observer;
}

bool VivaldiBookmarkMenuViews::CanShow() {
  return controller_ != nullptr;
}

void VivaldiBookmarkMenuViews::Show() {
  gfx::Point screen_point(button_rect_.x(), button_rect_.y());
  aura::Window* target_window = GetActiveNativeView();
  aura::Window* root_window = target_window->GetRootWindow();
  aura::client::ScreenPositionClient* screen_position_client =
      aura::client::GetScreenPositionClient(root_window);
  if (screen_position_client) {
    screen_position_client->ConvertPointToScreen(target_window, &screen_point);
  }
  gfx::Rect rect(screen_point, button_rect_.size());
  controller_->RunMenuAt(GetTopLevelWidget()->GetContentsView(), rect);
}

void VivaldiBookmarkMenuViews::BookmarkMenuControllerDeleted(
    BookmarkMenuController* controller) {
  if (observer_) {
    observer_->BookmarkMenuClosed(this);
  }
  controller_ = nullptr;
}

views::Widget* VivaldiBookmarkMenuViews::GetTopLevelWidget() {
  return views::Widget::GetTopLevelWidgetForNativeView(GetActiveNativeView());
}

aura::Window* VivaldiBookmarkMenuViews::GetActiveNativeView() {
  return web_contents_->GetFullscreenRenderWidgetHostView()
             ? web_contents_->GetFullscreenRenderWidgetHostView()
                   ->GetNativeView()
             : web_contents_->GetNativeView();
}

Browser* VivaldiBookmarkMenuViews::GetBrowser() {
  views::Widget* widget = GetTopLevelWidget();
  return widget ? chrome::FindBrowserWithWindow(widget->GetNativeWindow())
                : nullptr;
}