// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/power_button.h"

#include "ash/constants/quick_settings_catalogs.h"
#include "ash/public/cpp/ash_view_ids.h"
#include "ash/public/cpp/session/session_controller.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller_impl.h"
#include "ash/shell.h"
#include "ash/shutdown_reason.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/style/icon_button.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/wm/lock_state_controller.h"
#include "base/i18n/rtl.h"
#include "chromeos/dbus/power/power_manager_client.h"
#include "quick_settings_metrics_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/chromeos/styles/cros_tokens_color_mappings.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace ash {

namespace {
// Rounded corner constants.
static constexpr int kRoundedCornerRadius = 16;
static constexpr int kNonRoundedCornerRadius = 4;

constexpr gfx::RoundedCornersF kBottomRightNonRoundedCorners(
    kRoundedCornerRadius,
    kRoundedCornerRadius,
    kNonRoundedCornerRadius,
    kRoundedCornerRadius);

constexpr gfx::RoundedCornersF kBottomLeftNonRoundedCorners(
    kRoundedCornerRadius,
    kRoundedCornerRadius,
    kRoundedCornerRadius,
    kNonRoundedCornerRadius);

constexpr gfx::RoundedCornersF kTopLeftNonRoundedCorners(
    kNonRoundedCornerRadius,
    kRoundedCornerRadius,
    kRoundedCornerRadius,
    kRoundedCornerRadius);

constexpr gfx::RoundedCornersF kTopRightNonRoundedCorners(
    kRoundedCornerRadius,
    kNonRoundedCornerRadius,
    kRoundedCornerRadius,
    kRoundedCornerRadius);

constexpr gfx::RoundedCornersF kAllRoundedCorners(kRoundedCornerRadius,
                                                  kRoundedCornerRadius,
                                                  kRoundedCornerRadius,
                                                  kRoundedCornerRadius);

// The highlight path generator for the `PowerButton`.
class HighlightPathGenerator : public views::HighlightPathGenerator {
 public:
  explicit HighlightPathGenerator(PowerButton* power_button)
      : power_button_(power_button) {}
  HighlightPathGenerator(const HighlightPathGenerator&) = delete;
  HighlightPathGenerator& operator=(const HighlightPathGenerator&) = delete;

 private:
  // HighlightPathGenerator:
  absl::optional<gfx::RRectF> GetRoundRect(const gfx::RectF& rect) override {
    gfx::RectF bounds(power_button_->GetLocalBounds());
    gfx::RoundedCornersF rounded = kAllRoundedCorners;
    if (power_button_->IsMenuShowing()) {
      // Don't need to check RTL since the `HighlightPathGenerator` will auto
      // adjust the shape for the RTL case.
      rounded = kTopLeftNonRoundedCorners;
    }

    return gfx::RRectF(bounds, rounded);
  }

  // Owned by views hierarchy.
  PowerButton* const power_button_;
};

}  // namespace

