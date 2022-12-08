// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_CONTACT_INFO_SYNC_BRIDGE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_CONTACT_INFO_SYNC_BRIDGE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/supports_user_data.h"
#include "base/threading/thread_checker.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_backend.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service_observer.h"
#include "components/sync/model/entity_change.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/protocol/entity_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace autofill {

class AutofillWebDataService;

class ContactInfoSyncBridge : public AutofillWebDataServiceObserverOnDBSequence,
                              public base::SupportsUserData::Data,
                              public syncer::ModelTypeSyncBridge {
 public:
  ContactInfoSyncBridge(
      std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor,
      AutofillWebDataBackend* backend);
  ~ContactInfoSyncBridge() override;

  ContactInfoSyncBridge(const ContactInfoSyncBridge&) = delete;
  ContactInfoSyncBridge& operator=(const ContactInfoSyncBridge&) = delete;

  static void CreateForWebDataServiceAndBackend(
      AutofillWebDataBackend* web_data_backend,
      AutofillWebDataService* web_data_service);

  static syncer::ModelTypeSyncBridge* FromWebDataService(
      AutofillWebDataService* web_data_service);

  // syncer::ModelTypeSyncBridge implementation.
  std::unique_ptr<syncer::MetadataChangeList> CreateMetadataChangeList()
      override;
  absl::optional<syncer::ModelError> MergeSyncData(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_data) override;
  absl::optional<syncer::ModelError> ApplySyncChanges(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_changes) override;
  void GetData(StorageKeyList storage_keys, DataCallback callback) override;
  void GetAllDataForDebugging(DataCallback callback) override;
  std::string GetClientTag(const syncer::EntityData& entity_data) override;
  std::string GetStorageKey(const syncer::EntityData& entity_data) override;

 private:
  // The bridge should be used on the same sequence where it has been
  // constructed.
  THREAD_CHECKER(thread_checker_);

  // ContactInfoSyncBridge is owned by `web_data_backend_` through
  // SupportsUserData, so it's guaranteed to outlive `this`.
  const raw_ptr<AutofillWebDataBackend> web_data_backend_;

  base::ScopedObservation<AutofillWebDataBackend,
                          AutofillWebDataServiceObserverOnDBSequence>
      scoped_observation_{this};
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_CONTACT_INFO_SYNC_BRIDGE_H_
