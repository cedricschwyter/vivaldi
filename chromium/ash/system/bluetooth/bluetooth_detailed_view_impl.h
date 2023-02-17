// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BLUETOOTH_BLUETOOTH_DETAILED_VIEW_IMPL_H_
#define ASH_SYSTEM_BLUETOOTH_BLUETOOTH_DETAILED_VIEW_IMPL_H_

#include "ash/ash_export.h"
#include "ash/system/bluetooth/bluetooth_detailed_view.h"
#include "ash/system/tray/tray_detailed_view.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace views {
class Button;
class ImageView;
class ToggleButton;
}  // namespace views

namespace ash {

class DetailedViewDelegate;
class HoverHighlightView;
class RoundedContainer;

// The implementation of BluetoothDetailedView.
class ASH_EXPORT BluetoothDetailedViewImpl : public BluetoothDetailedView,
                                             public TrayDetailedView {
 public:
  METADATA_HEADER(BluetoothDetailedViewImpl);

  BluetoothDetailedViewImpl(DetailedViewDelegate* detailed_view_delegate,
                            BluetoothDetailedView::Delegate* delegate);
  BluetoothDetailedViewImpl(const BluetoothDetailedViewImpl&) = delete;
  BluetoothDetailedViewImpl& operator=(const BluetoothDetailedViewImpl&) =
      delete;
  ~BluetoothDetailedViewImpl() override;

  // BluetoothDetailedView:
  views::View* GetAsView() override;
  void UpdateBluetoothEnabledState(bool enabled) override;
  BluetoothDeviceListItemView* AddDeviceListItem() override;
  views::View* AddDeviceListSubHeader(const gfx::VectorIcon& icon,
                                      int text_id) override;
  void NotifyDeviceListChanged() override;
  views::View* device_list() override;

  // TrayDetailedView:
  void HandleViewClicked(views::View* view) override;

 private:
  friend class BluetoothDetailedViewImplTest;

  // Creates and configures the title section settings button.
  void CreateTitleSettingsButton();

  // Creates the top rounded container, which contains the main on/off toggle.
  void CreateTopContainer();

  // Creates the main rounded container, which holds the pair new device row
  // and the device list.
  void CreateMainContainer();

  // Attempts to close the quick settings and open the Bluetooth settings.
  void OnSettingsClicked();

  // Handles clicks on the Bluetooth toggle button.
  void OnToggleClicked();

  // Handles toggling Bluetooth via the UI to `new_state`.
  void ToggleBluetoothState(bool new_state);

  // Owned by views hierarchy.
  views::Button* settings_button_ = nullptr;
  RoundedContainer* top_container_ = nullptr;
  HoverHighlightView* toggle_row_ = nullptr;
  views::ImageView* toggle_icon_ = nullptr;
  views::ToggleButton* toggle_button_ = nullptr;
  RoundedContainer* main_container_ = nullptr;
  HoverHighlightView* pair_new_device_view_ = nullptr;
  views::ImageView* pair_new_device_icon_ = nullptr;
  views::View* device_list_ = nullptr;

  base::WeakPtrFactory<BluetoothDetailedViewImpl> weak_factory_{this};
};

}  // namespace ash

#endif  // ASH_SYSTEM_BLUETOOTH_BLUETOOTH_DETAILED_VIEW_IMPL_H_
