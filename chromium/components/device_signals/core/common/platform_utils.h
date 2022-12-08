// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_PLATFORM_UTILS_H_
#define COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_PLATFORM_UTILS_H_

#include "base/process/process_handle.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class FilePath;
}  // namespace base

namespace device_signals {

// Extracts the common details for resolving a file path on different
// platforms. Resolves environment variables and relative markers in
// `file_path`, and returns the path via `resolved_file_path`. For
// consistency on all platforms, this method will return false if no file
// system item resides at the end path and true otherwise.
bool ResolvePath(const base::FilePath& file_path,
                 base::FilePath* resolved_file_path);

// Returns the file path pointing to the executable file that spawned
// the given process `pid`.
absl::optional<base::FilePath> GetProcessExePath(base::ProcessId pid);

}  // namespace device_signals

#endif  // COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_PLATFORM_UTILS_H_
