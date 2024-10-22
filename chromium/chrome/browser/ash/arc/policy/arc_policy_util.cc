// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/arc/policy/arc_policy_util.h"

#include <memory>

#include "ash/components/arc/arc_prefs.h"
#include "ash/constants/ash_switches.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/ash/policy/handlers/configuration_policy_handler_ash.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/policy/proto/cloud_policy.pb.h"
#include "components/prefs/pref_value_map.h"

namespace arc {
namespace policy_util {

namespace {

// Constants used to parse ARC++ JSON policy.
constexpr char kApplicationsKey[] = "applications";
constexpr char kInstallTypeKey[] = "installType";
constexpr char kPackageNameKey[] = "packageName";
constexpr char kInstallTypeRequired[] = "REQUIRED";
constexpr char kInstallTypeForceInstalled[] = "FORCE_INSTALLED";

}  // namespace

bool IsAccountManaged(const Profile* profile) {
  return profile->GetProfilePolicyConnector()->IsManaged();
}

bool IsArcDisabledForEnterprise() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      ash::switches::kEnterpriseDisableArc);
}

std::set<std::string> GetRequestedPackagesFromArcPolicy(
    const std::string& arc_policy) {
  absl::optional<base::Value> dict = ParsePolicyJson(arc_policy);
  if (!dict.has_value() || !dict.value().is_dict()) {
    return {};
  }

  std::map<std::string, std::set<std::string>> install_type_map =
      CreateInstallTypeMap(dict.value());
  std::set<std::string> required_packages =
      install_type_map[kInstallTypeRequired];
  std::set<std::string> force_installed_packages =
      install_type_map[kInstallTypeForceInstalled];

  // Only required and force installed packages are considered "requested"
  std::set<std::string> requested_packages;
  requested_packages.insert(required_packages.begin(), required_packages.end());
  requested_packages.insert(force_installed_packages.begin(),
                            force_installed_packages.end());
  return requested_packages;
}

void RecordPolicyMetrics(const std::string& arc_policy) {
  absl::optional<base::Value> dict = ParsePolicyJson(arc_policy);
  if (!dict.has_value() || !dict.value().is_dict()) {
    return;
  }

  for (const auto it : dict.value().DictItems()) {
    UMA_HISTOGRAM_ENUMERATION("Arc.Policy.Keys",
                              GetPolicyKeyFromString(it.first));
  }

  std::map<std::string, std::set<std::string>> install_type_map =
      CreateInstallTypeMap(dict.value());

  for (auto& it : install_type_map) {
    InstallType install_type_enum = GetInstallTypeEnumFromString(it.first);
    UMA_HISTOGRAM_ENUMERATION("Arc.Policy.InstallTypesOnDevice",
                              install_type_enum);
  }
}

absl::optional<base::Value> ParsePolicyJson(const std::string& arc_policy) {
  return base::JSONReader::Read(
      arc_policy, base::JSONParserOptions::JSON_ALLOW_TRAILING_COMMAS);
}

std::map<std::string, std::set<std::string>> CreateInstallTypeMap(
    const base::Value& dict) {
  const base::Value* const packages =
      dict.FindKeyOfType(kApplicationsKey, base::Value::Type::LIST);
  if (!packages)
    return {};

  std::map<std::string, std::set<std::string>> install_type_map;
  for (const auto& package : packages->GetList()) {
    if (!package.is_dict())
      continue;
    const base::Value* const install_type =
        package.FindKeyOfType(kInstallTypeKey, base::Value::Type::STRING);
    if (!install_type)
      continue;

    const base::Value* const package_name =
        package.FindKeyOfType(kPackageNameKey, base::Value::Type::STRING);
    if (!package_name || package_name->GetString().empty())
      continue;
    install_type_map[install_type->GetString()].insert(
        package_name->GetString());
  }
  return install_type_map;
}

ArcPolicyKey GetPolicyKeyFromString(const std::string& policy_key) {
  if (policy_key == "accountTypesWithManagementDisabled") {
    return ArcPolicyKey::kAccountTypesWithManagementDisabled;
  } else if (policy_key == "alwaysOnVpnPackage") {
    return ArcPolicyKey::kAlwaysOnVpnPackage;
  } else if (policy_key == "applications") {
    return ArcPolicyKey::kApplications;
  } else if (policy_key == "availableAppSetPolicy") {
    return ArcPolicyKey::kAvailableAppSetPolicy;
  } else if (policy_key == "complianceRules") {
    return ArcPolicyKey::kComplianceRules;
  } else if (policy_key == "installUnknownSourcesDisabled") {
    return ArcPolicyKey::kInstallUnknownSourcesDisabled;
  } else if (policy_key == "maintenanceWindow") {
    return ArcPolicyKey::kMaintenanceWindow;
  } else if (policy_key == "modifyAccountsDisabled") {
    return ArcPolicyKey::kModifyAccountsDisabled;
  } else if (policy_key == "permissionGrants") {
    return ArcPolicyKey::kPermissionGrants;
  } else if (policy_key == "permittedAccessibilityServices") {
    return ArcPolicyKey::kPermittedAccessibilityServices;
  } else if (policy_key == "playStoreMode") {
    return ArcPolicyKey::kPlayStoreMode;
  } else if (policy_key == "shortSupportMessage") {
    return ArcPolicyKey::kShortSupportMessage;
  } else if (policy_key == "statusReportingSettings") {
    return ArcPolicyKey::kStatusReportingSettings;
  } else if (policy_key == "workAccountAppWhitelist") {
    return ArcPolicyKey::kWorkAccountAppWhitelist;
  }

  LOG(WARNING) << "Unknown policy key: " << policy_key;
  return ArcPolicyKey::kUnknown;
}

InstallType GetInstallTypeEnumFromString(const std::string& install_type) {
  if (install_type == "OPTIONAL") {
    return InstallType::kOptional;
  } else if (install_type == "REQUIRED") {
    return InstallType::kRequired;
  } else if (install_type == "PRELOAD") {
    return InstallType::kPreload;
  } else if (install_type == "FORCE_INSTALLED") {
    return InstallType::kForceInstalled;
  } else if (install_type == "BLOCKED") {
    return InstallType::kBlocked;
  } else if (install_type == "AVAILABLE") {
    return InstallType::kAvailable;
  } else if (install_type == "REQUIRED_FOR_SETUP") {
    return InstallType::kRequiredForSetup;
  } else if (install_type == "KIOSK") {
    return InstallType::kKiosk;
  }

  LOG(WARNING) << "Unknown app install type in the policy: " << install_type;
  return InstallType::kUnknown;
}

}  // namespace policy_util
}  // namespace arc
