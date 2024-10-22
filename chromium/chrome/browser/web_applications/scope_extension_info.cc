// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "chrome/browser/web_applications/scope_extension_info.h"

namespace web_app {

ScopeExtensionInfo::ScopeExtensionInfo(const url::Origin& origin,
                                       bool has_origin_wildcard)
    : origin(origin), has_origin_wildcard(has_origin_wildcard) {}

base::Value ScopeExtensionInfo::AsDebugValue() const {
  base::Value root(base::Value::Type::DICT);
  root.SetStringKey("origin", origin.GetDebugString());
  root.SetBoolKey("has_origin_wildcard", has_origin_wildcard);
  return root;
}

bool operator==(const ScopeExtensionInfo& scope_extension1,
                const ScopeExtensionInfo& scope_extension2) {
  return scope_extension1.origin == scope_extension2.origin &&
         scope_extension1.has_origin_wildcard ==
             scope_extension2.has_origin_wildcard;
}

bool operator!=(const ScopeExtensionInfo& scope_extension1,
                const ScopeExtensionInfo& scope_extension2) {
  return !(scope_extension1 == scope_extension2);
}

bool operator<(const ScopeExtensionInfo& scope_extension1,
               const ScopeExtensionInfo& scope_extension2) {
  return std::tie(scope_extension1.origin,
                  scope_extension1.has_origin_wildcard) <
         std::tie(scope_extension2.origin,
                  scope_extension2.has_origin_wildcard);
}

}  // namespace web_app
