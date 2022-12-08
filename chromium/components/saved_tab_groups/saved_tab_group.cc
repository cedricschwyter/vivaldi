// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/saved_tab_groups/saved_tab_group.h"

#include <string>
#include <vector>

#include "base/guid.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/saved_tab_groups/saved_tab_group_tab.h"
#include "components/sync/protocol/saved_tab_group_specifics.pb.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_id.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

SavedTabGroup::SavedTabGroup(
    const std::u16string& title,
    const tab_groups::TabGroupColorId& color,
    const std::vector<SavedTabGroupTab>& urls,
    absl::optional<base::GUID> saved_guid,
    absl::optional<tab_groups::TabGroupId> local_group_id,
    absl::optional<base::Time> creation_time_windows_epoch_micros,
    absl::optional<base::Time> update_time_windows_epoch_micros)
    : saved_guid_(saved_guid.has_value() ? saved_guid.value()
                                         : base::GUID::GenerateRandomV4()),
      local_group_id_(local_group_id),
      title_(title),
      color_(color),
      saved_tabs_(urls),
      creation_time_windows_epoch_micros_(
          creation_time_windows_epoch_micros.has_value()
              ? creation_time_windows_epoch_micros.value()
              : base::Time::Now()),
      update_time_windows_epoch_micros_(
          update_time_windows_epoch_micros.has_value()
              ? update_time_windows_epoch_micros.value()
              : base::Time::Now()) {}

SavedTabGroup::SavedTabGroup(const SavedTabGroup& other) = default;

SavedTabGroup::~SavedTabGroup() = default;

absl::optional<SavedTabGroupTab> SavedTabGroup::GetTab(
    const base::GUID& tab_id) {
  absl::optional<int> index = GetIndexOfTab(tab_id);
  if (!index.has_value())
    return absl::nullopt;

  return saved_tabs()[index.value()];
}

bool SavedTabGroup::ContainsTab(const base::GUID& tab_id) const {
  absl::optional<int> index = GetIndexOfTab(tab_id);
  return index.has_value();
}

absl::optional<int> SavedTabGroup::GetIndexOfTab(
    const base::GUID& tab_id) const {
  auto it = base::ranges::find_if(
      saved_tabs(),
      [tab_id](const SavedTabGroupTab& tab) { return tab.guid() == tab_id; });
  if (it != saved_tabs().end())
    return it - saved_tabs().begin();
  return absl::nullopt;
}

