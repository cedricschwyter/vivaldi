// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/isolated_web_apps/install_isolated_web_app_command.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/sequence_checker.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "chrome/browser/web_applications/commands/web_app_command.h"
#include "chrome/browser/web_applications/isolated_web_apps/pending_install_info.h"
#include "chrome/browser/web_applications/isolation_data.h"
#include "chrome/browser/web_applications/locks/app_lock.h"
#include "chrome/browser/web_applications/os_integration/os_integration_manager.h"
#include "chrome/browser/web_applications/web_app_data_retriever.h"
#include "chrome/browser/web_applications/web_app_helpers.h"
#include "chrome/browser/web_applications/web_app_id.h"
#include "chrome/browser/web_applications/web_app_install_finalizer.h"
#include "chrome/browser/web_applications/web_app_install_info.h"
#include "chrome/browser/web_applications/web_app_install_utils.h"
#include "chrome/browser/web_applications/web_app_url_loader.h"
#include "chrome/browser/web_applications/web_app_utils.h"
#include "components/webapps/browser/install_result_code.h"
#include "components/webapps/browser/installable/installable_metrics.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/manifest/manifest_util.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"
#include "url/gurl.h"

namespace web_app {

constexpr static char kGeneratedInstallPagePath[] =
    "/.well-known/_generated_install_page.html";

namespace {

bool IsUrlLoadingResultSuccess(WebAppUrlLoader::Result result) {
  return result == WebAppUrlLoader::Result::kUrlLoaded;
}

absl::optional<std::string> UTF16ToUTF8(base::StringPiece16 src) {
  std::string dest;
  if (!base::UTF16ToUTF8(src.data(), src.length(), &dest)) {
    return absl::nullopt;
  }
  return dest;
}

}  // namespace

InstallIsolatedWebAppCommand::InstallIsolatedWebAppCommand(
    const IsolatedWebAppUrlInfo& isolation_info,
    const IsolationData& isolation_data,
    std::unique_ptr<content::WebContents> web_contents,
    std::unique_ptr<WebAppUrlLoader> url_loader,
    content::BrowserContext& browser_context,
    base::OnceCallback<void(base::expected<InstallIsolatedWebAppCommandSuccess,
                                           InstallIsolatedWebAppCommandError>)>
        callback)
    : WebAppCommandTemplate<AppLock>("InstallIsolatedWebAppCommand"),
      lock_description_(std::make_unique<AppLockDescription>(
          base::flat_set<AppId>{isolation_info.app_id()})),
      isolation_info_(isolation_info),
      isolation_data_(isolation_data),
      web_contents_(std::move(web_contents)),
      url_loader_(std::move(url_loader)),
      browser_context_(browser_context),
      data_retriever_(std::make_unique<WebAppDataRetriever>()) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(web_contents_ != nullptr);
  DCHECK(!callback.is_null());

  callback_ =
      base::BindOnce(
          [](base::expected<InstallIsolatedWebAppCommandSuccess,
                            InstallIsolatedWebAppCommandError> result) {
            webapps::InstallableMetrics::TrackInstallResult(result.has_value());
            return result;
          })
          .Then(std::move(callback));
}

void InstallIsolatedWebAppCommand::SetDataRetrieverForTesting(
    std::unique_ptr<WebAppDataRetriever> data_retriever) {
  data_retriever_ = std::move(data_retriever);
}

InstallIsolatedWebAppCommand::~InstallIsolatedWebAppCommand() = default;

LockDescription& InstallIsolatedWebAppCommand::lock_description() const {
  return *lock_description_;
}

base::Value InstallIsolatedWebAppCommand::ToDebugValue() const {
  base::Value::Dict debug_value;
  debug_value.Set("app_id", isolation_info_.app_id());
  debug_value.Set("origin", isolation_info_.origin().Serialize());
  debug_value.Set("bundle_id", isolation_info_.web_bundle_id().id());
  debug_value.Set("bundle_type",
                  static_cast<int>(isolation_info_.web_bundle_id().type()));
  debug_value.Set("isolation_data", isolation_data_.AsDebugValue());
  return base::Value(std::move(debug_value));
}

