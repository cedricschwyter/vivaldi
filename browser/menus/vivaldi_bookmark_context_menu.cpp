// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved.

#include "browser/menus/vivaldi_bookmark_context_menu.h"

#include "app/vivaldi_resources.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/menus/vivaldi_menu_enums.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "extensions/api/bookmark_context_menu/bookmark_context_menu_api.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/controls/menu/menu_item_view.h"

namespace vivaldi {

static BookmarkSorter::SortField SortField = BookmarkSorter::FIELD_NONE;
static BookmarkSorter::SortOrder SortOrder = BookmarkSorter::ORDER_NONE;
static bool FolderGroup = false;

void BuildBookmarkContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_model->AddItemWithStringId(IDC_VIV_BOOKMARK_BAR_OPEN_CURRENT_TAB,
      IDS_VIV_BOOKMARK_BAR_OPEN_CURRENT_TAB);
  menu_model->AddItemWithStringId(IDC_VIV_BOOKMARK_BAR_OPEN_NEW_TAB,
      IDS_VIV_BOOKMARK_BAR_OPEN_NEW_TAB);
  menu_model->AddItemWithStringId(IDC_VIV_BOOKMARK_BAR_OPEN_BACKGROUND_TAB,
      IDS_VIV_BOOKMARK_BAR_OPEN_BACKGROUND_TAB);
  menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  menu_model->AddItemWithStringId(IDC_VIV_BOOKMARK_BAR_OPEN_NEW_WINDOW,
      IDS_VIV_BOOKMARK_BAR_OPEN_NEW_WINDOW);
  menu_model->AddItemWithStringId(IDC_VIV_BOOKMARK_BAR_OPEN_NEW_PRIVATE_WINDOW,
      IDS_VIV_BOOKMARK_BAR_OPEN_NEW_PRIVATE_WINDOW);
  menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  menu_model->AddItemWithStringId(IDC_VIV_BOOKMARK_BAR_ADD_ACTIVE_TAB,
      IDS_VIV_BOOKMARK_ADD_ACTIVE_TAB);
  menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  // Hide these entries as long as UI is missing in JS.
  /*
  menu_model->AddItemWithStringId(IDC_BOOKMARK_BAR_ADD_NEW_BOOKMARK,
      IDS_VIV_BOOKMARK_BAR_NEW_BOOKMARK);
  menu_model->AddItemWithStringId(IDC_BOOKMARK_BAR_NEW_FOLDER,
      IDS_VIV_BOOKMARK_BAR_NEW_FOLDER);
  menu_model->AddItemWithStringId(IDC_BOOKMARK_BAR_EDIT,
      IDS_VIV_BOOKMARK_BAR_EDIT);
  */
  menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  menu_model->AddItemWithStringId(IDC_CUT,
      IDS_VIV_BOOKMARK_BAR_CUT);
  menu_model->AddItemWithStringId(IDC_COPY,
      IDS_VIV_BOOKMARK_BAR_COPY);
  menu_model->AddItemWithStringId(IDC_PASTE,
      IDS_VIV_BOOKMARK_BAR_PASTE);
  menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  menu_model->AddItemWithStringId(IDC_BOOKMARK_BAR_REMOVE,
      IDS_VIV_BOOKMARK_BAR_REMOVE);
}

void ExecuteBookmarkContextMenuCommand(Browser* browser, int id, int command) {
  extensions::BookmarkContextMenuAPI* api =
    extensions::BookmarkContextMenuAPI::GetFactoryInstance()
        ->Get(browser->profile()->GetOriginalProfile());
  switch (command) {
    case IDC_VIV_BOOKMARK_BAR_OPEN_CURRENT_TAB:
    case IDC_VIV_BOOKMARK_BAR_OPEN_NEW_TAB:
    case IDC_VIV_BOOKMARK_BAR_OPEN_BACKGROUND_TAB:
    case IDC_VIV_BOOKMARK_BAR_OPEN_NEW_WINDOW:
    case IDC_VIV_BOOKMARK_BAR_OPEN_NEW_PRIVATE_WINDOW:
    case IDC_VIV_BOOKMARK_BAR_ADD_ACTIVE_TAB:
    case IDC_BOOKMARK_BAR_ADD_NEW_BOOKMARK:
    case IDC_BOOKMARK_BAR_NEW_FOLDER:
    case IDC_BOOKMARK_BAR_EDIT:
    case IDC_CUT:
    case IDC_COPY:
    case IDC_PASTE:
    case IDC_BOOKMARK_BAR_REMOVE:
      api->OnAction(id, 0, command);
      break;
  }
}

void OpenBookmarkById(Browser* browser, int id, int mouse_event_flags) {
  extensions::BookmarkContextMenuAPI* api =
    extensions::BookmarkContextMenuAPI::GetFactoryInstance()
        ->Get(browser->profile()->GetOriginalProfile());
  api->OnActivated(id, mouse_event_flags);
}


void SetBookmarkSortProperties(BookmarkSorter::SortField sort_field,
                               BookmarkSorter::SortOrder sort_order,
                               bool folder_group) {
  SortField = sort_field;
  SortOrder = sort_order;
  FolderGroup = folder_group;
}

void SortBookmarkNodes(const bookmarks::BookmarkNode* parent,
                       std::vector<bookmarks::BookmarkNode*>& nodes) {
  nodes.reserve(parent->child_count());
  for (int i = 0; i < parent->child_count(); ++i) {
    nodes.push_back(const_cast<bookmarks::BookmarkNode*>(parent->GetChild(i)));
  }
  BookmarkSorter sorter(SortField, SortOrder, FolderGroup);
  sorter.sort(nodes);
}

bool AddIfSeparator(const bookmarks::BookmarkNode* node,
                    views::MenuItemView* menu) {
  static base::string16 separator = base::UTF8ToUTF16("---");
  static base::string16 separator_description = base::UTF8ToUTF16("separator");
  if (node->GetTitle().compare(separator) == 0 &&
      node->GetDescription().compare(separator_description) == 0) {
    // Old add separators in unsorted mode
    if (SortField == BookmarkSorter::FIELD_NONE) {
      menu->AppendSeparator();
    }
    return true;
  }
  return false;
}


}  // vivaldi