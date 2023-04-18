// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_READ_ANYTHING_READ_ANYTHING_MODEL_H_
#define CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_READ_ANYTHING_READ_ANYTHING_MODEL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_menu_model.h"
#include "chrome/common/accessibility/read_anything.mojom.h"
#include "chrome/common/accessibility/read_anything_constants.h"
#include "components/services/screen_ai/buildflags/buildflags.h"
#include "content/public/browser/ax_event_notification_details.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "ui/base/models/combobox_model.h"

using read_anything::mojom::LetterSpacing;
using read_anything::mojom::LineSpacing;

///////////////////////////////////////////////////////////////////////////////
// ReadAnythingFontModel
//
//  A class that stores the data for the font combobox.
//  This class is owned by the ReadAnythingModel and has the same lifetime as
//  the browser.
//
class ReadAnythingFontModel : public ui::ComboboxModel {
 public:
  ReadAnythingFontModel();
  ReadAnythingFontModel(const ReadAnythingFontModel&) = delete;
  ReadAnythingFontModel& operator=(const ReadAnythingFontModel&) = delete;
  ~ReadAnythingFontModel() override;

  std::string GetFontNameAt(size_t index);
  bool IsValidFontName(const std::string& font_name);
  bool IsValidFontIndex(size_t index);
  void SetDefaultIndexFromPrefsFontName(std::string prefs_font_name);
  std::string GetLabelFontListAt(size_t index);
  size_t GetStartingStateIndex() { return GetDefaultIndex().value(); }

 protected:
  // ui::Combobox implementation:
  absl::optional<size_t> GetDefaultIndex() const override;
  size_t GetItemCount() const override;
  std::u16string GetItemAt(size_t index) const override;
  std::u16string GetDropDownTextAt(size_t index) const override;

 private:
  // Styled font names for the drop down options in front-end.
  std::vector<std::u16string> font_choices_;

  // Default index for drop down, either zero or populated from prefs.
  size_t default_index_ = 0;
};

///////////////////////////////////////////////////////////////////////////////
// ReadAnythingColorsModel
//
//  A class that stores the data for the colors combobox.
//  This class is owned by the ReadAnythingModel and has the same lifetime as
//  the browser.
//
class ReadAnythingColorsModel : public ReadAnythingMenuModel {
 public:
  ReadAnythingColorsModel();
  ReadAnythingColorsModel(const ReadAnythingColorsModel&) = delete;
  ReadAnythingColorsModel& operator=(const ReadAnythingColorsModel&) = delete;
  ~ReadAnythingColorsModel() override;

  // Simple struct to hold the various colors to keep code cleaner.
  struct ColorInfo {
    // The name of the colors, e.g. Default, Light, Dark.
    std::u16string name;

    // The resources value/identifier for the icon image asset.
    int icon_asset;

    // The foreground color, used for text and icon hints.
    ui::ColorId foreground_color_id;

    // The background color, used for text background.
    ui::ColorId background_color_id;

    // The separator color, used for visual separators between elements in the
    // toolbar.
    ui::ColorId separator_color_id;
  };

  bool IsValidIndex(size_t index) override;
  ColorInfo& GetColorsAt(size_t index);
  ui::ImageModel GetDropDownIconAt(size_t index) const;

 private:
  // Individual combobox choices for colors presented in front-end.
  std::vector<ColorInfo> colors_choices_;
};

//////////////////////////////////////////////////////////////////////////////
// ReadAnythingLineSpacingModel
//
//  A class that stores the data for the colors combobox.
//  This class is owned by the ReadAnythingModel and has the same lifetime as
//  the browser.
//
class ReadAnythingLineSpacingModel : public ReadAnythingMenuModel {
 public:
  ReadAnythingLineSpacingModel();
  ReadAnythingLineSpacingModel(const ReadAnythingLineSpacingModel&) = delete;
  ReadAnythingLineSpacingModel& operator=(const ReadAnythingLineSpacingModel&) =
      delete;
  ~ReadAnythingLineSpacingModel() override;

  // Simple struct to hold the various spacings to keep code cleaner.
  struct LineSpacingInfo {
    // The enum value of the line spacing.
    read_anything::mojom::LineSpacing enum_value;

    // The name of the line spacing, e.g. Standard, Loose, Very Loose.
    std::u16string name;

    // The resources value/identifier for the icon image asset.
    const gfx::VectorIcon& icon_asset;
  };

  bool IsValidIndex(size_t index) override;
  size_t GetIndexForLineSpacing(read_anything::mojom::LineSpacing line_spacing);
  read_anything::mojom::LineSpacing GetLineSpacingAt(size_t index);

 private:
  // Names for the drop down options in front-end.
  std::vector<LineSpacingInfo> lines_choices_;
};

///////////////////////////////////////////////////////////////////////////////
// ReadAnythingLetterSpacingModel
//
//  A class that stores the data for the letter spacing combobox.
//  This class is owned by the ReadAnythingModel and has the same lifetime as
//  the browser.
//

