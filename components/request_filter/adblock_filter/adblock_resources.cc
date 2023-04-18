// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/request_filter/adblock_filter/adblock_resources.h"

#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/escape.h"
#include "base/strings/string_piece.h"
#include "base/task/sequenced_task_runner.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/apk_assets.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#else
#include "base/json/json_file_value_serializer.h"
#endif

namespace adblock_filter {
namespace {
#if BUILDFLAG(IS_ANDROID)
const base::FilePath::CharType kRedirectableResourcesFilePath[] =
    FILE_PATH_LITERAL("assets/adblocker_resources/redirectable_resources.json");
const base::FilePath::CharType kInjectableResourcesFilePath[] =
    FILE_PATH_LITERAL("assets/adblocker_resources/injectable_resources.json");
#else
const base::FilePath::CharType kRedirectableResourcesFilePath[] =
    FILE_PATH_LITERAL(
        "vivaldi/adblocker_resources/redirectable_resources.json");
const base::FilePath::CharType kInjectableResourcesFilePath[] =
    FILE_PATH_LITERAL("vivaldi/adblocker_resources/injectable_resources.json");
#endif

constexpr auto kAliasMap =
    base::MakeFixedFlatMap<base::StringPiece, base::StringPiece>({
        // Aliases used by ublock rules
        {"1x1-transparent.gif", "1x1.gif"},
        {"2x2-transparent.png", "2x2.png"},
        {"3x2-transparent.png", "3x2.png"},
        {"32x32-transparent.png", "32x32.png"},
        {"addthis.com/addthis_widget.js", "addthis_widget.js"},
        {"amazon-adsystem.com/aax2/amzn_ads.js", "amazon_ads.js"},
        {"ampproject.org/v0.js", "ampproject_v0.js"},
        {"static.chartbeat.com/chartbeat.js", "chartbeat.js"},
        {"doubleclick.net/instream/ad_status.js",
         "doubleclick_instream_ad_status.js"},
        {"google-analytics.com/analytics.js", "google-analytics_analytics.js"},
        {"google-analytics.com/cx/api.js", "google-analytics_cx_api.js"},
        {"google-analytics.com/ga.js", "google-analytics_ga.js"},
        {"google-analytics.com/inpage_linkid.js",
         "google-analytics_inpage_linkid.js"},
        {"googlesyndication.com/adsbygoogle.js",
         "googlesyndication_adsbygoogle.js"},
        {"googletagmanager.com/gtm.js", "googletagmanager_gtm.js"},
        {"googletagservices.com/gpt.js", "googletagservices_gpt.js"},
        {"ligatus.com/*/angular-tag.js", "ligatus_angular-tag.js"},
        {"d3pkae9owd2lcf.cloudfront.net/mb105.js", "monkeybroker.js"},
        {"silent-noeval.js", "noeval-silent.js"},
        {"bab-defuser.js", "nobab.js"},
        {"fuckadblock.js-3.2.0", "nofab.js"},
        {"noopmp3-0.1s", "noop-0.1s.mp3"},
        {"noopmp4-1s", "noop-1s.mp4"},
        {"noopjs", "noop.js"},
        {"noopvmap-1.0", "noop-vmap1.0.xml"},
        {"nooptext", "noop.txt"},
        {"widgets.outbrain.com/outbrain.js", "outbrain-widget.js"},
        {"popads.net.js", "popads.js"},
        {"scorecardresearch.com/beacon.js", "scorecardresearch_beacon.js"},
        {"nowoif.js", "window.open-defuser.js"},

        // Aliases used to support adblock rewrite rules
        {"blank-text", "noop.txt"},
        {"blank-css", "noop.css"},
        {"blank-js", "noop.js"},
        {"blank-html", "noop.html"},
        {"blank-mp3", "noopmp3-0.1s"},
        {"blank-mp4", "noopmp4-1s"},
        {"1x1-transparent-gif", "1x1.gif"},
        {"2x2-transparent-png", "2x2.png"},
        {"3x2-transparent-png", "3x2.png"},
        {"32x32-transparent-png", "32x32.png"},

        // Surrogate names used by the DDG list
        {"ga.js", "google-analytics_ga.js"},
        {"analytics.js", "google-analytics_analytics.js"},
        {"inpage_linkid.js", "google-analytics_inpage_linkid.js"},
        {"api.js", "google-analytics_cx_api.js"},
        {"gpt.js", "googletagservices_gpt.js"},
        {"gtm.js", "googletagmanager_gtm.js"},
        {"adsbygoogle.js", "googlesyndication_adsbygoogle.js"},
        {"ad_status.js", "doubleclick_instream_ad_status.js"},
        {"beacon.js", "scorecardresearch_beacon.js"},
        {"outbrain.js", "outbrain-widget.js"},
        {"amzn_ads.js", "amazon_ads.js"},
    });

constexpr auto kMimetypeForEmpty =
    base::MakeFixedFlatMap<flat::ResourceType, base::StringPiece>({
        {flat::ResourceType_SUBDOCUMENT, "text/html,"},
        {flat::ResourceType_OTHER, "text/plain,"},
        {flat::ResourceType_STYLESHEET, "text/css,"},
        {flat::ResourceType_SCRIPT, "application/javascript,"},
        {flat::ResourceType_XMLHTTPREQUEST, "text/plain,"},
    });

constexpr auto kMimeTypeForExtension =
    base::MakeFixedFlatMap<base::StringPiece, base::StringPiece>({
        {".gif", "image/gif;base64,"},
        {".html", "text/html,"},
        {".js", "application/javascript,"},
        {".mp3", "audio/mp3;base64,"},
        {".mp4", "video/mp4;base64,"},
        {".png", "image/png;base64,"},
        {".txt", "text/plain,"},
        {".css", "text/css,"},
        {".xml", "text/xml,"},
    });

// uBlock technically allows to inject any of those scripts, even if it doesn't
// make sense for all of them.
constexpr auto kInjectableRedirectables =
    base::MakeFixedFlatSet<base::StringPiece>(
        {"amazon_ads.js", "doubleclick_instream_ad_status.js",
         "google-analytics_analytics.js", "google-analytics_cx_api.js",
         "google-analytics_ga.js", "googlesyndication_adsbygoogle.js",
         "googletagmanager_gtm.js", "googletagservices_gpt.js", "noeval.js",
         "noeval-silent.js", "nobab.js", "nofab.js", "noop.js", "popads.js",
         "popads-dummy.js", "window.open-defuser.js"});

std::unique_ptr<base::Value> LoadResources(
    const base::FilePath::CharType* resource_file) {
#if BUILDFLAG(IS_ANDROID)
  base::MemoryMappedFile::Region region;
  base::MemoryMappedFile mapped_file;
  int json_fd = base::android::OpenApkAsset(resource_file, &region);
  if (json_fd < 0) {
    LOG(ERROR) << "Adblock resources not found in APK assest.";
    return nullptr;
  } else {
    if (!mapped_file.Initialize(base::File(json_fd), region)) {
      LOG(ERROR) << "failed to initialize memory mapping for " << resource_file;
      return nullptr;
    }
    base::StringPiece json_text(reinterpret_cast<char*>(mapped_file.data()),
                                mapped_file.length());
    JSONStringValueDeserializer deserializer(json_text);
    return deserializer.Deserialize(nullptr, nullptr);
  }
#else
  base::FilePath path;
  base::PathService::Get(chrome::DIR_RESOURCES, &path);
  path = path.Append(resource_file);
  JSONFileValueDeserializer deserializer(path);
  return deserializer.Deserialize(nullptr, nullptr);
#endif
}
}  // namespace

Resources::Resources(scoped_refptr<base::SequencedTaskRunner> task_runner)
    : weak_factory_(this) {
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadResources, kRedirectableResourcesFilePath),
      base::BindOnce(&Resources::OnLoadFinished, weak_factory_.GetWeakPtr(),
                     &redirectable_resources_));
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadResources, kInjectableResourcesFilePath),
      base::BindOnce(&Resources::OnLoadFinished, weak_factory_.GetWeakPtr(),
                     &injectable_resources_));
}
Resources::~Resources() = default;

