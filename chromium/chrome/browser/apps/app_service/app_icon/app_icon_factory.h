// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_APP_SERVICE_APP_ICON_APP_ICON_FACTORY_H_
#define CHROME_BROWSER_APPS_APP_SERVICE_APP_ICON_APP_ICON_FACTORY_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/apps/app_service/app_icon/app_icon_util.h"
#include "chrome/browser/web_applications/web_app_install_info.h"
#include "components/services/app_service/public/cpp/icon_types.h"
#include "ui/gfx/image/image_skia.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "ash/components/arc/mojom/app.mojom.h"
#include "ash/components/arc/mojom/intent_helper.mojom.h"
#include "ui/base/resource/resource_scale_factor.h"
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

namespace content {
class BrowserContext;
}

namespace gfx {
class ImageSkia;
}

class SkBitmap;

namespace apps {

using ScaleToSize = std::map<float, int>;

static const int kInvalidIconResource = 0;

std::map<std::pair<int, int>, gfx::ImageSkia>& GetResourceIconCache();

// Gets the ImageSkia for the resource `icon_resource` and the size
// `size_in_dip`.
gfx::ImageSkia CreateResizedResourceImage(int icon_resource,
                                          int32_t size_in_dip);

apps::ScaleToSize GetScaleToSize(const gfx::ImageSkia& image_skia);

// Decodes `data` to a SkBitmap. The decode happens in-process, so must only be
// done with trusted data. Returns an empty bitmap if decoding fails.
SkBitmap DecompressToSkBitmap(const unsigned char* data, size_t size);

// Creates an ImageSkia for the given `bitmap` and `icon_scale`;
gfx::ImageSkia SkBitmapToImageSkia(SkBitmap bitmap, float icon_scale);

// Returns a callback that converts compressed data to an ImageSkia.
base::OnceCallback<void(std::vector<uint8_t> compressed_data)>
CompressedDataToImageSkiaCallback(
    base::OnceCallback<void(gfx::ImageSkia)> callback,
    float icon_scale);

// Converts compressed data to a SkBitmap.
void CompressedDataToSkBitmap(std::vector<uint8_t> compressed_data,
                              base::OnceCallback<void(SkBitmap)> callback);

// Encodes a single SkBitmap representation from the given ImageSkia to the
// compressed PNG data. |rep_icon_scale| argument denotes, which ImageSkiaRep to
// take as input. See ImageSkia::GetRepresentation() comments. Returns the
// encoded PNG data. This function should not be called on the UI thread.
std::vector<uint8_t> EncodeImageToPngBytes(const gfx::ImageSkia image,
                                           float rep_icon_scale);

gfx::ImageSkia LoadMaskImage(const ScaleToSize& scale_to_size);

gfx::ImageSkia ApplyBackgroundAndMask(const gfx::ImageSkia& image);

#if BUILDFLAG(IS_CHROMEOS_ASH)
gfx::ImageSkia CompositeImagesAndApplyMask(
    const gfx::ImageSkia& foreground_image,
    const gfx::ImageSkia& background_image);

void ArcRawIconPngDataToImageSkia(
    arc::mojom::RawIconPngDataPtr icon,
    int size_hint_in_dip,
    base::OnceCallback<void(const gfx::ImageSkia& icon)> callback);

void ArcActivityIconsToImageSkias(
    const std::vector<arc::mojom::ActivityIconPtr>& icons,
    base::OnceCallback<void(const std::vector<gfx::ImageSkia>& icons)>
        callback);

// TODO(crbug.com/1189994): Unify this function with AppIconLoader class.
// It's the same as AppIconLoader::OnReadWebAppIcon().
gfx::ImageSkia ConvertSquareBitmapsToImageSkia(
    const std::map<SquareSizePx, SkBitmap>& icon_bitmaps,
    IconEffects icon_effects,
    int size_hint_in_dip);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

gfx::ImageSkia ConvertIconBitmapsToImageSkia(
    const std::map<SquareSizePx, SkBitmap>& icon_bitmaps,
    int size_hint_in_dip);

// Modifies |iv| to apply icon post-processing effects (like badging and
// desaturation to gray) to an uncompressed icon.
void ApplyIconEffects(IconEffects icon_effects,
                      int size_hint_in_dip,
                      IconValuePtr iv,
                      LoadIconCallback callback);

// Encodes `iv` as a compressed PNG icon with `scale_factor`.
void ConvertUncompressedIconToCompressedIconWithScale(float rep_icon_scale,
                                                      LoadIconCallback callback,
                                                      IconValuePtr iv);

// Encodes |iv| as a compressed PNG icon.
void ConvertUncompressedIconToCompressedIcon(IconValuePtr iv,
                                             LoadIconCallback callback);

// Loads an icon from an extension.
void LoadIconFromExtension(IconType icon_type,
                           int size_hint_in_dip,
                           content::BrowserContext* context,
                           const std::string& extension_id,
                           IconEffects icon_effects,
                           LoadIconCallback callback);

// Loads an icon from a web app.
void LoadIconFromWebApp(content::BrowserContext* context,
                        IconType icon_type,
                        int size_hint_in_dip,
                        const std::string& web_app_id,
                        IconEffects icon_effects,
                        LoadIconCallback callback);

#if BUILDFLAG(IS_CHROMEOS)
// Requests a compressed icon data for an web app identified by `web_app_id`.
void GetWebAppCompressedIconData(content::BrowserContext* context,
                                 const std::string& web_app_id,
                                 int size_in_dip,
                                 ui::ResourceScaleFactor scale_factor,
                                 LoadIconCallback callback);

// Requests a compressed icon data for a chrome app identified by
// `extension_id`.
void GetChromeAppCompressedIconData(content::BrowserContext* context,
                                    const std::string& extension_id,
                                    int size_in_dip,
                                    ui::ResourceScaleFactor scale_factor,
                                    LoadIconCallback callback);
#endif  // BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_CHROMEOS_ASH)
// Requests a compressed icon data for an ARC app identified by `app_id`.
void GetArcAppCompressedIconData(content::BrowserContext* context,
                                 const std::string& app_id,
                                 int size_in_dip,
                                 ui::ResourceScaleFactor scale_factor,
                                 LoadIconCallback callback);

// Requests a compressed icon data for a Guest OS app identified by `app_id`.
void GetGuestOSAppCompressedIconData(content::BrowserContext* context,
                                     const std::string& app_id,
                                     int size_in_dip,
                                     ui::ResourceScaleFactor scale_factor,
                                     LoadIconCallback callback);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Loads an icon from a FilePath. If that fails, it calls the fallback.
//
// The file named by |path| might be empty, not found or otherwise unreadable.
// If so, "fallback(callback)" is run. If the file is non-empty and readable,
// just "callback" is run, even if that file doesn't contain a valid image.
//
// |fallback| should run its callback argument once complete, even on a
// failure. A failure should be indicated by passing nullptr, in which case the
// pipeline will use a generic fallback icon.
void LoadIconFromFileWithFallback(
    IconType icon_type,
    int size_hint_in_dip,
    const base::FilePath& path,
    IconEffects icon_effects,
    LoadIconCallback callback,
    base::OnceCallback<void(LoadIconCallback)> fallback);

// Creates an icon with the specified effects from |compressed_icon_data|.
void LoadIconFromCompressedData(IconType icon_type,
                                int size_hint_in_dip,
                                IconEffects icon_effects,
                                const std::string& compressed_icon_data,
                                LoadIconCallback callback);

// Loads an icon from a compiled-into-the-binary resource, with a resource_id
// named IDR_XXX, for some value of XXX.
void LoadIconFromResource(IconType icon_type,
                          int size_hint_in_dip,
                          int resource_id,
                          bool is_placeholder_icon,
                          IconEffects icon_effects,
                          LoadIconCallback callback);

}  // namespace apps

#endif  // CHROME_BROWSER_APPS_APP_SERVICE_APP_ICON_APP_ICON_FACTORY_H_
