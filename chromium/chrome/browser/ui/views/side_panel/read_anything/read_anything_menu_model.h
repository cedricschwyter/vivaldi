// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_READ_ANYTHING_READ_ANYTHING_MENU_MODEL_H_
#define CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_READ_ANYTHING_READ_ANYTHING_MENU_MODEL_H_

#include "ui/base/models/simple_menu_model.h"

///////////////////////////////////////////////////////////////////////////////
// ReadAnythingMenuModel
//
//  This class makes the menu (dropdown) for the ReadAnythingMenuButton.
//
class ReadAnythingMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  ReadAnythingMenuModel();
  ReadAnythingMenuModel(const ReadAnythingMenuModel&) = delete;
  ReadAnythingMenuModel& operator=(const ReadAnythingMenuModel&) = delete;
  ~ReadAnythingMenuModel() override;

  // ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  virtual bool IsValidIndex(size_t index);
  void SetSelectedIndex(size_t index);
  absl::optional<size_t> GetSelectedIndex() const { return selected_index_; }
  void SetCallback(base::RepeatingCallback<void()> callback);

 private:
  absl::optional<size_t> selected_index_ = absl::nullopt;
  base::RepeatingClosure callback_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_READ_ANYTHING_READ_ANYTHING_MENU_MODEL_H_
