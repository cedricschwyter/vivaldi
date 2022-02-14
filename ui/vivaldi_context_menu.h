//
// Copyright (c) 2016 Vivaldi Technologies AS. All rights reserved.
//
#ifndef UI_VIVALDI_CONTEXT_MENU_H_
#define UI_VIVALDI_CONTEXT_MENU_H_

#include "browser/menus/bookmark_sorter.h"

namespace bookmarks {
class BookmarkNode;
}

namespace content {
class WebContents;
struct ContextMenuParams;
}

namespace gfx {
class Image;
class Rect;
}

namespace ui {
class SimpleMenuModel;
}

namespace vivaldi {
class VivaldiBookmarkMenu;
}

namespace vivaldi {
class VivaldiContextMenu;
}

namespace vivaldi {
VivaldiContextMenu* CreateVivaldiContextMenu(
    content::WebContents* web_contents,
    ui::SimpleMenuModel* menu_model,
    const content::ContextMenuParams& params);

VivaldiBookmarkMenu* CreateVivaldiBookmarkMenu(
    content::WebContents* web_contents,
    const bookmarks::BookmarkNode* node,
    int offset,
    vivaldi::BookmarkSorter::SortField sort_field,
    vivaldi::BookmarkSorter::SortOrder sort_order,
    bool folder_group,
    const gfx::Rect& button_rect);

class VivaldiBookmarkMenuObserver {
 public:
  virtual void BookmarkMenuClosed(VivaldiBookmarkMenu* menu) = 0;
 protected:
  virtual ~VivaldiBookmarkMenuObserver() {}
};

class VivaldiContextMenu {
 public:
  virtual ~VivaldiContextMenu() {}

  virtual void Show() = 0;
  virtual void SetIcon(const gfx::Image& icon, int id) {}
  virtual void SetSelectedItem(int id) {}
  virtual void UpdateMenu(ui::SimpleMenuModel* menu_model, int id) {}
};

class VivaldiBookmarkMenu {
 public:
  virtual ~VivaldiBookmarkMenu() {}
  virtual bool CanShow() = 0;
  virtual void Show() = 0;
  virtual void set_observer(VivaldiBookmarkMenuObserver* observer) {}
};


}  // namespace vivaldi

#endif  // UI_VIVALDI_CONTEXT_MENU_H_