class PowerButton::MenuController : public ui::SimpleMenuModel::Delegate,
                                    public views::ContextMenuController {
 public:
  explicit MenuController(PowerButton* button) : power_button_(button) {}
  MenuController(const MenuController&) = delete;
  MenuController& operator=(const MenuController&) = delete;
  ~MenuController() override = default;

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override {
    switch (command_id) {
      case VIEW_ID_QS_POWER_OFF_MENU_BUTTON:
        quick_settings_metrics_util::RecordQsButtonActivated(
            QsButtonCatalogName::kPowerOffMenuButton);
        Shell::Get()->lock_state_controller()->StartShutdownAnimation(
            ShutdownReason::TRAY_SHUT_DOWN_BUTTON);
        break;
      case VIEW_ID_QS_POWER_SIGNOUT_MENU_BUTTON:
        quick_settings_metrics_util::RecordQsButtonActivated(
            QsButtonCatalogName::kPowerSignoutMenuButton);
        Shell::Get()->session_controller()->RequestSignOut();
        break;
      case VIEW_ID_QS_POWER_RESTART_MENU_BUTTON:
        quick_settings_metrics_util::RecordQsButtonActivated(
            QsButtonCatalogName::kPowerRestartMenuButton);
        chromeos::PowerManagerClient::Get()->RequestRestart(
            power_manager::REQUEST_RESTART_FOR_USER, "Reboot by user");
        break;
      case VIEW_ID_QS_POWER_LOCK_MENU_BUTTON:
        quick_settings_metrics_util::RecordQsButtonActivated(
            QsButtonCatalogName::kPowerLockMenuButton);
        Shell::Get()->session_controller()->LockScreen();
        break;
      default:
        NOTREACHED();
    }
  }

  // views::ContextMenuController:
  void ShowContextMenuForViewImpl(views::View* source,
                                  const gfx::Point& point,
                                  ui::MenuSourceType source_type) override {
    // Build the menu model and save it to `context_menu_model_`.
    BuildMenuModel();
    menu_model_adapter_ = std::make_unique<views::MenuModelAdapter>(
        context_menu_model_.get(),
        base::BindRepeating(&MenuController::OnMenuClosed,
                            base::Unretained(this)));
    root_menu_item_view_ = menu_model_adapter_->CreateMenu();
    int run_types = views::MenuRunner::USE_ASH_SYS_UI_LAYOUT |
                    views::MenuRunner::CONTEXT_MENU |
                    views::MenuRunner::FIXED_ANCHOR;
    menu_runner_ =
        std::make_unique<views::MenuRunner>(root_menu_item_view_, run_types);

    menu_runner_->RunMenuAt(source->GetWidget(), /*button_controller=*/nullptr,
                            source->GetBoundsInScreen(),
                            views::MenuAnchorPosition::kBubbleTopRight,
                            source_type, /*native_view_for_gestures=*/nullptr,
                            /*corners=*/
                            base::i18n::IsRTL() ? kBottomRightNonRoundedCorners
                                                : kBottomLeftNonRoundedCorners);
  }

  // Builds and saves a SimpleMenuModel to `context_menu_model_`;
  void BuildMenuModel() {
    // `context_menu_model_` and the other related pointers will be live for one
    // menu view's life cycle. This model will be built based on the use case
    // right before the menu view is shown. For example in the non-logged in
    // page, we only build power off and restart button.
    context_menu_model_ =
        std::make_unique<ui::SimpleMenuModel>(/*delegate=*/this);

    SessionControllerImpl* session_controller =
        Shell::Get()->session_controller();
    bool const is_on_login_screen =
        session_controller->login_status() == LoginStatus::NOT_LOGGED_IN;
    bool const can_show_settings = TrayPopupUtils::CanOpenWebUISettings();
    bool const can_lock_screen = session_controller->CanLockScreen();

    context_menu_model_->AddItemWithIcon(
        VIEW_ID_QS_POWER_OFF_MENU_BUTTON,
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_POWER_OFF),
        ui::ImageModel::FromVectorIcon(kSystemPowerButtonMenuPowerOffIcon,
                                       ui::kColorAshSystemUIMenuIcon,
                                       kTrayTopShortcutButtonIconSize));
    context_menu_model_->AddItemWithIcon(
        VIEW_ID_QS_POWER_RESTART_MENU_BUTTON,
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_REBOOT),
        ui::ImageModel::FromVectorIcon(kSystemPowerButtonMenuRestartIcon,
                                       ui::kColorAshSystemUIMenuIcon,
                                       kTrayTopShortcutButtonIconSize));
    if (!is_on_login_screen) {
      context_menu_model_->AddItemWithIcon(
          VIEW_ID_QS_POWER_SIGNOUT_MENU_BUTTON,
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SIGN_OUT),
          ui::ImageModel::FromVectorIcon(kSystemPowerButtonMenuSignOutIcon,
                                         ui::kColorAshSystemUIMenuIcon,
                                         kTrayTopShortcutButtonIconSize));
    }
    if (can_show_settings && can_lock_screen) {
      context_menu_model_->AddItemWithIcon(
          VIEW_ID_QS_POWER_LOCK_MENU_BUTTON,
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_LOCK),
          ui::ImageModel::FromVectorIcon(kSystemPowerButtonMenuLockScreenIcon,
                                         ui::kColorAshSystemUIMenuIcon,
                                         kTrayTopShortcutButtonIconSize));
    }
  }

  // Called when the context menu is closed. Used as a callback for
  // `menu_model_adapter_`.
  void OnMenuClosed() {
    menu_runner_.reset();
    context_menu_model_.reset();
    root_menu_item_view_ = nullptr;
    menu_model_adapter_.reset();
    power_button_->UpdateView();
  }

  // The context menu model and its adapter for `PowerButton`.
  std::unique_ptr<ui::SimpleMenuModel> context_menu_model_;
  std::unique_ptr<views::MenuModelAdapter> menu_model_adapter_;

  // The menu runner that is responsible to run the menu.
  std::unique_ptr<views::MenuRunner> menu_runner_;

  // The root menu item view of `context_menu_model_`. Cached for testing.
  views::MenuItemView* root_menu_item_view_ = nullptr;

  // Owned by views hierarchy.
  PowerButton* power_button_ = nullptr;
};

