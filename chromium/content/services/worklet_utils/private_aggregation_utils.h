// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SERVICES_WORKLET_UTILS_PRIVATE_AGGREGATION_UTILS_H_
#define CONTENT_SERVICES_WORKLET_UTILS_PRIVATE_AGGREGATION_UTILS_H_

#include "content/common/aggregatable_report.mojom-forward.h"
#include "content/common/private_aggregation_host.mojom-forward.h"

namespace gin {
class Arguments;
}

namespace worklet_utils {

// Parses arguments provided to `sendHistogramReport()` and returns the
// corresponding contribution. In case of an error, throws an exception and
// returns `nullptr`.
content::mojom::AggregatableReportHistogramContributionPtr
ParseSendHistogramReportArguments(const gin::Arguments& args);

// Parses arguments provided to `enableDebugMode()` and updates
// `debug_mode_details` as appropriate. `debug_mode_details` must be passed a
// reference to the existing (likely default) details. In case of an error,
// throws an exception and does not update `debug_mode_details`.
void ParseAndApplyEnableDebugModeArguments(
    const gin::Arguments& args,
    content::mojom::DebugModeDetails& debug_mode_details);

}  // namespace worklet_utils

#endif  // CONTENT_SERVICES_WORKLET_UTILS_PRIVATE_AGGREGATION_UTILS_H_
