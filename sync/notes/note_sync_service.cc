// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/notes/note_sync_service.h"

#include "base/feature_list.h"

namespace sync_notes {

NoteSyncService::NoteSyncService(file_sync::SyncedFileStore* synced_file_store)
    : note_model_type_processor_(synced_file_store) {}

NoteSyncService::~NoteSyncService() = default;

std::string NoteSyncService::EncodeNoteSyncMetadata() {
  return note_model_type_processor_.EncodeSyncMetadata();
}

void NoteSyncService::DecodeNoteSyncMetadata(
    const std::string& metadata_str,
    const base::RepeatingClosure& schedule_save_closure,
    vivaldi::NotesModel* model) {
  note_model_type_processor_.ModelReadyToSync(metadata_str,
                                              schedule_save_closure, model);
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
NoteSyncService::GetNoteSyncControllerDelegate() {
  return note_model_type_processor_.GetWeakPtr();
}

void NoteSyncService::SetNotesLimitForTesting(size_t limit) {
  note_model_type_processor_.SetMaxNotesTillSyncEnabledForTest(  // IN-TEST
      limit);
}

}  // namespace sync_notes
