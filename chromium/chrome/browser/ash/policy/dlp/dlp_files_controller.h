// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_DLP_DLP_FILES_CONTROLLER_H_
#define CHROME_BROWSER_ASH_POLICY_DLP_DLP_FILES_CONTROLLER_H_

#include <sys/types.h>
#include <cstddef>
#include <functional>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/policy/dlp/dlp_rules_manager.h"
#include "chromeos/dbus/dlp/dlp_service.pb.h"
#include "components/services/app_service/public/cpp/intent.h"
#include "storage/browser/file_system/file_system_url.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom-forward.h"
#include "url/gurl.h"

namespace storage {
class FileSystemURL;
}  // namespace storage

namespace views {
class Widget;
}  // namespace views

namespace policy {

class DlpWarnNotifier;
class DlpFilesEventStorage;

// DlpFilesController is responsible for deciding whether file transfers are
// allowed according to the files sources saved in the DLP daemon and the rules
// of the Data leak prevention policy set by the admin.
class DlpFilesController {
 public:
  // Types of file actions. These actions are used when warning dialogs are
  // shown because of files restrictions. This is used in UMA histograms, should
  // not change order.
  enum class FileAction {
    kUnknown = 0,
    kDownload = 1,
    kTransfer = 2,
    kUpload = 3,
    kCopy = 4,
    kMove = 5,
    kMaxValue = kMove
  };

  // DlpFileMetadata keeps metadata about a file, such as whether it's managed
  // or not and the source URL, if it exists.
  struct DlpFileMetadata {
    DlpFileMetadata() = delete;
    DlpFileMetadata(const std::string& source_url, bool is_dlp_restricted);

    friend bool operator==(const DlpFileMetadata& a, const DlpFileMetadata& b) {
      return a.is_dlp_restricted == b.is_dlp_restricted &&
             a.source_url == b.source_url;
    }
    friend bool operator!=(const DlpFileMetadata& a, const DlpFileMetadata& b) {
      return !(a == b);
    }

    // Source URL from which the file was downloaded.
    std::string source_url;
    // Whether the file is under any DLP rule or not.
    bool is_dlp_restricted;
  };

  // DlpFileRestrictionDetails keeps aggregated information about DLP rules
  // that apply to a file. It consists of the level (e.g. block, warn) and
  // destinations for which this level applies (URLs and/or components).
  struct DlpFileRestrictionDetails {
    DlpFileRestrictionDetails();
    DlpFileRestrictionDetails(const DlpFileRestrictionDetails&) = delete;
    DlpFileRestrictionDetails& operator=(const DlpFileRestrictionDetails&) =
        delete;
    DlpFileRestrictionDetails(DlpFileRestrictionDetails&&);
    DlpFileRestrictionDetails& operator=(DlpFileRestrictionDetails&&);
    ~DlpFileRestrictionDetails();

    // The level for which the restriction is enforced.
    DlpRulesManager::Level level;
    // List of URLs for which the restriction is enforced.
    std::vector<std::string> urls;
    // List of components for which the restriction is enforced.
    std::vector<DlpRulesManager::Component> components;
  };

  // FileDaemonInfo represents file info used for communication with the DLP
  // daemon.
  struct FileDaemonInfo {
    FileDaemonInfo() = delete;
    FileDaemonInfo(ino64_t inode,
                   const base::FilePath& path,
                   const std::string& source_url);

    friend bool operator==(const FileDaemonInfo& a, const FileDaemonInfo& b) {
      return a.inode == b.inode && a.path == b.path &&
             a.source_url == b.source_url;
    }
    friend bool operator!=(const FileDaemonInfo& a, const FileDaemonInfo& b) {
      return !(a == b);
    }

    // File inode.
    ino64_t inode;
    // File path.
    base::FilePath path;
    // Source URL from which the file was downloaded.
    GURL source_url;
  };

  // DlpFileDestination represents the destination for file transfer. It either
  // has a url or a component.
  struct DlpFileDestination {
    DlpFileDestination();
    explicit DlpFileDestination(const std::string& url);
    explicit DlpFileDestination(const dlp::DlpComponent component);
    explicit DlpFileDestination(const DlpRulesManager::Component component);

    DlpFileDestination(const DlpFileDestination&);
    DlpFileDestination& operator=(const DlpFileDestination&);
    DlpFileDestination(DlpFileDestination&&);
    DlpFileDestination& operator=(DlpFileDestination&&);

    bool operator==(const DlpFileDestination&) const;
    bool operator!=(const DlpFileDestination&) const;
    bool operator<(const DlpFileDestination& other) const;
    bool operator<=(const DlpFileDestination& other) const;
    bool operator>(const DlpFileDestination& other) const;
    bool operator>=(const DlpFileDestination& other) const;

    ~DlpFileDestination();

    // Destination url or destination path.
    absl::optional<std::string> url_or_path;
    // Destination component.
    absl::optional<DlpRulesManager::Component> component;
  };

  using GetDisallowedTransfersCallback =
      base::OnceCallback<void(std::vector<storage::FileSystemURL>)>;
  using GetFilesRestrictedByAnyRuleCallback = GetDisallowedTransfersCallback;
  using FilterDisallowedUploadsCallback = base::OnceCallback<void(
      std::vector<blink::mojom::FileChooserFileInfoPtr>)>;
  using CheckIfDownloadAllowedCallback = base::OnceCallback<void(bool)>;
  using CheckIfLaunchAllowedCallback = base::OnceCallback<void(bool)>;
  using GetDlpMetadataCallback =
      base::OnceCallback<void(std::vector<DlpFileMetadata>)>;
  using IsFilesTransferRestrictedCallback =
      base::OnceCallback<void(const std::vector<FileDaemonInfo>&)>;

