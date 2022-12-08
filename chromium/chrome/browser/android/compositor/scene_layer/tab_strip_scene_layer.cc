// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/compositor/scene_layer/tab_strip_scene_layer.h"

#include "base/android/jni_android.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "cc/resources/scoped_ui_resource.h"
#include "chrome/android/chrome_jni_headers/TabStripSceneLayer_jni.h"
#include "chrome/browser/android/compositor/layer/tab_handle_layer.h"
#include "chrome/browser/android/compositor/layer_title_cache.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "ui/android/resources/nine_patch_resource.h"
#include "ui/android/resources/resource_manager_impl.h"
#include "ui/gfx/geometry/transform.h"

// Vivaldi
#include "app/vivaldi_apptools.h"
#include "ui/android/color_utils_android.h"

using base::android::JavaParamRef;
using base::android::JavaRef;

namespace android {

TabStripSceneLayer::TabStripSceneLayer(JNIEnv* env,
                                       const JavaRef<jobject>& jobj)
    : SceneLayer(env, jobj),
      tab_strip_layer_(cc::SolidColorLayer::Create()),
      scrollable_strip_layer_(cc::Layer::Create()),
      scrim_layer_(cc::SolidColorLayer::Create()),
      new_tab_button_(cc::UIResourceLayer::Create()),
      left_fade_(cc::UIResourceLayer::Create()),
      right_fade_(cc::UIResourceLayer::Create()),
      model_selector_button_(cc::UIResourceLayer::Create()),
      write_index_(0),
      content_tree_(nullptr) {
  new_tab_button_->SetIsDrawable(true);
  model_selector_button_->SetIsDrawable(true);
  left_fade_->SetIsDrawable(true);
  right_fade_->SetIsDrawable(true);
  scrim_layer_->SetIsDrawable(true);

  // When the ScrollingStripStacker is used, the new tab button and tabs scroll,
  // while the incognito button and left/ride fade stay fixed. Put the new tab
  // button and tabs in a separate layer placed visually below the others.
  scrollable_strip_layer_->SetIsDrawable(true);
  const bool tab_strip_improvements_enabled =
      base::FeatureList::IsEnabled(chrome::android::kTabStripImprovements);
  if (!tab_strip_improvements_enabled) {
    scrollable_strip_layer_->AddChild(new_tab_button_);
  }

  tab_strip_layer_->SetBackgroundColor(SkColors::kBlack);
  tab_strip_layer_->SetIsDrawable(true);
  tab_strip_layer_->AddChild(scrollable_strip_layer_);

  tab_strip_layer_->AddChild(left_fade_);
  tab_strip_layer_->AddChild(right_fade_);
  tab_strip_layer_->AddChild(model_selector_button_);
  if (tab_strip_improvements_enabled) {
    tab_strip_layer_->AddChild(new_tab_button_);
  }
  tab_strip_layer_->AddChild(scrim_layer_);

  // Note(david@vivaldi.com): The correct layer child assignment will be done
  // in |SetContentTree()| as we are dealing with a pair of
  // |TabStripSceneLayer|s.
  if (!vivaldi::IsVivaldiRunning())
  layer()->AddChild(tab_strip_layer_);

  // Vivaldi
  use_light_foreground_on_background = false;
  loading_text_ = cc::UIResourceLayer::Create();
  loading_text_->SetIsDrawable(true);
  tab_strip_layer_->AddChild(loading_text_);
}

TabStripSceneLayer::~TabStripSceneLayer() {
}

void TabStripSceneLayer::SetContentTree(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    const JavaParamRef<jobject>& jcontent_tree) {
  SceneLayer* content_tree = FromJavaObject(env, jcontent_tree);
  if (content_tree_ &&
      (!content_tree_->layer()->parent() ||
       content_tree_->layer()->parent()->id() != layer()->id()))
    content_tree_ = nullptr;

  if (content_tree != content_tree_) {
    if (content_tree_)
      content_tree_->layer()->RemoveFromParent();
    content_tree_ = content_tree;
    if (content_tree) {
      layer()->InsertChild(content_tree->layer(), 0);
      content_tree->layer()->SetPosition(
          gfx::PointF(0, -layer()->position().y()));
      // Note(david@vivaldi.com): Add the stacking strip to the main strip scene
      // layer in order to achieve the correct scrolling behaviour. The main
      // strip will be normally added to the current layer.
      if (is_stack_strip_)
        content_tree->layer()->AddChild(tab_strip_layer_);
      else
        layer()->AddChild(tab_strip_layer_);
    }
  }
}

void TabStripSceneLayer::BeginBuildingFrame(JNIEnv* env,
                                            const JavaParamRef<jobject>& jobj,
                                            jboolean visible) {
  write_index_ = 0;
  tab_strip_layer_->SetHideLayerAndSubtree(!visible);
}

void TabStripSceneLayer::FinishBuildingFrame(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj) {
  if (tab_strip_layer_->hide_layer_and_subtree())
    return;

  for (unsigned i = write_index_; i < tab_handle_layers_.size(); ++i)
    tab_handle_layers_[i]->layer()->RemoveFromParent();

  tab_handle_layers_.erase(tab_handle_layers_.begin() + write_index_,
                           tab_handle_layers_.end());
}

void TabStripSceneLayer::UpdateTabStripLayer(JNIEnv* env,
                                             const JavaParamRef<jobject>& jobj,
                                             jfloat width,
                                             jfloat height,
                                             jfloat y_offset,
                                             jboolean should_readd_background) {
  gfx::RectF content(0, y_offset, width, height);
  // Note(david@vivaldi.com): We apply a fixed height for the stack strip. The
  // |y_offset| however is only applied to the main strip of which the stacking
  // strip is a child of.
  if (is_stack_strip_)
    tab_strip_layer_->SetPosition(gfx::PointF(0, y_offset));
  else
  layer()->SetPosition(gfx::PointF(0, y_offset));
  tab_strip_layer_->SetBounds(gfx::Size(width, height));
  scrollable_strip_layer_->SetBounds(gfx::Size(width, height));

  // Content tree should not be affected by tab strip scene layer visibility.
  if (content_tree_ && !is_stack_strip_) // Vivaldi
    content_tree_->layer()->SetPosition(gfx::PointF(0, -y_offset));

  // Make sure tab strip changes are committed after rotating the device.
  // See https://crbug.com/503930 for more details.
  // InsertChild() forces the tree sync, which seems to fix the problem.
  // Note that this is a workaround.
  // TODO(changwan): find out why the update is not committed after rotation.
  if (should_readd_background) {
    int background_index = 0;
    if (content_tree_ && content_tree_->layer()) {
      background_index = 1;
    }
    DCHECK(layer()->children()[background_index] == tab_strip_layer_);
    layer()->InsertChild(tab_strip_layer_, background_index);
  }

  // Note(david@vivaldi.com): This will indicate a property change of
  // the |tab_strip_layer_| which makes sure that any changes are correclty
  // drawn.
  tab_strip_layer_->SetSubtreePropertyChanged();
}

void TabStripSceneLayer::UpdateStripScrim(JNIEnv* env,
                                          const JavaParamRef<jobject>& jobj,
                                          jfloat x,
                                          jfloat y,
                                          jfloat width,
                                          jfloat height,
                                          jint color,
                                          jfloat alpha) {
  if (alpha == 0.f) {
    scrim_layer_->SetIsDrawable(false);
    return;
  }

  scrim_layer_->SetIsDrawable(true);
  // TODO(crbug/1308932): Remove FromColor and make all SkColor4f.
  scrim_layer_->SetBackgroundColor(SkColor4f::FromColor(color));
  scrim_layer_->SetBounds(gfx::Size(width, height));
  scrim_layer_->SetPosition(gfx::PointF(x, y));
  scrim_layer_->SetOpacity(alpha);
}

void TabStripSceneLayer::UpdateNewTabButton(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    jint resource_id,
    jfloat x,
    jfloat y,
    jfloat width,
    jfloat height,
    jfloat touch_target_offset,
    jboolean visible,
    jint tint,
    jfloat button_alpha,
    const JavaParamRef<jobject>& jresource_manager) {
  ui::ResourceManager* resource_manager =
      ui::ResourceManagerImpl::FromJavaObject(jresource_manager);
  ui::Resource* button_resource =
      resource_manager->GetStaticResourceWithTint(resource_id, tint);

  // Vivaldi
  if (!use_light_foreground_on_background) {
    button_resource =
        resource_manager->GetStaticResourceWithTint(resource_id, SK_ColorBLACK);
  }

  new_tab_button_->SetUIResourceId(button_resource->ui_resource()->id());
  float left_offset = (width - button_resource->size().width()) / 2;
  float top_offset = (height - button_resource->size().height()) / 2;
  // The touch target for the new tab button is skewed towards the end of the
  // strip. This ensures that the view itself is correctly aligned without
  // adjusting the touch target.
  left_offset += touch_target_offset;
  new_tab_button_->SetPosition(gfx::PointF(x + left_offset, y + top_offset));
  new_tab_button_->SetBounds(button_resource->size());
  new_tab_button_->SetHideLayerAndSubtree(!visible);
  new_tab_button_->SetOpacity(button_alpha);
}

void TabStripSceneLayer::UpdateModelSelectorButton(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    jint resource_id,
    jfloat x,
    jfloat y,
    jfloat width,
    jfloat height,
    jboolean incognito,
    jboolean visible,
    const JavaParamRef<jobject>& jresource_manager) {
  ui::ResourceManager* resource_manager =
      ui::ResourceManagerImpl::FromJavaObject(jresource_manager);
  ui::Resource* button_resource = resource_manager->GetResource(
      ui::ANDROID_RESOURCE_TYPE_STATIC, resource_id);

  // Vivaldi
  if (!use_light_foreground_on_background) {
    button_resource =
        resource_manager->GetStaticResourceWithTint(resource_id, SK_ColorBLACK);
  }

  model_selector_button_->SetUIResourceId(button_resource->ui_resource()->id());
  float left_offset = (width - button_resource->size().width()) / 2;
  float top_offset = (height - button_resource->size().height()) / 2;
  model_selector_button_->SetPosition(
      gfx::PointF(x + left_offset, y + top_offset));
  model_selector_button_->SetBounds(button_resource->size());
  model_selector_button_->SetHideLayerAndSubtree(!visible);
}

void TabStripSceneLayer::UpdateTabStripLeftFade(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    jint resource_id,
    jfloat opacity,
    const JavaParamRef<jobject>& jresource_manager) {

  // Hide layer if it's not visible.
  if (opacity == 0.f) {
    left_fade_->SetHideLayerAndSubtree(true);
    return;
  }

  // Set UI resource.
  ui::ResourceManager* resource_manager =
      ui::ResourceManagerImpl::FromJavaObject(jresource_manager);
  ui::Resource* fade_resource = resource_manager->GetResource(
      ui::ANDROID_RESOURCE_TYPE_STATIC, resource_id);
  // Note (david@vivaldi.com): In Vivaldi we tint the fade resource.
  if (vivaldi::IsVivaldiRunning()) {
    fade_resource = resource_manager->GetStaticResourceWithTint(
        resource_id, tab_strip_layer_->background_color().toSkColor());
  }
  left_fade_->SetUIResourceId(fade_resource->ui_resource()->id());

  // The same resource is used for both left and right fade, so the
  // resource must be rotated for the left fade.
  gfx::Transform fade_transform;
  fade_transform.RotateAboutYAxis(180.0);
  left_fade_->SetTransform(fade_transform);

  // Set opacity.
  left_fade_->SetOpacity(opacity);

  // Set bounds. Use the parent layer height so the 1px fade resource is
  // stretched vertically.
  left_fade_->SetBounds(gfx::Size(fade_resource->size().width(),
                                  scrollable_strip_layer_->bounds().height()));

  // Set position. The rotation set above requires the layer to be offset
  // by its width in order to display on the left edge.
  left_fade_->SetPosition(gfx::PointF(fade_resource->size().width(), 0));

  // Ensure layer is visible.
  left_fade_->SetHideLayerAndSubtree(false);
}

void TabStripSceneLayer::UpdateTabStripRightFade(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    jint resource_id,
    jfloat opacity,
    const JavaParamRef<jobject>& jresource_manager) {

  // Hide layer if it's not visible.
  if (opacity == 0.f) {
    right_fade_->SetHideLayerAndSubtree(true);
    return;
  }

  // Set UI resource.
  ui::ResourceManager* resource_manager =
      ui::ResourceManagerImpl::FromJavaObject(jresource_manager);
  ui::Resource* fade_resource = resource_manager->GetResource(
      ui::ANDROID_RESOURCE_TYPE_STATIC, resource_id);
  // Note (david@vivaldi.com): In Vivaldi we tint the fade resource.
  if (vivaldi::IsVivaldiRunning()) {
    fade_resource = resource_manager->GetStaticResourceWithTint(
        resource_id, tab_strip_layer_->background_color().toSkColor());
  }
  right_fade_->SetUIResourceId(fade_resource->ui_resource()->id());

  // Set opacity.
  right_fade_->SetOpacity(opacity);

  // Set bounds. Use the parent layer height so the 1px fade resource is
  // stretched vertically.
  right_fade_->SetBounds(gfx::Size(fade_resource->size().width(),
                                   scrollable_strip_layer_->bounds().height()));

  // Set position. The right fade is positioned at the end of the tab strip.
  float x =
      scrollable_strip_layer_->bounds().width() - fade_resource->size().width();
  right_fade_->SetPosition(gfx::PointF(x, 0));

  // Ensure layer is visible.
  right_fade_->SetHideLayerAndSubtree(false);
}

void TabStripSceneLayer::PutStripTabLayer(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    jint id,
    jint close_resource_id,
    jint handle_resource_id,
    jint handle_outline_resource_id,
    jint close_tint,
    jint handle_tint,
    jint handle_outline_tint,
    jboolean foreground,
    jboolean close_pressed,
    jfloat toolbar_width,
    jfloat x,
    jfloat y,
    jfloat width,
    jfloat height,
    jfloat content_offset_x,
    jfloat close_button_alpha,
    jboolean is_loading,
    jfloat spinner_rotation,
    jfloat brightness,
    const JavaParamRef<jobject>& jlayer_title_cache,
    const JavaParamRef<jobject>& jresource_manager,
    jfloat tab_alpha, // Vivaldi
    jboolean is_shown_as_favicon, // Vivaldi
    jfloat title_offset) { // Vivaldi
  LayerTitleCache* layer_title_cache =
      LayerTitleCache::FromJavaObject(jlayer_title_cache);
  ui::ResourceManager* resource_manager =
      ui::ResourceManagerImpl::FromJavaObject(jresource_manager);
  scoped_refptr<TabHandleLayer> layer = GetNextLayer(layer_title_cache);
  ui::NinePatchResource* tab_handle_resource =
      ui::NinePatchResource::From(resource_manager->GetStaticResourceWithTint(
          handle_resource_id, handle_tint));
  ui::NinePatchResource* tab_handle_outline_resource =
      ui::NinePatchResource::From(resource_manager->GetStaticResourceWithTint(
          handle_outline_resource_id, handle_outline_tint));
  ui::Resource* close_button_resource =
      resource_manager->GetStaticResourceWithTint(close_resource_id,
                                                  close_tint);
  layer->SetProperties(id, close_button_resource, tab_handle_resource,
                       tab_handle_outline_resource, foreground, close_pressed,
                       toolbar_width, x, y, width, height, content_offset_x,
                       close_button_alpha, is_loading, spinner_rotation,
                       brightness,
                       tab_alpha, is_shown_as_favicon, title_offset); // Vivaldi
}

scoped_refptr<TabHandleLayer> TabStripSceneLayer::GetNextLayer(
    LayerTitleCache* layer_title_cache) {
  if (write_index_ < tab_handle_layers_.size())
    return tab_handle_layers_[write_index_++];

  scoped_refptr<TabHandleLayer> layer_tree =
      TabHandleLayer::Create(layer_title_cache);
  tab_handle_layers_.push_back(layer_tree);
  scrollable_strip_layer_->AddChild(layer_tree->layer());
  write_index_++;
  return layer_tree;
}

bool TabStripSceneLayer::ShouldShowBackground() {
  if (content_tree_)
    return content_tree_->ShouldShowBackground();
  return SceneLayer::ShouldShowBackground();
}

SkColor TabStripSceneLayer::GetBackgroundColor() {
  if (content_tree_)
    return content_tree_->GetBackgroundColor();
  return SceneLayer::GetBackgroundColor();
}

// Vivaldi
void TabStripSceneLayer::SetTabStripBackgroundColor(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    jint java_color,
    jboolean use_light) {
  SkColor4f color =
      SkColor4f::FromColor(ui::JavaColorToOptionalSkColor(java_color).value());
  tab_strip_layer_->SetBackgroundColor(color);
  use_light_foreground_on_background = use_light;
}

// Vivaldi
void TabStripSceneLayer::SetIsStackStrip(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jobj,
      jboolean jis_stack_strip){
  is_stack_strip_ = jis_stack_strip;
}

// Vivaldi
void TabStripSceneLayer::UpdateLoadingState(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jobj,
    jint loading_text_resource_id,
    const base::android::JavaParamRef<jobject>& jresource_manager,
    jboolean should_show_loading) {
  ui::ResourceManager* resource_manager =
      ui::ResourceManagerImpl::FromJavaObject(jresource_manager);
  ui::Resource* title_resource = resource_manager->GetResource(
      ui::ANDROID_RESOURCE_TYPE_DYNAMIC_BITMAP, loading_text_resource_id);
  if (title_resource) {
    loading_text_->SetUIResourceId(title_resource->ui_resource()->id());
    loading_text_->SetBounds(title_resource->size());
    int pos_x = tab_strip_layer_->bounds().width() / 2;
    int pos_y = tab_strip_layer_->bounds().height() / 2;
    loading_text_->SetPosition(
        gfx::PointF(pos_x - (loading_text_->bounds().width() / 2),
                    pos_y - loading_text_->bounds().height() / 2));
    loading_text_->SetHideLayerAndSubtree(!should_show_loading);
  }
}

static jlong JNI_TabStripSceneLayer_Init(JNIEnv* env,
                                         const JavaParamRef<jobject>& jobj) {
  // This will automatically bind to the Java object and pass ownership there.
  TabStripSceneLayer* scene_layer = new TabStripSceneLayer(env, jobj);
  return reinterpret_cast<intptr_t>(scene_layer);
}

}  // namespace android
