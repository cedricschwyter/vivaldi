// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved

#include "sync/vivaldi_sync_auth_manager.h"

#include <string>

#include "sync/vivaldi_syncmanager.h"
#include "vivaldi_account/vivaldi_account_manager.h"

namespace vivaldi {

namespace {
GoogleServiceAuthError ToGoogleServiceAuthError(
    VivaldiAccountManager::FetchError error) {
  switch (error.type) {
    case VivaldiAccountManager::NONE:
      return GoogleServiceAuthError(GoogleServiceAuthError::NONE);
    case VivaldiAccountManager::NETWORK_ERROR:
      return GoogleServiceAuthError::FromConnectionError(error.error_code);
    case VivaldiAccountManager::SERVER_ERROR:
      return GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR);
    case VivaldiAccountManager::INVALID_CREDENTIALS:
      return GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
          GoogleServiceAuthError::InvalidGaiaCredentialsReason::
              CREDENTIALS_REJECTED_BY_SERVER);
    default:
      NOTREACHED();
  }
}

AccountInfo ToChromiumAccountInfo(
    VivaldiAccountManager::AccountInfo account_info) {
  AccountInfo chromium_account_info;
  // Email is the closest thing to a username that the chromium account info
  // takes. It isn't really used for anything else than disply purposes anyway.
  chromium_account_info.email = account_info.username;
  chromium_account_info.account_id = account_info.account_id;
  chromium_account_info.picture_url = account_info.picture_url;

  return chromium_account_info;
}
}  // anonymous namespace

VivaldiSyncAuthManager::VivaldiSyncAuthManager(
    syncer::SyncPrefs* sync_prefs,
    identity::IdentityManager* identity_manager,
    const AccountStateChangedCallback& account_state_changed,
    const CredentialsChangedCallback& credentials_changed,
    VivaldiAccountManager* account_manager)
    : SyncAuthManager(sync_prefs,
                      identity_manager,
                      account_state_changed,
                      credentials_changed),
      account_manager_(account_manager) {}

VivaldiSyncAuthManager::~VivaldiSyncAuthManager() {
  if (registered_for_account_notifications_)
    account_manager_->RemoveObserver(this);
}

void VivaldiSyncAuthManager::RegisterForAuthNotifications() {
  account_manager_->AddObserver(this);
  registered_for_account_notifications_ = true;

  account_info_ = ToChromiumAccountInfo(account_manager_->account_info());
}

syncer::SyncAccountInfo VivaldiSyncAuthManager::GetActiveAccountInfo() const {
  if (!registered_for_account_notifications_)
    return syncer::SyncAccountInfo();

  return syncer::SyncAccountInfo(account_info_, true);
}

syncer::SyncTokenStatus VivaldiSyncAuthManager::GetSyncTokenStatus() const {
  syncer::SyncTokenStatus token_status;
  token_status.connection_status_update_time =
      partial_token_status_.connection_status_update_time;
  token_status.connection_status = partial_token_status_.connection_status;
  token_status.token_request_time = account_manager_->GetTokenRequestTime();
  token_status.token_receive_time = account_manager_->token_received_time();
  token_status.has_token = !account_manager_->access_token().empty();
  token_status.next_token_request_time =
      account_manager_->GetNextTokenRequestTime();
  token_status.last_get_token_error =
      ToGoogleServiceAuthError(account_manager_->last_token_fetch_error());

  return token_status;
}

void VivaldiSyncAuthManager::ConnectionStatusChanged(
    syncer::ConnectionStatus status) {
  partial_token_status_.connection_status_update_time = base::Time::Now();
  partial_token_status_.connection_status = status;

  switch (status) {
    case syncer::CONNECTION_AUTH_ERROR:
      access_token_.clear();
      account_manager_->RequestNewToken();
      break;
    case syncer::CONNECTION_OK:
      last_auth_error_ = GoogleServiceAuthError::AuthErrorNone();
      break;
    case syncer::CONNECTION_SERVER_ERROR:
      last_auth_error_ =
          GoogleServiceAuthError(GoogleServiceAuthError::CONNECTION_FAILED);
      break;
    case syncer::CONNECTION_NOT_ATTEMPTED:
      // The connection status should never change to "not attempted".
      NOTREACHED();
      break;
  }
}

void VivaldiSyncAuthManager::OnVivaldiAccountUpdated() {
  AccountInfo new_account =
      ToChromiumAccountInfo(account_manager_->account_info());
  if (new_account.account_id == account_info_.account_id)
    return;

  if (!account_info_.IsEmpty()) {
    account_info_ = AccountInfo();
    Clear();
    account_state_changed_callback_.Run();
  }

  if (!new_account.IsEmpty()) {
    account_info_ = new_account;
    account_state_changed_callback_.Run();
  }
}

void VivaldiSyncAuthManager::OnTokenFetchSucceeded() {
  last_auth_error_ = GoogleServiceAuthError::AuthErrorNone();
  access_token_ = account_manager_->access_token();
  credentials_changed_callback_.Run();
}

void VivaldiSyncAuthManager::OnTokenFetchFailed() {
  if (account_manager_->last_token_fetch_error().type !=
      VivaldiAccountManager::INVALID_CREDENTIALS)
    return;

  sync_prefs_->SetSyncAuthError(true);
  last_auth_error_ =
      ToGoogleServiceAuthError(account_manager_->last_token_fetch_error());
  credentials_changed_callback_.Run();
}

void VivaldiSyncAuthManager::OnVivaldiAccountShutdown() {}

}  // namespace vivaldi
