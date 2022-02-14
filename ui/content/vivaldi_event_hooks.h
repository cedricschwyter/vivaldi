// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef UI_CONTENT_VIVALDI_EVENT_HOOKS_H_
#define UI_CONTENT_VIVALDI_EVENT_HOOKS_H_

#include "base/supports_user_data.h"
#include "chromium/content/common/content_export.h"
#include "third_party/blink/public/platform/web_drag_operation.h"

namespace blink {
class WebMouseEvent;
class WebMouseWheelEvent;
}  // namespace blink

namespace content {
struct DropData;
class RenderWidgetHostViewBase;
class WebContents;
}  // namespace content

namespace ui {
class LatencyInfo;
}  // namespace ui

// Hooks into Chromium event processing. The implementation is provided in
// tabs_private_api.cc and stored in WebContents associated with the Vivaldi
// window using SupportsUserData API.
class CONTENT_EXPORT VivaldiEventHooks : public base::SupportsUserData::Data {
 public:
  // Extra flag to extend ui::DragDropTypes::DragOperation to indicate cancelled
  // drag operation.
  static const int DRAG_CANCEL = 1 << 30;

  static const void* UserDataKey();

  static VivaldiEventHooks* FromRootView(
      content::RenderWidgetHostViewBase* root_view);

  static VivaldiEventHooks* FromWebContents(content::WebContents* web_contents);

  // Check for a mouse gesture event before it is dispatched to the web page
  // or default chromium handlers. Return true to stop further event
  // propagation or false to allow normal event flow.
  virtual bool HandleMouseEvent(content::RenderWidgetHostViewBase* root_view,
                                const blink::WebMouseEvent& event) = 0;

  // Check for a wheel gesture event before it is dispatched to the web page
  // or default chromium handlers. Return true to stop further event
  // propagation or false to allow normal event flow.
  virtual bool HandleWheelEvent(content::RenderWidgetHostViewBase* root_view,
                                const blink::WebMouseWheelEvent& event,
                                const ui::LatencyInfo& latency) = 0;

  // A method to hook into InputRouterImpl::MouseWheelEventHandled. Check for a
  // wheel gesture after the event was not consumed by a child view. Return true
  // to send a copy of the event to the root view for further processing.
  virtual bool ShouldCopyWheelEventToRoot(
      content::RenderWidgetHostViewBase* view,
      const blink::WebMouseWheelEvent& event) = 0;

  // Hook to notify UI about the end of the drag operation and pointer position
  // when the user released the pointer. Return true to prevent any default
  // action in Chromium. cancelled indicate that the platform API indicated
  // explicitly cancelled drag (currently can be true only on Windows).
  virtual bool HandleDragEnd(blink::WebDragOperation operation,
                             bool cancelled,
                             int screen_x,
                             int screen_y) = 0;

 private:
  static VivaldiEventHooks* FromOutermostContents(
      content::WebContents* web_contents);
};

#endif  // UI_CONTENT_VIVALDI_EVENT_HOOKS_H_
