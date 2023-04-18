// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/privacy/privacy_indicators_controller.h"

#include <string>

#include "ash/constants/ash_constants.h"
#include "ash/constants/ash_features.h"
#include "ash/public/cpp/notification_utils.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/notification_center/notification_center_tray.h"
#include "ash/system/privacy/privacy_indicators_tray_item_view.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/unified/unified_system_tray.h"
#include "base/functional/callback_forward.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"

namespace ash {

PrivacyIndicatorsNotificationDelegate::PrivacyIndicatorsNotificationDelegate(
    absl::optional<base::RepeatingClosure> launch_app,
    absl::optional<base::RepeatingClosure> launch_settings)
    : launch_app_callback_(launch_app),
      launch_settings_callback_(launch_settings) {
  UpdateButtonIndices();
}

PrivacyIndicatorsNotificationDelegate::
    ~PrivacyIndicatorsNotificationDelegate() = default;

void PrivacyIndicatorsNotificationDelegate::SetLaunchAppCallback(
    const base::RepeatingClosure& launch_app) {
  launch_app_callback_ = launch_app;
  UpdateButtonIndices();
}

void PrivacyIndicatorsNotificationDelegate::SetLaunchSettingsCallback(
    const base::RepeatingClosure& launch_settings) {
  launch_settings_callback_ = launch_settings;
  UpdateButtonIndices();
}

void PrivacyIndicatorsNotificationDelegate::Click(
    const absl::optional<int>& button_index,
    const absl::optional<std::u16string>& reply) {
  // Click on the notification body is no-op.
  if (!button_index)
    return;

  if (*button_index == launch_app_button_index_) {
    DCHECK(launch_app_callback_);
    launch_app_callback_->Run();
    return;
  }

  if (*button_index == launch_settings_button_index_) {
    DCHECK(launch_settings_callback_);
    launch_settings_callback_->Run();
  }
}

void PrivacyIndicatorsNotificationDelegate::UpdateButtonIndices() {
  int current_index = 0;
  if (launch_app_callback_) {
    launch_app_button_index_ = current_index++;
  }

  if (launch_settings_callback_) {
    launch_settings_button_index_ = current_index;
  }
}

void ASH_EXPORT UpdatePrivacyIndicators(
    const std::string& app_id,
    absl::optional<std::u16string> app_name,
    bool is_camera_used,
    bool is_microphone_used,
    scoped_refptr<PrivacyIndicatorsNotificationDelegate> delegate) {
  ModifyPrivacyIndicatorsNotification(app_id, app_name, is_camera_used,
                                      is_microphone_used, delegate);
  UpdatePrivacyIndicatorsView(app_id, is_camera_used, is_microphone_used);
}

std::string GetPrivacyIndicatorsNotificationId(const std::string& app_id) {
  return kPrivacyIndicatorsNotificationIdPrefix + app_id;
}

std::unique_ptr<message_center::Notification>
CreatePrivacyIndicatorsNotification(
    const std::string& app_id,
    absl::optional<std::u16string> app_name,
    bool is_camera_used,
    bool is_microphone_used,
    scoped_refptr<PrivacyIndicatorsNotificationDelegate> delegate) {
  std::u16string app_name_str = app_name.value_or(l10n_util::GetStringUTF16(
      IDS_PRIVACY_NOTIFICATION_MESSAGE_DEFAULT_APP_NAME));

  std::u16string title;
  std::u16string message;
  const gfx::VectorIcon* app_icon;
  if (is_camera_used && is_microphone_used) {
    title = l10n_util::GetStringUTF16(
        IDS_PRIVACY_NOTIFICATION_TITLE_CAMERA_AND_MIC);
    message = l10n_util::GetStringFUTF16(
        IDS_PRIVACY_NOTIFICATION_MESSAGE_CAMERA_AND_MIC, app_name_str);
    app_icon = &kPrivacyIndicatorsIcon;
  } else if (is_camera_used) {
    title = l10n_util::GetStringUTF16(IDS_PRIVACY_NOTIFICATION_TITLE_CAMERA);
    message = l10n_util::GetStringFUTF16(
        IDS_PRIVACY_NOTIFICATION_MESSAGE_CAMERA, app_name_str);
    app_icon = &kPrivacyIndicatorsCameraIcon;
  } else {
    title = l10n_util::GetStringUTF16(IDS_PRIVACY_NOTIFICATION_TITLE_MIC);
    message = l10n_util::GetStringFUTF16(IDS_PRIVACY_NOTIFICATION_MESSAGE_MIC,
                                         app_name_str);
    app_icon = &kPrivacyIndicatorsMicrophoneIcon;
  }

  message_center::RichNotificationData optional_fields;
  optional_fields.pinned = true;
  // Make the notification low priority so that it is silently added (no popup).
  optional_fields.priority = message_center::LOW_PRIORITY;

  optional_fields.parent_vector_small_image = &kPrivacyIndicatorsIcon;

  if (delegate->launch_app_callback()) {
    optional_fields.buttons.emplace_back(
        l10n_util::GetStringUTF16(IDS_PRIVACY_NOTIFICATION_BUTTON_APP_LAUNCH));
  }

  if (delegate->launch_settings_callback()) {
    optional_fields.buttons.emplace_back(l10n_util::GetStringUTF16(
        IDS_PRIVACY_NOTIFICATION_BUTTON_APP_SETTINGS));
  }

  auto notification = CreateSystemNotificationPtr(
      message_center::NotificationType::NOTIFICATION_TYPE_SIMPLE,
      GetPrivacyIndicatorsNotificationId(app_id), title, message,
      /*display_source=*/std::u16string(),
      /*origin_url=*/GURL(),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kPrivacyIndicatorsNotifierId,
                                 NotificationCatalogName::kPrivacyIndicators),
      optional_fields,
      /*delegate=*/delegate, *app_icon,
      message_center::SystemNotificationWarningLevel::NORMAL);

