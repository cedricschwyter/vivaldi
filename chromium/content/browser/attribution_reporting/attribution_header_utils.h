// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ATTRIBUTION_REPORTING_ATTRIBUTION_HEADER_UTILS_H_
#define CONTENT_BROWSER_ATTRIBUTION_REPORTING_ATTRIBUTION_HEADER_UTILS_H_

#include "base/types/expected.h"
#include "base/values.h"
#include "content/browser/attribution_reporting/attribution_reporting.mojom-forward.h"
#include "content/browser/attribution_reporting/attribution_source_type.h"
#include "content/common/content_export.h"

namespace base {
class Time;
}  // namespace base

namespace url {
class Origin;
}  // namespace url

namespace content {

class StorableSource;

CONTENT_EXPORT
base::expected<StorableSource,
               attribution_reporting::mojom::SourceRegistrationError>
ParseSourceRegistration(base::Value::Dict registration,
                        base::Time source_time,
                        url::Origin reporting_origin,
                        url::Origin source_origin,
                        AttributionSourceType source_type);

}  // namespace content

#endif  // CONTENT_BROWSER_ATTRIBUTION_REPORTING_ATTRIBUTION_HEADER_UTILS_H_
