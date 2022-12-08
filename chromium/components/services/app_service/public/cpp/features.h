// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_APP_SERVICE_PUBLIC_CPP_FEATURES_H_
#define COMPONENTS_SERVICES_APP_SERVICE_PUBLIC_CPP_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace apps {

COMPONENT_EXPORT(APP_TYPES) BASE_DECLARE_FEATURE(kAppServiceLaunchWithoutMojom);
COMPONENT_EXPORT(APP_TYPES)
BASE_DECLARE_FEATURE(kAppServiceSetPermissionWithoutMojom);
COMPONENT_EXPORT(APP_TYPES)
BASE_DECLARE_FEATURE(kAppServiceUninstallWithoutMojom);
COMPONENT_EXPORT(APP_TYPES) BASE_DECLARE_FEATURE(kAppServiceWithoutMojom);
COMPONENT_EXPORT(APP_TYPES)
BASE_DECLARE_FEATURE(kAppServiceGetMenuWithoutMojom);
COMPONENT_EXPORT(APP_TYPES)
BASE_DECLARE_FEATURE(kAppServiceCapabilityAccessWithoutMojom);
COMPONENT_EXPORT(APP_TYPES) BASE_DECLARE_FEATURE(kStopMojomAppService);

}  // namespace apps

#endif  // COMPONENTS_SERVICES_APP_SERVICE_PUBLIC_CPP_FEATURES_H_
