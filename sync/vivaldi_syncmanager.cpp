// Copyright (c) 2015-2017 Vivaldi Technologies AS. All rights reserved

#include "sync/vivaldi_syncmanager.h"

#include <utility>

#include "components/browser_sync/sync_auth_manager.h"
#include "components/prefs/pref_service.h"
#include "components/sync/device_info/local_device_info_provider.h"
#include "prefs/vivaldi_gen_prefs.h"
#include "sync/vivaldi_sync_auth_manager.h"

namespace vivaldi {

VivaldiSyncManager::VivaldiSyncManager(
    ProfileSyncService::InitParams* init_params,
    Profile* profile,
    std::shared_ptr<VivaldiInvalidationService> invalidation_service,
    VivaldiAccountManager* account_manager)
    : ProfileSyncService(std::move(*init_params)),
      invalidation_service_(invalidation_service),
      ui_helper_(profile, this),
      weak_factory_(this) {
  auth_manager_ = std::make_unique<VivaldiSyncAuthManager>(
      &sync_prefs_, identity_manager_,
      base::BindRepeating(&VivaldiSyncManager::AccountStateChanged,
                          base::Unretained(this)),
      base::BindRepeating(&VivaldiSyncManager::CredentialsChanged,
                          base::Unretained(this)),
      account_manager);
}

VivaldiSyncManager::~VivaldiSyncManager() {}

void VivaldiSyncManager::ClearSyncData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!engine_)
    return;

  engine_->StartConfiguration();
  engine_->ClearServerData(base::BindRepeating(
      &VivaldiSyncManager::StopAndClear, weak_factory_.GetWeakPtr()));
  is_clearing_sync_data_ = true;

  NotifyObservers();
}

void VivaldiSyncManager::OnEngineInitialized(
    syncer::ModelTypeSet initial_types,
    const syncer::WeakHandle<syncer::JsBackend>& js_backend,
    const syncer::WeakHandle<syncer::DataTypeDebugInfoListener>&
        debug_info_listener,
    const std::string& cache_guid,
    const std::string& session_name,
    const std::string& birthday,
    const std::string& bag_of_chips,
    bool success) {
  std::string custom_session_name =
      sync_client_->GetPrefService()->GetString(vivaldiprefs::kSyncSessionName);
  ProfileSyncService::OnEngineInitialized(
      initial_types, js_backend, debug_info_listener, cache_guid,
      custom_session_name.empty() ? session_name : custom_session_name,
      birthday, bag_of_chips, success);
}

void VivaldiSyncManager::OnConfigureDone(
    const syncer::DataTypeManager::ConfigureResult& result) {
  if (IsFirstSetupComplete()) {
    // Extra paranoia, except for non-official builds where we might need
    // encyption off for debugging.
    if (!user_settings_->IsEncryptEverythingEnabled() &&
        version_info::IsOfficialBuild()) {
      StopAndClear();
      return;
    }
    ProfileSyncService::OnConfigureDone(result);
  }
}

void VivaldiSyncManager::ShutdownImpl(syncer::ShutdownReason reason) {
  if (reason == syncer::DISABLE_SYNC) {
    sync_client_->GetPrefService()->ClearPref(
        vivaldiprefs::kSyncIsUsingSeparateEncryptionPassword);
  }
  is_clearing_sync_data_ = false;
  ProfileSyncService::ShutdownImpl(reason);
}
}  // namespace vivaldi
