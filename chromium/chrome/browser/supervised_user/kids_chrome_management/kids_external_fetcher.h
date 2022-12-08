// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_KIDS_CHROME_MANAGEMENT_KIDS_EXTERNAL_FETCHER_H_
#define CHROME_BROWSER_SUPERVISED_USER_KIDS_CHROME_MANAGEMENT_KIDS_EXTERNAL_FETCHER_H_

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_piece.h"
#include "chrome/browser/supervised_user/kids_chrome_management/kids_access_token_fetcher.h"
#include "chrome/browser/supervised_user/kids_chrome_management/kidschromemanagement_messages.pb.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

// Holds the status of the fetch. The callback's response will be set iff the
// status is ok.
class KidsExternalFetcherStatus {
 public:
  enum State {
    NO_ERROR,                   // No error.
    GOOGLE_SERVICE_AUTH_ERROR,  // Error occurred during the access token
                                // fetching phase. See GetGoogleServiceAuthError
                                // for details.
    HTTP_ERROR,        // The request was performed, but http returned errors.
    INVALID_RESPONSE,  // The request was performed without error, but http
                       // response could not be processed or was unexpected.
  };
  // Status might be used in base::expected context as possible error, since it
  // contains two error-enabled attributes which are copyable / assignable.
  KidsExternalFetcherStatus(const KidsExternalFetcherStatus&);
  KidsExternalFetcherStatus& operator=(const KidsExternalFetcherStatus&);

  ~KidsExternalFetcherStatus();
  KidsExternalFetcherStatus() = delete;

  // Convenience creators instead of exposing KidsExternalFetcherStatus(State
  // state).
  static KidsExternalFetcherStatus Ok();
  static KidsExternalFetcherStatus GoogleServiceAuthError(
      GoogleServiceAuthError
          error);  // The copy follows the interface of
                   // https://source.chromium.org/chromium/chromium/src/+/main:components/signin/public/identity_manager/primary_account_access_token_fetcher.h;l=241;drc=8ba1bad80dc22235693a0dd41fe55c0fd2dbdabd
  static KidsExternalFetcherStatus HttpError();
  static KidsExternalFetcherStatus InvalidResponse();

  // KidsExternalFetcherStatus::IsOk iff google_service_auth_error_.state() ==
  // NONE and state_ == NONE
  bool IsOk() const;

  State state() const;
  const class GoogleServiceAuthError& google_service_auth_error() const;

 private:
  // Disallows impossible states.
  explicit KidsExternalFetcherStatus(State state);
  explicit KidsExternalFetcherStatus(
      class GoogleServiceAuthError
          google_service_auth_error);  // Implies State ==
                                       // GOOGLE_SERVICE_AUTH_ERROR
  KidsExternalFetcherStatus(
      State state,
      class GoogleServiceAuthError google_service_auth_error);

  State state_;
  class GoogleServiceAuthError google_service_auth_error_;
};

// Use instance of Fetcher to start request and write the result onto the
// receiving delegate. Every instance of Fetcher is disposable and should be
// used only once.
template <typename Request, typename Response>
class KidsExternalFetcher {
 public:
  using Callback = base::OnceCallback<void(KidsExternalFetcherStatus,
                                           std::unique_ptr<Response>)>;
  virtual ~KidsExternalFetcher() = default;
};

// Creates a disposable instance of an access token consumer that will fetch
// list of family members.
std::unique_ptr<
    KidsExternalFetcher<kids_chrome_management::ListFamilyMembersRequest,
                        kids_chrome_management::ListFamilyMembersResponse>>
FetchListFamilyMembers(
    signin::IdentityManager& identity_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    base::StringPiece url,
    KidsExternalFetcher<
        kids_chrome_management::ListFamilyMembersRequest,
        kids_chrome_management::ListFamilyMembersResponse>::Callback callback);

#endif  // CHROME_BROWSER_SUPERVISED_USER_KIDS_CHROME_MANAGEMENT_KIDS_EXTERNAL_FETCHER_H_
