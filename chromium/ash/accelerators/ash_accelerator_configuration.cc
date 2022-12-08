// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/ash_accelerator_configuration.h"

#include <vector>

#include "ash/accelerators/accelerator_layout_table.h"
#include "ash/accelerators/accelerator_table.h"
#include "ash/accelerators/debug_commands.h"
#include "ash/constants/ash_features.h"
#include "ash/public/cpp/accelerators.h"
#include "ash/public/cpp/accelerators_util.h"
#include "ash/public/mojom/accelerator_info.mojom.h"
#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"

namespace {

void AppendAcceleratorData(
    std::vector<ash::AcceleratorData>& data,
    base::span<const ash::AcceleratorData> accelerators) {
  data.reserve(data.size() + accelerators.size());
  for (const auto& accelerator : accelerators) {
    data.push_back(accelerator);
  }
}

}  // namespace

namespace ash {

AshAcceleratorConfiguration::AshAcceleratorConfiguration()
    : AcceleratorConfiguration(ash::mojom::AcceleratorSource::kAsh) {}
AshAcceleratorConfiguration::~AshAcceleratorConfiguration() = default;

// TODO(jimmyxgong): Implement all functions below as these are only stubs.

const std::vector<mojom::AcceleratorLayoutInfoPtr>&
AshAcceleratorConfiguration::GetAcceleratorLayoutInfos() {
  return layout_infos_;
}

const std::vector<AcceleratorInfo>&
AshAcceleratorConfiguration::GetConfigForAction(AcceleratorActionId action_id) {
  const auto accelerator_iter = id_to_accelerator_infos_.find(action_id);
  DCHECK(accelerator_iter != id_to_accelerator_infos_.end());

  return accelerator_iter->second;
}

bool AshAcceleratorConfiguration::IsMutable() const {
  return false;
}

AcceleratorConfigResult AshAcceleratorConfiguration::AddUserAccelerator(
    AcceleratorActionId action_id,
    const ui::Accelerator& accelerator) {
  return AcceleratorConfigResult::kActionLocked;
}

AcceleratorConfigResult AshAcceleratorConfiguration::RemoveAccelerator(
    AcceleratorActionId action_id,
    const ui::Accelerator& accelerator) {
  return AcceleratorConfigResult::kActionLocked;
}

AcceleratorConfigResult AshAcceleratorConfiguration::ReplaceAccelerator(
    AcceleratorActionId action_id,
    const ui::Accelerator& old_acc,
    const ui::Accelerator& new_acc) {
  return AcceleratorConfigResult::kActionLocked;
}

AcceleratorConfigResult AshAcceleratorConfiguration::RestoreDefault(
    AcceleratorActionId action_id) {
  return AcceleratorConfigResult::kActionLocked;
}

AcceleratorConfigResult AshAcceleratorConfiguration::RestoreAllDefaults() {
  return AcceleratorConfigResult::kActionLocked;
}

void AshAcceleratorConfiguration::Initialize() {
  std::vector<AcceleratorData> accelerators;
  AppendAcceleratorData(
      accelerators, base::make_span(kAcceleratorData, kAcceleratorDataLength));

  if (::features::IsImprovedKeyboardShortcutsEnabled()) {
    AppendAcceleratorData(
        accelerators,
        base::make_span(kEnableWithPositionalAcceleratorsData,
                        kEnableWithPositionalAcceleratorsDataLength));
    if (ash::features::IsImprovedDesksKeyboardShortcutsEnabled()) {
      AppendAcceleratorData(
          accelerators,
          base::make_span(
              kEnabledWithImprovedDesksKeyboardShortcutsAcceleratorData,
              kEnabledWithImprovedDesksKeyboardShortcutsAcceleratorDataLength));
    }
  } else if (::features::IsNewShortcutMappingEnabled()) {
    AppendAcceleratorData(
        accelerators,
        base::make_span(kEnableWithNewMappingAcceleratorData,
                        kEnableWithNewMappingAcceleratorDataLength));
  } else {
    AppendAcceleratorData(
        accelerators,
        base::make_span(kDisableWithNewMappingAcceleratorData,
                        kDisableWithNewMappingAcceleratorDataLength));
  }

  // Debug accelerators.
  if (debug::DebugAcceleratorsEnabled()) {
    AppendAcceleratorData(
        accelerators,
        base::make_span(kDebugAcceleratorData, kDebugAcceleratorDataLength));
  }

  // Developer accelerators.
  if (debug::DeveloperAcceleratorsEnabled()) {
    AppendAcceleratorData(accelerators,
                          base::make_span(kDeveloperAcceleratorData,
                                          kDeveloperAcceleratorDataLength));
  }

  Initialize(accelerators);
  InitializeDeprecatedAccelerators();
}

void AshAcceleratorConfiguration::Initialize(
    base::span<const AcceleratorData> accelerators) {
  accelerator_infos_.clear();
  id_to_accelerator_infos_.clear();
  accelerator_to_id_.Clear();

  AddAccelerators(accelerators, mojom::AcceleratorType::kDefault);
}

void AshAcceleratorConfiguration::InitializeDeprecatedAccelerators() {
  base::span<const DeprecatedAcceleratorData> deprecated_accelerator_data(
      kDeprecatedAcceleratorsData, kDeprecatedAcceleratorsDataLength);
  base::span<const AcceleratorData> deprecated_accelerators(
      kDeprecatedAccelerators, kDeprecatedAcceleratorsLength);

  InitializeDeprecatedAccelerators(std::move(deprecated_accelerator_data),
                                   std::move(deprecated_accelerators));
}

void AshAcceleratorConfiguration::InitializeDeprecatedAccelerators(
    base::span<const DeprecatedAcceleratorData> deprecated_data,
    base::span<const AcceleratorData> deprecated_accelerators) {
  for (const auto& data : deprecated_data) {
    actions_with_deprecations_[data.action] = &data;
  }

  AddAccelerators(deprecated_accelerators, mojom::AcceleratorType::kDeprecated);
}

void AshAcceleratorConfiguration::AddAccelerators(
    base::span<const AcceleratorData> accelerators,
    mojom::AcceleratorType type) {
  for (const auto& data : accelerators) {
    ui::Accelerator accelerator(data.keycode, data.modifiers);
    accelerator.set_key_state(data.trigger_on_press
                                  ? ui::Accelerator::KeyState::PRESSED
                                  : ui::Accelerator::KeyState::RELEASED);
    // TODO(jimmyxgong): Ash accelerators should not be locked when
    // customization is allowed.
    AcceleratorInfo info(type, accelerator, KeycodeToKeyString(data.keycode),
                         /*locked=*/true);
    accelerator_to_id_.InsertNew(std::make_pair(accelerator, data.action));
    id_to_accelerator_infos_[static_cast<uint32_t>(data.action)].push_back(
        info);
    accelerator_infos_.push_back(info);
    AddLayoutInfo(data);
  }
  UpdateAccelerators(id_to_accelerator_infos_);
}

const DeprecatedAcceleratorData*
AshAcceleratorConfiguration::GetDeprecatedAcceleratorData(
    AcceleratorActionId action) {
  auto it = actions_with_deprecations_.find(action);
  if (it == actions_with_deprecations_.end()) {
    return nullptr;
  }
  return it->second;
}

bool AshAcceleratorConfiguration::IsDeprecated(
    const ui::Accelerator& accelerator) {
  const auto* action_id = FindAcceleratorAction(accelerator);
  // Not a registered accelerator, return false.
  if (!action_id) {
    return false;
  }

  const auto accelerators_iter = id_to_accelerator_infos_.find(*action_id);
  DCHECK(accelerators_iter != id_to_accelerator_infos_.end());

  for (auto const& info : accelerators_iter->second) {
    if (info.type == mojom::AcceleratorType::kDeprecated &&
        info.accelerator == accelerator) {
      return true;
    }
  }
  return false;
}

void AshAcceleratorConfiguration::AddLayoutInfo(const AcceleratorData& data) {
  const auto* layout_iter = kAcceleratorLayouts.find(data.action);
  DCHECK(layout_iter != kAcceleratorLayouts.end());
  const AcceleratorLayoutDetails& layout_details = layout_iter->second;

  mojom::AcceleratorLayoutInfoPtr layout_info =
      mojom::AcceleratorLayoutInfo::New();
  layout_info->category = layout_details.category;
  layout_info->sub_category = layout_details.sub_category;
  layout_info->description = l10n_util::GetStringUTF16(
      kAcceleratorActionToStringIdMap.at(data.action));
  layout_info->style = layout_details.layout_style;
  layout_info->source = mojom::AcceleratorSource::kAsh;
  layout_info->action = static_cast<uint32_t>(data.action);

  layout_infos_.push_back(std::move(layout_info));
}

}  // namespace ash