  notification->set_accent_color_id(ui::kColorAshPrivacyIndicatorsBackground);

  return notification;
}

void ModifyPrivacyIndicatorsNotification(
    const std::string& app_id,
    absl::optional<std::u16string> app_name,
    bool is_camera_used,
    bool is_microphone_used,
    scoped_refptr<PrivacyIndicatorsNotificationDelegate> delegate) {
  // With `features::kVideoConference` enabled, the tray serves as this
  // notifier, so do not show these notifications.
  if (features::IsVideoConferenceEnabled()) {
    return;
  }
  auto* message_center = message_center::MessageCenter::Get();
  std::string id = GetPrivacyIndicatorsNotificationId(app_id);
  bool notification_exists = message_center->FindVisibleNotificationById(id);

  if (!is_camera_used && !is_microphone_used) {
    if (notification_exists)
      message_center->RemoveNotification(id, /*by_user=*/false);
    return;
  }

  auto notification = CreatePrivacyIndicatorsNotification(
      app_id, app_name, is_camera_used, is_microphone_used, delegate);
  if (notification_exists) {
    message_center->UpdateNotification(id, std::move(notification));
    return;
  }
  message_center->AddNotification(std::move(notification));
}

void UpdatePrivacyIndicatorsView(const std::string& app_id,
                                 bool is_camera_used,
                                 bool is_microphone_used) {
  // Privacy indicators don't exist when video conference controls are enabled.
  if (!features::IsPrivacyIndicatorsEnabled() ||
      features::IsVideoConferenceEnabled()) {
    return;
  }
  DCHECK(Shell::HasInstance());
  for (auto* root_window_controller :
       Shell::Get()->GetAllRootWindowControllers()) {
    DCHECK(root_window_controller);
    auto* status_area_widget = root_window_controller->GetStatusAreaWidget();
    DCHECK(status_area_widget);

    auto* privacy_indicators_view =
        features::IsQsRevampEnabled()
            ? status_area_widget->notification_center_tray()
                  ->privacy_indicators_view()
            : status_area_widget->unified_system_tray()
                  ->privacy_indicators_view();

    DCHECK(privacy_indicators_view);

    privacy_indicators_view->Update(app_id, is_camera_used, is_microphone_used);
  }
}

void UpdatePrivacyIndicatorsScreenShareStatus(bool is_screen_sharing) {
  if (!features::IsPrivacyIndicatorsEnabled())
    return;

  DCHECK(Shell::HasInstance());
  for (auto* root_window_controller :
       Shell::Get()->GetAllRootWindowControllers()) {
    DCHECK(root_window_controller);
    auto* status_area_widget = root_window_controller->GetStatusAreaWidget();
    DCHECK(status_area_widget);

    auto* privacy_indicators_view =
        features::IsQsRevampEnabled()
            ? status_area_widget->notification_center_tray()
                  ->privacy_indicators_view()
            : status_area_widget->unified_system_tray()
                  ->privacy_indicators_view();

    DCHECK(privacy_indicators_view);

    privacy_indicators_view->UpdateScreenShareStatus(is_screen_sharing);
  }
}

}  // namespace ash