  explicit DlpFilesController(const DlpRulesManager& rules_manager);
  DlpFilesController(const DlpFilesController& other) = delete;
  DlpFilesController& operator=(const DlpFilesController& other) = delete;

  virtual ~DlpFilesController();

  // Returns a list of files disallowed to be transferred in |result_callback|.
  void GetDisallowedTransfers(
      const std::vector<storage::FileSystemURL>& transferred_files,
      storage::FileSystemURL destination,
      GetDisallowedTransfersCallback result_callback);

  // Retrieves metadata for each entry in |files| and returns it as a list in
  // |result_callback|.
  void GetDlpMetadata(const std::vector<storage::FileSystemURL>& files,
                      GetDlpMetadataCallback result_callback);

  // Filters files disallowed to be uploaded to `destination`.
  void FilterDisallowedUploads(
      std::vector<blink::mojom::FileChooserFileInfoPtr> uploaded_files,
      const GURL& destination,
      FilterDisallowedUploadsCallback result_callback);

  // Checks whether the file download from `download_url` to `file_path` is
  // allowed.
  void CheckIfDownloadAllowed(const GURL& download_url,
                              const base::FilePath& file_path,
                              CheckIfDownloadAllowedCallback result_callback);

  // Checks whether launching `app_id` with `intent` is allowed.
  void CheckIfLaunchAllowed(const std::string& app_id,
                            apps::IntentPtr intent,
                            CheckIfLaunchAllowedCallback result_callback);

  // Returns a sublist of |transferred_files| which aren't allowed to be
  // transferred to either |destination_url| or |destination_component| in
  // |result_callback|.
  void IsFilesTransferRestricted(
      const std::vector<FileDaemonInfo>& transferred_files,
      const DlpFileDestination& destination,
      FileAction files_action,
      IsFilesTransferRestrictedCallback result_callback);

  // Returns restriction information for `source_url`.
  std::vector<DlpFileRestrictionDetails> GetDlpRestrictionDetails(
      const std::string& source_url);

  // Returns a list of components to which the transfer of a file with
  // `source_url` is blocked.
  std::vector<DlpRulesManager::Component> GetBlockedComponents(
      const std::string& source_url);

  // Returns whether a dlp policy matches for the `file`.
  bool IsDlpPolicyMatched(const FileDaemonInfo& file);

  // The same source url information stored for |source| is copied for
  // |destination|
  virtual void CopySourceInformation(const storage::FileSystemURL& source,
                                     const storage::FileSystemURL& destination);

  void SetWarnNotifierForTesting(
      std::unique_ptr<DlpWarnNotifier> warn_notifier);

  DlpFilesEventStorage* GetEventStorageForTesting();

 private:
  // Called back from warning dialog. Passes blocked files sources along
  // to |callback|. In case |should_proceed| is true, passes only
  // |restricted_files_sources|, otherwise passes also |warned_files_sources|.
  void OnDlpWarnDialogReply(
      std::vector<FileDaemonInfo> restricted_files_sources,
      std::vector<FileDaemonInfo> warned_files_sources,
      std::vector<std::string> warned_src_patterns,
      const DlpFileDestination& dst,
      const absl::optional<std::string>& dst_pattern,
      FileAction files_action,
      IsFilesTransferRestrictedCallback callback,
      bool should_proceed);

  void ReturnDisallowedTransfers(
      base::flat_map<std::string, storage::FileSystemURL> files_map,
      GetDisallowedTransfersCallback result_callback,
      dlp::CheckFilesTransferResponse response);

  void ReturnAllowedUploads(
      std::vector<blink::mojom::FileChooserFileInfoPtr> uploaded_files,
      FilterDisallowedUploadsCallback result_callback,
      dlp::CheckFilesTransferResponse response);

  void ReturnDlpMetadata(std::vector<absl::optional<ino64_t>> inodes,
                         GetDlpMetadataCallback result_callback,
                         const ::dlp::GetFilesSourcesResponse response);

  // Reports an event if a `DlpReportingManager` instance exists. When
  // `dst_pattern` is missing, we report `dst.component.value()` instead. When
  // `level` is missing, we report a warning proceeded event.
  void MaybeReportEvent(ino64_t inode,
                        const base::FilePath& path,
                        const std::string& source_pattern,
                        const DlpFileDestination& dst,
                        const absl::optional<std::string>& dst_pattern,
                        absl::optional<DlpRulesManager::Level> level);

  // Closes warning dialog if `response` has error.
  ::dlp::CheckFilesTransferResponse MaybeCloseDialog(
      ::dlp::CheckFilesTransferResponse response);

  const DlpRulesManager& rules_manager_;

  // Is used for creating and showing the warning dialog.
  std::unique_ptr<DlpWarnNotifier> warn_notifier_;
  // Pointer to the associated DlpWarnDialog widget.
  // Not null only while the dialog is opened.
  base::WeakPtr<views::Widget> warn_dialog_widget_ = nullptr;

  // Keeps track of events and detects duplicate ones using time based approach.
  std::unique_ptr<DlpFilesEventStorage> event_storage_;

  base::WeakPtrFactory<DlpFilesController> weak_ptr_factory_{this};
};

}  // namespace policy

#endif  // CHROME_BROWSER_ASH_POLICY_DLP_DLP_FILES_CONTROLLER_H_