PowerButton::PowerButton()
    : background_view_(AddChildView(std::make_unique<View>())),
      button_content_(AddChildView(std::make_unique<IconButton>(
          base::BindRepeating(&PowerButton::OnButtonActivated,
                              base::Unretained(this)),
          IconButton::Type::kMediumFloating,
          &kUnifiedMenuPowerIcon,
          IDS_ASH_STATUS_TRAY_SHUTDOWN))),
      context_menu_(std::make_unique<MenuController>(/*button=*/this)) {
  SetID(VIEW_ID_QS_POWER_BUTTON);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  // Inits the `background_view_`'s layer. This view is `SetPaintToLayer` so it
  // can be set the customized rounded corner.
  background_view_->SetPaintToLayer(ui::LAYER_SOLID_COLOR);
  auto* background_layer = background_view_->layer();
  background_layer->SetRoundedCornerRadius(kAllRoundedCorners);
  background_layer->SetFillsBoundsOpaquely(false);
  background_layer->SetIsFastRoundedCorner(true);

  set_context_menu_controller(context_menu_.get());

  // Also paint the `IconButton` to a layer on top of the `background_view_`
  button_content_->SetPaintToLayer();
  button_content_->layer()->SetFillsBoundsOpaquely(false);

  // Installs the customized focus ring path generator for the button.
  button_content_->SetInstallFocusRingOnFocus(true);
  views::FocusRing::Get(button_content_)
      ->SetPathGenerator(
          std::make_unique<HighlightPathGenerator>(/*power_button=*/this));
  button_content_->SetFocusPainter(nullptr);
}

PowerButton::~PowerButton() = default;

bool PowerButton::IsMenuShowing() {
  auto* menu_runner = context_menu_->menu_runner_.get();
  return menu_runner && menu_runner->IsRunning();
}

void PowerButton::OnThemeChanged() {
  views::View::OnThemeChanged();

  button_content_->SetIconColor(
      GetColorProvider()->GetColor(cros_tokens::kCrosSysPrimary));
  SkColor inactive_color =
      GetColorProvider()->GetColor(cros_tokens::kCrosSysOnPrimary);
  SkColor active_color =
      GetColorProvider()->GetColor(cros_tokens::kCrosSysSystemPrimaryContainer);
  background_view_->layer()->SetColor(IsMenuShowing() ? active_color
                                                      : inactive_color);
}

void PowerButton::UpdateView() {
  UpdateRoundedCorners();
  OnThemeChanged();
  views::FocusRing* focus_ring = views::FocusRing::Get(button_content_);
  if (button_content_->HasFocus() && focus_ring) {
    // Updating the focus ring path, make sure the focus ring gets updated to
    // match this new state.
    focus_ring->InvalidateLayout();
    focus_ring->SchedulePaint();
  }
}

void PowerButton::UpdateRoundedCorners() {
  gfx::RoundedCornersF corners = kAllRoundedCorners;
  if (IsMenuShowing()) {
    corners = base::i18n::IsRTL() ? kTopRightNonRoundedCorners
                                  : kTopLeftNonRoundedCorners;
  }

  background_view_->layer()->SetRoundedCornerRadius(corners);
}

void PowerButton::OnButtonActivated(const ui::Event& event) {
  quick_settings_metrics_util::RecordQsButtonActivated(
      QsButtonCatalogName::kPowerButton);
  ui::MenuSourceType type;

  if (event.IsMouseEvent())
    type = ui::MENU_SOURCE_MOUSE;
  else if (event.IsTouchEvent())
    type = ui::MENU_SOURCE_TOUCH;
  else if (event.IsKeyEvent())
    type = ui::MENU_SOURCE_KEYBOARD;
  else
    type = ui::MENU_SOURCE_STYLUS;

  context_menu_->ShowContextMenuForView(
      /*source=*/this, GetBoundsInScreen().CenterPoint(), type);

  UpdateView();
}

views::MenuItemView* PowerButton::GetMenuViewForTesting() {
  return context_menu_->root_menu_item_view_;
}

}  // namespace ash
