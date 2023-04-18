// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BROWSER_PLUGIN_GUEST_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_BROWSER_PLUGIN_GUEST_DELEGATE_H_

#include "content/common/content_export.h"
#include "content/public/browser/web_contents.h"

namespace content {
class RenderFrameHost;

class BrowserPluginGuest;

// Objects implement this interface to get notified about changes in the guest
// WebContents and to provide necessary functionality.
class CONTENT_EXPORT BrowserPluginGuestDelegate {
 public:
  virtual ~BrowserPluginGuestDelegate() = default;

  // NOTE(andre@vivaldi.com) : We had to make sure the webcontents is added to
  // a tabstrip to get extension-events from the beginning of life.
  // |disposition| is ours.
  virtual std::unique_ptr<WebContents> CreateNewGuestWindow(
      const WebContents::CreateParams& create_params,
      int disposition);

  // Returns the WebContents that currently owns this guest.
  virtual WebContents* GetOwnerWebContents();

  // Returns the RenderFrameHost that owns this guest, but has not yet attached
  // it.
  // TODO(crbug.com/769461): Have all guest types return the specific owner
  // RenderFrameHost and not assume it's the owner's main frame.
  virtual RenderFrameHost* GetProspectiveOuterDocument();

  // NOTE(andre@vivaldi.com):
  // It is always set for tab and inspected webviews that might move between
  // embedders. Used to reset guest_host_ in between hand-overs. Ie. move
  // between docked/un-docked devtools.
  BrowserPluginGuest* delegate_to_browser_plugin_ = nullptr;

  // NOTE(andre@vivaldi.com):
  // Helper to create and initialize a BrowserPluginGuest for a webcontents
  // already created.
  virtual void CreatePluginGuest(WebContents* contents);

};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BROWSER_PLUGIN_GUEST_DELEGATE_H_