void InstallIsolatedWebAppCommand::StartWithLock(
    std::unique_ptr<AppLock> lock) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  lock_ = std::move(lock);

  CreateStoragePartition();
  LoadUrl();
}

void InstallIsolatedWebAppCommand::CreateStoragePartition() {
  browser_context_->GetStoragePartition(
      isolation_info_.storage_partition_config(&*browser_context_),
      /*can_create=*/true);
}

void InstallIsolatedWebAppCommand::LoadUrl() {
  DCHECK(web_contents_ != nullptr);

  // |web_app::IsolatedWebAppURLLoaderFactory| uses the isolation data in
  // order to determine the current state of content serving (installation
  // process vs application data serving) and source of data (proxy, web
  // bundle, etc...).
  IsolatedWebAppPendingInstallInfo::FromWebContents(*web_contents_)
      .set_isolation_data(isolation_data_);

  GURL install_page_url =
      isolation_info_.origin().GetURL().Resolve(kGeneratedInstallPagePath);
  url_loader_->LoadUrl(install_page_url, web_contents_.get(),
                       WebAppUrlLoader::UrlComparison::kIgnoreQueryParamsAndRef,
                       base::BindOnce(&InstallIsolatedWebAppCommand::OnLoadUrl,
                                      weak_factory_.GetWeakPtr()));
}

void InstallIsolatedWebAppCommand::OnLoadUrl(WebAppUrlLoaderResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!IsUrlLoadingResultSuccess(result)) {
    ReportFailure(base::StrCat({"Error during URL loading: ",
                                ConvertUrlLoaderResultToString(result)}));
    return;
  }

  CheckInstallabilityAndRetrieveManifest();
}

void InstallIsolatedWebAppCommand::CheckInstallabilityAndRetrieveManifest() {
  data_retriever_->CheckInstallabilityAndRetrieveManifest(
      web_contents_.get(),
      /*bypass_service_worker_check=*/true,
      base::BindOnce(&InstallIsolatedWebAppCommand::
                         OnCheckInstallabilityAndRetrieveManifest,
                     weak_factory_.GetWeakPtr()));
}

base::expected<WebAppInstallInfo, std::string>
InstallIsolatedWebAppCommand::CreateInstallInfoFromManifest(
    const blink::mojom::Manifest& manifest,
    const GURL& manifest_url) {
  WebAppInstallInfo info;
  UpdateWebAppInfoFromManifest(manifest, manifest_url, &info);

  if (!manifest.id.has_value()) {
    return base::unexpected{
        base::StrCat({"Manifest `id` is not present. manifest_url: ",
                      manifest_url.possibly_invalid_spec()})};
  }

  // In other installations the best-effort encoding is fine, but for isolated
  // apps we have the opportunity to report this error.
  absl::optional<std::string> encoded_id = UTF16ToUTF8(*manifest.id);
  if (!encoded_id.has_value()) {
    return base::unexpected{
        "Failed to convert manifest `id` from UTF16 to UTF8."};
  }

  if (!encoded_id->empty()) {
    // Recommend to use "/" for manifest id and not empty manifest id because
    // the manifest parser does additional work on resolving manifest id taking
    // `start_url` into account. (See https://w3c.github.io/manifest/#id-member
    // on how the manifest parser resolves the `id` field).
    //
    // It is required for Isolated Web Apps to have app id based on origin of
    // the application and do not include other information in order to be able
    // to identify Isolated Web Apps by origin because there is always only 1
    // app per origin.
    return base::unexpected{base::StrCat(
        {R"(Manifest `id` must be "/". Resolved manifest id: )", *encoded_id})};
  }

  info.manifest_id = "";

  url::Origin origin = isolation_info_.origin();
  if (manifest.scope != origin.GetURL()) {
    return base::unexpected{
        base::StrCat({"Scope should resolve to the origin. scope: ",
                      manifest.scope.possibly_invalid_spec(),
                      ", origin: ", origin.Serialize()})};
  }

  return info;
}

