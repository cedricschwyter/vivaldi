// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#include "browser/removed_partners_tracker.h"

#include <numeric>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "browser/vivaldi_default_bookmarks.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/vivaldi_bookmark_kit.h"
#include "components/bookmarks/vivaldi_partners.h"
#include "components/prefs/pref_service.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace vivaldi_partners {

// To determine meta data changes for which we should clear the partner id
class RemovedPartnersTracker::MetaInfoChangeFilter {
 public:
  MetaInfoChangeFilter(const bookmarks::BookmarkNode* node)
      : id_(node->id()),
        speeddial_(vivaldi_bookmark_kit::GetSpeeddial(node)),
        bookmarkbar_(vivaldi_bookmark_kit::GetBookmarkbar(node)),
        description_(vivaldi_bookmark_kit::GetDescription(node)),
        nickname_(vivaldi_bookmark_kit::GetNickname(node)) {}
  bool HasChanged(const bookmarks::BookmarkNode* node) {
    if (id_ == node->id() &&
        speeddial_ == vivaldi_bookmark_kit::GetSpeeddial(node) &&
        bookmarkbar_ == vivaldi_bookmark_kit::GetBookmarkbar(node) &&
        description_ == vivaldi_bookmark_kit::GetDescription(node) &&
        nickname_ == vivaldi_bookmark_kit::GetNickname(node)) {
      return false;
    }
    return true;
  }

 private:
  int64_t id_;
  bool speeddial_;
  bool bookmarkbar_;
  std::string description_;
  std::string nickname_;
};

/*static*/
void RemovedPartnersTracker::Create(PrefService* prefs,
                                    bookmarks::BookmarkModel* model) {
  new RemovedPartnersTracker(prefs, model);
}

/*static*/
std::set<base::GUID> RemovedPartnersTracker::ReadRemovedPartners(
    const base::Value::List& deleted_partners,
    bool& upgraded_old_id) {
  upgraded_old_id = false;
  std::set<base::GUID> removed_partners;

  for (const base::Value& deleted_partner : deleted_partners) {
    if (!deleted_partner.is_string())
      continue;
    base::GUID migrated_removed_partner =
        base::GUID::ParseCaseInsensitive(deleted_partner.GetString());
    if (migrated_removed_partner.is_valid()) {
      // Upgrade from old, locale-based id to new id.
      upgraded_old_id = upgraded_old_id || vivaldi_partners::MapLocaleIdToGUID(
                                               migrated_removed_partner);
      removed_partners.insert(migrated_removed_partner);
    }
  }
  return removed_partners;
}

RemovedPartnersTracker::RemovedPartnersTracker(PrefService* prefs,
                                               bookmarks::BookmarkModel* model)
    : model_(model), prefs_(prefs) {
  model_->AddObserver(this);
  if (model_->loaded())
    BookmarkModelLoaded(model, false);
}

RemovedPartnersTracker::~RemovedPartnersTracker() {
  model_->RemoveObserver(this);
}

void RemovedPartnersTracker::BookmarkNodeChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  TrackRemovals(node, false);
}

void RemovedPartnersTracker::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    size_t old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
  TrackRemovals(node, true);
}

void RemovedPartnersTracker::OnWillChangeBookmarkMetaInfo(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  // No need to filter on upgrade
  if (!vivaldi_default_bookmarks::g_bookmark_update_actve) {
    change_filter_ = std::make_unique<MetaInfoChangeFilter>(node);
  }
}

void RemovedPartnersTracker::BookmarkMetaInfoChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  if (change_filter_ && change_filter_->HasChanged(node)) {
    TrackRemovals(node, false);
  }
  change_filter_.reset();
}

void RemovedPartnersTracker::BookmarkModelLoaded(
    bookmarks::BookmarkModel* model,
    bool ids_reassigned) {
  const base::Value::List& deleted_partners =
      prefs_->GetList(vivaldiprefs::kBookmarksDeletedPartners);
  bool upgraded_old_id;
  removed_partners_ = ReadRemovedPartners(deleted_partners, upgraded_old_id);
  if (upgraded_old_id)
    SaveRemovedPartners();
}

void RemovedPartnersTracker::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
  delete this;
}

void RemovedPartnersTracker::SaveRemovedPartners() {
  base::Value::List removed_partners_list;

  for (const auto& removed_partner : removed_partners_) {
    removed_partners_list.Append(removed_partner.AsLowercaseString());
  }

  prefs_->SetList(vivaldiprefs::kBookmarksDeletedPartners,
                  std::move(removed_partners_list));
}

void RemovedPartnersTracker::TrackRemovals(const bookmarks::BookmarkNode* node,
                                           bool recursive) {
  if (vivaldi_default_bookmarks::g_bookmark_update_actve)
    return;
  DoTrackRemovals(node, recursive);
  SaveRemovedPartners();
}

void RemovedPartnersTracker::DoTrackRemovals(
    const bookmarks::BookmarkNode* node,
    bool recursive) {
  base::GUID partner_id = vivaldi_bookmark_kit::GetPartner(node);
  if (partner_id.is_valid()) {
    removed_partners_.insert(partner_id);
    vivaldi_bookmark_kit::RemovePartnerId(model_, node);
  }
  if (recursive) {
    for (auto& it : node->children()) {
      DoTrackRemovals(it.get(), true);
    }
  }
}

}  // namespace vivaldi_partners