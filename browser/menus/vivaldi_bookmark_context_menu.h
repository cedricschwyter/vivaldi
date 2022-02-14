// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved.

#ifndef BROWSER_MENUS_VIVALDI_BOOKMARK_CONTEXT_MENUS_H_
#define BROWSER_MENUS_VIVALDI_BOOKMARK_CONTEXT_MENUS_H_

#include <vector>

#include "browser/menus/bookmark_sorter.h"

namespace bookmarks {
class BookmarkNode;
}

namespace ui {
class SimpleMenuModel;
}

namespace views {
class MenuItemView;
}

class Browser;

namespace vivaldi {
void BuildBookmarkContextMenu(ui::SimpleMenuModel* menu_model);
void ExecuteBookmarkContextMenuCommand(Browser* browser, int id, int command);
void OpenBookmarkById(Browser* browser, int id, int mouse_event_flags);
void SetBookmarkSortProperties(BookmarkSorter::SortField sortField,
                               BookmarkSorter::SortOrder sortOrder,
                               bool groupFolders);
void SortBookmarkNodes(const bookmarks::BookmarkNode* parent,
                       std::vector<bookmarks::BookmarkNode*>& nodes);
bool AddIfSeparator(const bookmarks::BookmarkNode* node,
                    views::MenuItemView* menu);
}  // vivaldi


#endif  // BROWSER_MENUS_VIVALDI_BOOKMARK_CONTEXT_MENUS_H_