void InstallIsolatedWebAppCommand::OnCheckInstallabilityAndRetrieveManifest(
    blink::mojom::ManifestPtr opt_manifest,
    const GURL& manifest_url,
    bool valid_manifest_for_web_app,
    bool is_installable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!is_installable) {
    ReportFailure("App is not installable.");
    return;
  }

  // See |WebAppDataRetriever::CheckInstallabilityCallback| documentation for
  // details.
  DCHECK(valid_manifest_for_web_app)
      << "must be true when |is_installable| is true.";

  if (!opt_manifest) {
    ReportFailure("Manifest is null.");
    return;
  }

  // See |WebAppDataRetriever::CheckInstallabilityCallback| documentation for
  // details.
  DCHECK(!blink::IsEmptyManifest(opt_manifest))
      << "must not be empty when manifest is present.";

  // See |WebAppDataRetriever::CheckInstallabilityCallback| documentation for
  // details.
  DCHECK(!manifest_url.is_empty())
      << "must not be empty if manifest is not empty.";

  DCHECK(!web_contents_->IsBeingDestroyed());

  if (base::expected<WebAppInstallInfo, std::string> install_info =
          CreateInstallInfoFromManifest(*opt_manifest, manifest_url);
      install_info.has_value()) {
    DownloadIcons(*std::move(install_info));
  } else {
    ReportFailure(install_info.error());
  }
}

void InstallIsolatedWebAppCommand::FinalizeInstall(
    const WebAppInstallInfo& info) {
  WebAppInstallFinalizer::FinalizeOptions options(
      webapps::WebappInstallSource::ISOLATED_APP_DEV_INSTALL);
  options.isolation_data = isolation_data_;

  lock_->install_finalizer().FinalizeInstall(
      info, options,
      base::BindOnce(&InstallIsolatedWebAppCommand::OnFinalizeInstall,
                     weak_factory_.GetWeakPtr()));
}

void InstallIsolatedWebAppCommand::OnFinalizeInstall(
    const AppId& unused_app_id,
    webapps::InstallResultCode install_result_code,
    OsHooksErrors unused_os_hooks_errors) {
  if (install_result_code == webapps::InstallResultCode::kSuccessNewInstall) {
    ReportSuccess();
  } else {
    std::stringstream os;
    os << install_result_code;
    ReportFailure(base::StrCat({"Error during finalization: ", os.str()}));
  }
}

void InstallIsolatedWebAppCommand::DownloadIcons(
    WebAppInstallInfo install_info) {
  base::flat_set<GURL> icon_urls = GetValidIconUrlsToDownload(install_info);
  data_retriever_->GetIcons(
      web_contents_.get(), std::move(icon_urls),
      /*skip_page_favicons=*/true,
      base::BindOnce(&InstallIsolatedWebAppCommand::OnGetIcons,
                     weak_factory_.GetWeakPtr(), std::move(install_info)));
}

void InstallIsolatedWebAppCommand::OnGetIcons(
    WebAppInstallInfo install_info,
    IconsDownloadedResult result,
    std::map<GURL, std::vector<SkBitmap>> icons_map,
    std::map<GURL, int /*http_status_code*/> unused_icons_http_results) {
  if (result != IconsDownloadedResult::kCompleted) {
    ReportFailure(base::StrCat({"Error during icon downloading: ",
                                IconsDownloadedResultToString(result)}));
    return;
  }

  PopulateProductIcons(&install_info, &icons_map);
  PopulateOtherIcons(&install_info, icons_map);

  FinalizeInstall(install_info);
}

void InstallIsolatedWebAppCommand::OnSyncSourceRemoved() {
  // TODO(kuragin): Test cancellation on sync source removed event.
  ReportFailure("Sync source removed.");
}

void InstallIsolatedWebAppCommand::OnShutdown() {
  // TODO(kuragin): Test cancellation of pending installation during system
  // shutdown.
  ReportFailure("System is shutting down.");
}

void InstallIsolatedWebAppCommand::ReportFailure(base::StringPiece message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback_.is_null());

  SignalCompletionAndSelfDestruct(
      CommandResult::kFailure,
      base::BindOnce(std::move(callback_),
                     base::unexpected{InstallIsolatedWebAppCommandError{
                         .message = std::string{message}}}));
}

void InstallIsolatedWebAppCommand::ReportSuccess() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback_.is_null());

  SignalCompletionAndSelfDestruct(
      CommandResult::kSuccess,
      base::BindOnce(std::move(callback_),
                     InstallIsolatedWebAppCommandSuccess{}));
}

}  // namespace web_app