SavedTabGroup& SavedTabGroup::SetTitle(std::u16string title) {
  title_ = title;
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

SavedTabGroup& SavedTabGroup::SetColor(tab_groups::TabGroupColorId color) {
  color_ = color;
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

SavedTabGroup& SavedTabGroup::SetLocalGroupId(
    absl::optional<tab_groups::TabGroupId> tab_group_id) {
  local_group_id_ = tab_group_id;
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

SavedTabGroup& SavedTabGroup::SetUpdateTimeWindowsEpochMicros(
    base::Time update_time_windows_epoch_micros) {
  update_time_windows_epoch_micros_ = update_time_windows_epoch_micros;
  return *this;
}

SavedTabGroup& SavedTabGroup::AddTab(size_t index, SavedTabGroupTab tab) {
  CHECK_GE(index, 0u);
  CHECK_LE(index, saved_tabs_.size());
  CHECK(!ContainsTab(tab.guid()));
  saved_tabs_.emplace(saved_tabs_.begin() + index, std::move(tab));
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

SavedTabGroup& SavedTabGroup::RemoveTab(const base::GUID& tab_id) {
  absl::optional<size_t> index = GetIndexOfTab(tab_id);
  CHECK(index.has_value());
  CHECK_GE(index.value(), 0u);
  CHECK_LT(index.value(), saved_tabs_.size());
  saved_tabs_.erase(saved_tabs_.begin() + index.value());
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

SavedTabGroup& SavedTabGroup::ReplaceTabAt(const base::GUID& tab_id,
                                           SavedTabGroupTab tab) {
  absl::optional<size_t> index = GetIndexOfTab(tab_id);
  CHECK(index.has_value());
  CHECK_GE(index.value(), 0u);
  CHECK_LT(index.value(), saved_tabs_.size());
  CHECK(!ContainsTab(tab.guid()));
  saved_tabs_.erase(saved_tabs_.begin() + index.value());
  saved_tabs_.insert(saved_tabs_.begin() + index.value(), std::move(tab));
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

SavedTabGroup& SavedTabGroup::MoveTab(const base::GUID& tab_id,
                                      size_t new_index) {
  absl::optional<size_t> curr_index = GetIndexOfTab(tab_id);
  CHECK(curr_index.has_value());
  CHECK_GE(curr_index.value(), 0u);
  CHECK_LT(curr_index.value(), saved_tabs_.size());
  CHECK_GE(new_index, 0u);
  CHECK_LT(new_index, saved_tabs_.size());

  if (curr_index > new_index) {
    std::rotate(saved_tabs_.begin() + new_index,
                saved_tabs_.begin() + curr_index.value(),
                saved_tabs_.begin() + curr_index.value() + 1);
  } else if (curr_index < new_index) {
    std::rotate(
        saved_tabs_.rbegin() + ((saved_tabs_.size() - 1) - new_index),
        saved_tabs_.rbegin() + ((saved_tabs_.size() - 1) - curr_index.value()),
        saved_tabs_.rbegin() + ((saved_tabs_.size() - 1) - curr_index.value()) +
            1);
  }
  SetUpdateTimeWindowsEpochMicros(base::Time::Now());
  return *this;
}

bool SavedTabGroup::ShouldMergeGroup(
    sync_pb::SavedTabGroupSpecifics* sync_specific) {
  bool sync_update_is_latest =
      sync_specific->update_time_windows_epoch_micros() >=
      update_time_windows_epoch_micros()
          .ToDeltaSinceWindowsEpoch()
          .InMicroseconds();
  // TODO(dljames): crbug/1371953 - Investigate if we should consider the
  // creation time.
  return sync_update_is_latest;
}

std::unique_ptr<sync_pb::SavedTabGroupSpecifics> SavedTabGroup::MergeGroup(
    std::unique_ptr<sync_pb::SavedTabGroupSpecifics> sync_specific) {
  if (ShouldMergeGroup(sync_specific.get())) {
    SetTitle(base::UTF8ToUTF16(sync_specific->group().title()));
    SetColor(SyncColorToTabGroupColor(sync_specific->group().color()));
    SetUpdateTimeWindowsEpochMicros(base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(sync_specific->update_time_windows_epoch_micros())));
  }

  return ToSpecifics();
}

// static
SavedTabGroup SavedTabGroup::FromSpecifics(
    const sync_pb::SavedTabGroupSpecifics& specific) {
  const tab_groups::TabGroupColorId color =
      SyncColorToTabGroupColor(specific.group().color());
  const std::u16string& title = base::UTF8ToUTF16(specific.group().title());

  base::GUID guid = base::GUID::ParseLowercase(specific.guid());
  base::Time creation_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(specific.creation_time_windows_epoch_micros()));
  base::Time update_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(specific.update_time_windows_epoch_micros()));

  return SavedTabGroup(title, color, {}, guid, absl::nullopt, creation_time,
                       update_time);
}

std::unique_ptr<sync_pb::SavedTabGroupSpecifics> SavedTabGroup::ToSpecifics()
    const {
  std::unique_ptr<sync_pb::SavedTabGroupSpecifics> pb_specific =
      std::make_unique<sync_pb::SavedTabGroupSpecifics>();
  pb_specific->set_guid(saved_guid().AsLowercaseString());
  pb_specific->set_creation_time_windows_epoch_micros(
      creation_time_windows_epoch_micros()
          .ToDeltaSinceWindowsEpoch()
          .InMicroseconds());
  pb_specific->set_update_time_windows_epoch_micros(
      update_time_windows_epoch_micros()
          .ToDeltaSinceWindowsEpoch()
          .InMicroseconds());

  sync_pb::SavedTabGroup* pb_group = pb_specific->mutable_group();
  pb_group->set_color(TabGroupColorToSyncColor(color()));
  pb_group->set_title(base::UTF16ToUTF8(title()));

  return pb_specific;
}

// static
tab_groups::TabGroupColorId SavedTabGroup::SyncColorToTabGroupColor(
    const sync_pb::SavedTabGroup::SavedTabGroupColor color) {
  switch (color) {
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_GREY:
      return tab_groups::TabGroupColorId::kGrey;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_BLUE:
      return tab_groups::TabGroupColorId::kBlue;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_RED:
      return tab_groups::TabGroupColorId::kRed;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_YELLOW:
      return tab_groups::TabGroupColorId::kYellow;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_GREEN:
      return tab_groups::TabGroupColorId::kGreen;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_PINK:
      return tab_groups::TabGroupColorId::kPink;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_PURPLE:
      return tab_groups::TabGroupColorId::kPurple;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_CYAN:
      return tab_groups::TabGroupColorId::kCyan;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_ORANGE:
      return tab_groups::TabGroupColorId::kOrange;
    case sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_UNSPECIFIED:
      return tab_groups::TabGroupColorId::kGrey;
  }
}

// static
sync_pb::SavedTabGroup_SavedTabGroupColor
SavedTabGroup::TabGroupColorToSyncColor(
    const tab_groups::TabGroupColorId color) {
  switch (color) {
    case tab_groups::TabGroupColorId::kGrey:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_GREY;
    case tab_groups::TabGroupColorId::kBlue:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_BLUE;
    case tab_groups::TabGroupColorId::kRed:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_RED;
    case tab_groups::TabGroupColorId::kYellow:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_YELLOW;
    case tab_groups::TabGroupColorId::kGreen:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_GREEN;
    case tab_groups::TabGroupColorId::kPink:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_PINK;
    case tab_groups::TabGroupColorId::kPurple:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_PURPLE;
    case tab_groups::TabGroupColorId::kCyan:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_CYAN;
    case tab_groups::TabGroupColorId::kOrange:
      return sync_pb::SavedTabGroup::SAVED_TAB_GROUP_COLOR_ORANGE;
  }

  NOTREACHED() << "No known conversion for the supplied color.";
}