class ReadAnythingLetterSpacingModel : public ReadAnythingMenuModel {
 public:
  ReadAnythingLetterSpacingModel();
  ReadAnythingLetterSpacingModel(const ReadAnythingLetterSpacingModel&) =
      delete;
  ReadAnythingLetterSpacingModel& operator=(
      const ReadAnythingLetterSpacingModel&) = delete;
  ~ReadAnythingLetterSpacingModel() override;

  // Simple struct to hold the various spacings to keep code cleaner.
  struct LetterSpacingInfo {
    // The enum value of the letter spacing.
    read_anything::mojom::LetterSpacing enum_value;

    // The name of the letter spacing, e.g. Standard, Wide, Very Wide.
    std::u16string name;

    // The resources value/identifier for the icon image asset.
    const gfx::VectorIcon& icon_asset;
  };

  bool IsValidIndex(size_t index) override;
  size_t GetIndexForLetterSpacing(
      read_anything::mojom::LetterSpacing letter_spacing);
  read_anything::mojom::LetterSpacing GetLetterSpacingAt(size_t index);

 private:
  // Letter spacing choices for the drop down options in front-end.
  std::vector<LetterSpacingInfo> letters_choices_;
};

///////////////////////////////////////////////////////////////////////////////
// ReadAnythingModel
//
//  A class that stores data for the Read Anything feature.
//  This class is owned by the ReadAnythingCoordinator and has the same lifetime
//  as the browser.
//
class ReadAnythingModel {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void AccessibilityEventReceived(
        const content::AXEventNotificationDetails& details) {}
    virtual void OnActiveAXTreeIDChanged(const ui::AXTreeID& tree_id,
                                         const ukm::SourceId& ukm_source_id) {}
    virtual void OnAXTreeDestroyed(const ui::AXTreeID& tree_id) {}
    virtual void OnReadAnythingThemeChanged(
        const std::string& font_name,
        double font_scale,
        ui::ColorId foreground_color_id,
        ui::ColorId background_color_id,
        ui::ColorId separator_color_id,
        read_anything::mojom::LineSpacing line_spacing,
        read_anything::mojom::LetterSpacing letter_spacing) = 0;
#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
    virtual void ScreenAIServiceReady() {}
#endif
  };

  ReadAnythingModel();
  ReadAnythingModel(const ReadAnythingModel&) = delete;
  ReadAnythingModel& operator=(const ReadAnythingModel&) = delete;
  ~ReadAnythingModel();

  void Init(const std::string& font_name,
            double font_scale,
            read_anything::mojom::Colors colors,
            read_anything::mojom::LineSpacing line_spacing,
            read_anything::mojom::LetterSpacing letter_spacing);

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

  void AccessibilityEventReceived(
      const content::AXEventNotificationDetails& details);
  void OnActiveAXTreeIDChanged(const ui::AXTreeID& tree_id,
                               const ukm::SourceId& ukm_source_id);
  void OnAXTreeDestroyed(const ui::AXTreeID& tree_id);
#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
  void ScreenAIServiceReady();
#endif

  void SetSelectedFontByIndex(size_t new_index);
  double GetValidFontScale(double font_scale);
  void DecreaseTextSize();
  void IncreaseTextSize();
  void SetSelectedColorsByIndex(size_t new_index);
  void SetSelectedLineSpacingByIndex(size_t new_index);
  void SetSelectedLetterSpacingByIndex(size_t new_index);

  ReadAnythingFontModel* GetFontModel() { return font_model_.get(); }
  double GetFontScale() { return font_scale_; }
  ReadAnythingColorsModel* GetColorsModel() { return colors_model_.get(); }
  ReadAnythingLineSpacingModel* GetLineSpacingModel() {
    return line_spacing_model_.get();
  }
  ReadAnythingLetterSpacingModel* GetLetterSpacingModel() {
    return letter_spacing_model_.get();
  }

 private:
  void NotifyThemeChanged();

  // State:

  // Members of read_anything::mojom::ReadAnythingTheme:
  std::string font_name_ = kReadAnythingDefaultFontName;
  ui::ColorId foreground_color_id_ = kColorReadAnythingForeground;
  ui::ColorId background_color_id_ = kColorReadAnythingBackground;
  ui::ColorId separator_color_id_ = kColorReadAnythingSeparator;

  // A scale multiplier for font size (internal use only, not shown to user).
  float font_scale_ = kReadAnythingDefaultFontScale;

  read_anything::mojom::LineSpacing line_spacing_ = LineSpacing::kDefaultValue;
  read_anything::mojom::LetterSpacing letter_spacing_ =
      LetterSpacing::kDefaultValue;

  // Currently selected index for colors combobox
  int colors_combobox_index_ = 0;

  base::ObserverList<Observer> observers_;
  const std::unique_ptr<ReadAnythingFontModel> font_model_;
  const std::unique_ptr<ReadAnythingColorsModel> colors_model_;
  const std::unique_ptr<ReadAnythingLineSpacingModel> line_spacing_model_;
  const std::unique_ptr<ReadAnythingLetterSpacingModel> letter_spacing_model_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_READ_ANYTHING_READ_ANYTHING_MODEL_H_
