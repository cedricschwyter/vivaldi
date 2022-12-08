// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CREATOR_PUBLIC_CREATOR_API_H_
#define COMPONENTS_CREATOR_PUBLIC_CREATOR_API_H_

#include <string>

#include "url/gurl.h"

namespace creator {

struct Creator {
  GURL url;
  std::u16string title;
};

// This is the public access point for interacting with the creator contents.
class CreatorApi {
 public:
  CreatorApi();
  virtual ~CreatorApi();
  CreatorApi(const CreatorApi&) = delete;
  CreatorApi& operator=(const CreatorApi&) = delete;

  Creator GetCreator(std::string web_channel_id);
};

}  // namespace creator

#endif  // COMPONENTS_CREATOR_PUBLIC_CREATOR_API_H_
