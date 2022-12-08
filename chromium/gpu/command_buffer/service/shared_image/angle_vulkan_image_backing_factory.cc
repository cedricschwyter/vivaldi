// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/angle_vulkan_image_backing_factory.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/service/shared_context_state.h"
#include "gpu/command_buffer/service/shared_image/angle_vulkan_image_backing.h"
#include "ui/gl/gl_surface_egl.h"

namespace gpu {

AngleVulkanImageBackingFactory::AngleVulkanImageBackingFactory(
    const GpuPreferences& gpu_preferences,
    const GpuDriverBugWorkarounds& workarounds,
    SharedContextState* context_state)
    : GLCommonImageBackingFactory(gpu_preferences,
                                  workarounds,
                                  context_state->feature_info(),
                                  context_state->progress_reporter()),
      context_state_(context_state) {
  DCHECK(context_state_->GrContextIsVulkan());
  DCHECK(gl::GLSurfaceEGL::GetGLDisplayEGL()->ext->b_EGL_ANGLE_vulkan_image);
}

AngleVulkanImageBackingFactory::~AngleVulkanImageBackingFactory() = default;

std::unique_ptr<SharedImageBacking>
AngleVulkanImageBackingFactory::CreateSharedImage(
    const Mailbox& mailbox,
    viz::SharedImageFormat format,
    SurfaceHandle surface_handle,
    const gfx::Size& size,
    const gfx::ColorSpace& color_space,
    GrSurfaceOrigin surface_origin,
    SkAlphaType alpha_type,
    uint32_t usage,
    bool is_thread_safe) {
  auto backing = std::make_unique<AngleVulkanImageBacking>(
      context_state_, mailbox, format, size, color_space, surface_origin,
      alpha_type, usage);

  if (!backing->Initialize({}))
    return nullptr;

  return backing;
}

std::unique_ptr<SharedImageBacking>
AngleVulkanImageBackingFactory::CreateSharedImage(
    const Mailbox& mailbox,
    viz::SharedImageFormat format,
    const gfx::Size& size,
    const gfx::ColorSpace& color_space,
    GrSurfaceOrigin surface_origin,
    SkAlphaType alpha_type,
    uint32_t usage,
    base::span<const uint8_t> data) {
  auto backing = std::make_unique<AngleVulkanImageBacking>(
      context_state_, mailbox, format, size, color_space, surface_origin,
      alpha_type, usage);

  if (!backing->Initialize(data))
    return nullptr;

  return backing;
}

std::unique_ptr<SharedImageBacking>
AngleVulkanImageBackingFactory::CreateSharedImage(
    const Mailbox& mailbox,
    int client_id,
    gfx::GpuMemoryBufferHandle handle,
    gfx::BufferFormat buffer_format,
    gfx::BufferPlane plane,
    SurfaceHandle surface_handle,
    const gfx::Size& size,
    const gfx::ColorSpace& color_space,
    GrSurfaceOrigin surface_origin,
    SkAlphaType alpha_type,
    uint32_t usage) {
  NOTREACHED();
  return nullptr;
}

bool AngleVulkanImageBackingFactory::CanUseAngleVulkanImageBacking(
    uint32_t usage) const {
  // Ignore for mipmap usage.
  usage &= ~SHARED_IMAGE_USAGE_MIPMAP;

  // TODO(penghuang): verify the scanout is the right usage for video playback.
  // crbug.com/1280798
  constexpr auto kSupportedUsages =
#if BUILDFLAG(IS_LINUX)
      SHARED_IMAGE_USAGE_SCANOUT |
#endif
      SHARED_IMAGE_USAGE_GLES2 | SHARED_IMAGE_USAGE_GLES2_FRAMEBUFFER_HINT |
      SHARED_IMAGE_USAGE_RASTER | SHARED_IMAGE_USAGE_DISPLAY_READ |
      SHARED_IMAGE_USAGE_DISPLAY_WRITE | SHARED_IMAGE_USAGE_OOP_RASTERIZATION |
      SHARED_IMAGE_USAGE_CPU_UPLOAD;

  if (usage & ~kSupportedUsages)
    return false;

  // AngleVulkan backing is used for GL & Vulkan interop, so the usage must
  // contain GLES2
  // TODO(penghuang): use AngleVulkan backing for non GL & Vulkan interop usage?
  return usage & SHARED_IMAGE_USAGE_GLES2;
}

bool AngleVulkanImageBackingFactory::IsSupported(
    uint32_t usage,
    viz::SharedImageFormat format,
    const gfx::Size& size,
    bool thread_safe,
    gfx::GpuMemoryBufferType gmb_type,
    GrContextType gr_context_type,
    base::span<const uint8_t> pixel_data) {
  DCHECK_EQ(gr_context_type, GrContextType::kVulkan);
  if (!CanUseAngleVulkanImageBacking(usage))
    return false;

  if (thread_safe)
    return false;

  if (gmb_type != gfx::EMPTY_BUFFER) {
    return false;
  }

  return CanCreateSharedImage(size, pixel_data, GetFormatInfo(format),
                              GL_TEXTURE_2D);
}

}  // namespace gpu