void Resources::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void Resources::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void Resources::OnLoadFinished(base::Value* destination,
                               std::unique_ptr<base::Value> resources) {
  if (resources && resources->is_dict())
    *destination = std::move(*resources);

  if (loaded()) {
    for (Observer& observer : observers_)
      observer.OnResourcesLoaded();
  }
}

absl::optional<std::string> Resources::GetRedirect(
    const std::string& name,
    flat::ResourceType resource_type) const {
  // If resources aren't yet loaded, then we'll just block the request.
  if (!redirectable_resources_.is_dict() ||
      resource_type == flat::ResourceType_WEBSOCKET ||
      resource_type == flat::ResourceType_WEBRTC ||
      resource_type == flat::ResourceType_PING)
    return absl::nullopt;

  base::StringPiece actual_name(name);
  auto* actual_name_it = kAliasMap.find(name);
  if (actual_name_it != kAliasMap.end())
    actual_name = actual_name_it->second;

  if (actual_name == "empty") {
    auto* mimetype_it = kMimetypeForEmpty.find(resource_type);
    if (mimetype_it == kMimetypeForEmpty.end())
      return absl::nullopt;
    return std::string("data:") + std::string(mimetype_it->second);
  }

  const std::string* resource =
      redirectable_resources_.FindStringKey(actual_name);
  if (!resource)
    return absl::nullopt;

  size_t extension_separator_position = actual_name.find_last_of('.');
  if (extension_separator_position == base::StringPiece::npos)
    return absl::nullopt;

  auto* mimetype_it = kMimeTypeForExtension.find(
      actual_name.substr(extension_separator_position));
  if (mimetype_it == kMimeTypeForExtension.end())
    return absl::nullopt;

  return std::string("data:") + std::string(mimetype_it->second) +
         base::EscapeUrlEncodedData(*resource, false);
}

std::map<std::string, base::StringPiece> Resources::GetInjections() {
  DCHECK(loaded());

  std::map<std::string, base::StringPiece> result;

  for (auto resource : injectable_resources_.DictItems()) {
    result[resource.first] = base::StringPiece(resource.second.GetString());
  }

  for (auto resource : redirectable_resources_.DictItems()) {
    if (kInjectableRedirectables.count(resource.first)) {
      result[resource.first] = base::StringPiece(resource.second.GetString());
    }
  }

  return result;
}

}  // namespace adblock_filter