//
// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved.
//
#include "ui/vivaldi_bookmark_menu_mac.h"

#include "base/mac/scoped_nsobject.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/menus/bookmark_sorter.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/events/cocoa/cocoa_event_utils.h"

#include "app/vivaldi_commands.h"
#include "app/vivaldi_resources.h"
#include "components/bookmarks/vivaldi_bookmark_kit.h"
#include "prefs/vivaldi_gen_prefs.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

using bookmarks::BookmarkNode;

static std::u16string Separator;
static std::u16string SeparatorDescription;
static std::vector<int> MenuIds;
static std::string ContainerEdge = "below";
static unsigned int ContainerIndex = 0;

namespace vivaldi {

class SortFlags {
 public:
  SortFlags() {
    field_ = ::vivaldi::BookmarkSorter::FIELD_NONE;
    order_ = ::vivaldi::BookmarkSorter::ORDER_NONE;
  }
  void InitFromPrefs();

  ::vivaldi::BookmarkSorter::SortField field_;
  ::vivaldi::BookmarkSorter::SortOrder order_;
};

void SortFlags::InitFromPrefs() {
  AppController* appController = static_cast<AppController*>([NSApp delegate]);
  Profile* profile = appController.lastProfile;
  if (profile) {
    PrefService* pref_service = profile->GetPrefs();
    auto& dict = pref_service->GetDict(
        vivaldiprefs::kBookmarksManagerSorting);
    if (const std::string* sortField = dict.FindString("sortField")) {
      if (*sortField == "manually")
        field_ = ::vivaldi::BookmarkSorter::FIELD_NONE;
      else if (*sortField == "title")
        field_ = ::vivaldi::BookmarkSorter::FIELD_TITLE;
      else if (*sortField == "url")
        field_ = ::vivaldi::BookmarkSorter::FIELD_URL;
      else if (*sortField == "nickname")
        field_ = ::vivaldi::BookmarkSorter::FIELD_NICKNAME;
      else if (*sortField == "description")
        field_ = ::vivaldi::BookmarkSorter::FIELD_DESCRIPTION;
    }
    if (absl::optional<int> sortOrder = dict.FindInt("sortOrder")) {
      if (*sortOrder == 1)
        order_ = ::vivaldi::BookmarkSorter::ORDER_NONE;
      else if (*sortOrder == 2)
        order_ = ::vivaldi::BookmarkSorter::ORDER_ASCENDING;
      else if (*sortOrder == 3)
        order_ = ::vivaldi::BookmarkSorter::ORDER_DESCENDING;
    }
  }
}

std::vector<int>& GetBookmarkMenuIds() {
  return MenuIds;
}

bool IsBookmarkMenuId(int candidate) {
  for (auto id: GetBookmarkMenuIds()) {
    if (candidate == id) {
      return true;
    }
  }
  return false;
}

unsigned int GetMenuIndex() {
  return ContainerIndex;
}

void SetContainerState(const std::string& edge, unsigned int index) {
  if (ContainerEdge != edge || ContainerIndex != index) {
    ContainerEdge = edge;
    ContainerIndex = index;
    ClearBookmarkMenu();
  }
}

void ClearBookmarkMenu() {
  AppController* appController = static_cast<AppController*>([NSApp delegate]);
  if ([appController bookmarkMenuBridge])
    [appController bookmarkMenuBridge]->ClearBookmarkMenu();
}

void GetBookmarkNodes(const BookmarkNode* node,
                      std::vector<bookmarks::BookmarkNode*>& nodes) {
  SortFlags flags;
  flags.InitFromPrefs();
  vivaldi::BookmarkSorter sorter(flags.field_, flags.order_, true);

  nodes.reserve(node->children().size());
  for (auto& it: node->children()) {
    BookmarkNode* child =
        const_cast<bookmarks::BookmarkNode*>(it.get());
    if (flags.order_ == ::vivaldi::BookmarkSorter::ORDER_NONE ||
        !::vivaldi_bookmark_kit::IsSeparator(child)) {
      nodes.push_back(child);
    }
  }

  sorter.sort(nodes);
}

void AddExtraBookmarkMenuItems(NSMenu* menu, unsigned int* menu_index,
                               const BookmarkNode* node, bool on_top) {
  std::string edge = on_top ? "above" : "below";
  if (edge == ContainerEdge) {
    if (edge == "below") {
      [menu insertItem:[NSMenuItem separatorItem] atIndex:*menu_index];
      *menu_index += 1;
    }

    base::scoped_nsobject<NSMenuItem> item([[NSMenuItem alloc]
            initWithTitle:l10n_util::GetNSString(
                IDS_VIV_ADD_ACTIVE_TAB_TO_BOOKMARKS)
                   action:nil
            keyEquivalent:@""]);
    [item setTag:IDC_VIV_ADD_ACTIVE_TAB_TO_BOOKMARKS];
    AppController* app_controller =
      static_cast<AppController*>([NSApp delegate]);
    [item setTarget:app_controller];
    [app_controller setVivaldiMenuItemAction:item];
    [item setRepresentedObject:[NSString stringWithFormat:@"%lld", node->id()]];
    [menu insertItem:item atIndex:*menu_index];
    *menu_index += 1;

    if (edge == "above") {
      [menu insertItem:[NSMenuItem separatorItem] atIndex:*menu_index];
      *menu_index += 1;
    }
  }
}

void OnClearBookmarkMenu(NSMenu* menu, NSMenuItem* item) {
  if ([item tag] == IDC_VIV_ADD_ACTIVE_TAB_TO_BOOKMARKS)
    [menu removeItem:item];
}

WindowOpenDisposition WindowOpenDispositionFromNSEvent(NSEvent* event,
                                                       PrefService* prefs) {
  int event_flags = ui::EventFlagsFromNSEventWithModifiers(event,
                                                       [event modifierFlags]);
  WindowOpenDisposition default_disposition =
    WindowOpenDisposition::CURRENT_TAB;
  if (prefs->GetBoolean(vivaldiprefs::kBookmarksOpenInNewTab)) {
    default_disposition =
      prefs->GetBoolean(vivaldiprefs::kTabsOpenNewInBackground) ?
          WindowOpenDisposition::NEW_BACKGROUND_TAB :
          WindowOpenDisposition::NEW_FOREGROUND_TAB;
  }
  return ui::DispositionFromEventFlags(event_flags, default_disposition);
}

}  // namespace vivaldi
