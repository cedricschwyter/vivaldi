// Copyright (c) 2015-2018 Vivaldi Technologies AS. All rights reserved

#include "extraparts/vivaldi_browser_main_extra_parts.h"

#include <string>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/vivaldi_switches.h"
#include "browser/stats_reporter.h"
#include "browser/translate/vivaldi_translate_client.h"
#include "build/build_config.h"
#include "calendar/calendar_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "components/adverse_adblocking/adverse_ad_filter_list_factory.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/favicon/core/favicon_service.h"
#include "components/translate/core/browser/translate_language_list.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/api/content_settings/content_settings_helpers.h"
#include "extensions/buildflags/buildflags.h"
#include "media/base/media_switches.h"
#include "ui/lazy_load_service_factory.h"

#include "app/vivaldi_apptools.h"
#include "browser/vivaldi_runtime_feature.h"
#include "components/browser/vivaldi_brand_select.h"

#include "components/datasource/vivaldi_image_store.h"
#include "contact/contact_service_factory.h"
#include "notes/notes_factory.h"
#include "prefs/vivaldi_pref_names.h"
#include "ui/webui/vivaldi_web_ui_controller_factory.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/api/auto_update/auto_update_api.h"
#include "extensions/api/bookmark_context_menu/bookmark_context_menu_api.h"
#include "extensions/api/bookmarks/bookmarks_private_api.h"
#include "extensions/api/calendar/calendar_api.h"
#include "extensions/api/contacts/contacts_api.h"
#include "extensions/api/content_blocking/content_blocking_api.h"
#include "extensions/api/events/vivaldi_ui_events.h"
#include "extensions/api/extension_action_utils/extension_action_utils_api.h"
#include "extensions/api/history/history_private_api.h"
#include "extensions/api/import_data/import_data_api.h"
#include "extensions/api/menu_content/menu_content_api.h"
#include "extensions/api/notes/notes_api.h"
#include "extensions/api/page_actions/page_actions_api.h"
#include "extensions/api/prefs/prefs_api.h"
#include "extensions/api/reading_list/reading_list_api.h"
#include "extensions/api/runtime/runtime_api.h"
#include "extensions/api/search_engines/search_engines_api.h"
#include "extensions/api/sessions/vivaldi_sessions_api.h"
#include "extensions/api/settings/settings_api.h"
#include "extensions/api/sync/sync_api.h"
#include "extensions/api/tabs/tabs_private_api.h"
#include "extensions/api/theme/theme_private_api.h"
#include "extensions/api/translate_history/translate_history_api.h"
#include "extensions/api/vivaldi_account/vivaldi_account_api.h"
#include "extensions/api/vivaldi_utilities/vivaldi_utilities_api.h"
#include "extensions/api/window/window_private_api.h"
#include "extensions/api/zoom/zoom_api.h"
#include "extensions/vivaldi_extensions_init.h"
#include "ui/devtools/devtools_connector.h"
#include "ui/vivaldi_rootdocument_handler.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#endif

namespace vivaldi {
void StartGitIgnoreCheck();
}

VivaldiBrowserMainExtraParts::VivaldiBrowserMainExtraParts() {}

VivaldiBrowserMainExtraParts::~VivaldiBrowserMainExtraParts() {}

// Overridden from ChromeBrowserMainExtraParts:
void VivaldiBrowserMainExtraParts::PostEarlyInitialization() {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
  base::FilePath messaging(
  // Hardcoded from chromium/chrome/common/chrome_paths.cc
#if BUILDFLAG(IS_MAC)
      FILE_PATH_LITERAL("/Library/Google/Chrome/NativeMessagingHosts")
#else   // IS_MAC
      FILE_PATH_LITERAL("/etc/opt/chrome/native-messaging-hosts")
#endif  // IS_MAC
  );
  base::PathService::Override(chrome::DIR_NATIVE_MESSAGING, messaging);
#endif
}

void VivaldiBrowserMainExtraParts::
    EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  translate::TranslateLanguageList::DisableUpdate();
#if !BUILDFLAG(IS_ANDROID)
  vivaldi::NotesModelFactory::GetInstance();
  calendar::CalendarServiceFactory::GetInstance();
  contact::ContactServiceFactory::GetInstance();
  vivaldi_runtime_feature::Init();
#endif
  VivaldiImageStore::InitFactory();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::AutoUpdateAPI::Init();
  extensions::BookmarkContextMenuAPI::GetFactoryInstance();
  extensions::CalendarAPI::GetFactoryInstance();
  extensions::ContactsAPI::GetFactoryInstance();
  extensions::ContentBlockingAPI::GetFactoryInstance();
  extensions::VivaldiBookmarksAPI::GetFactoryInstance();
  extensions::VivaldiUIEvents::InitSingleton();
  extensions::DevtoolsConnectorAPI::GetFactoryInstance();
  extensions::ExtensionActionUtilFactory::GetInstance();
  extensions::ImportDataAPI::GetFactoryInstance();
  extensions::NotesAPI::GetFactoryInstance();
  extensions::MenuContentAPI::GetFactoryInstance();
  extensions::TabsPrivateAPI::Init();
  extensions::ThemePrivateAPI::GetFactoryInstance();
  extensions::SearchEnginesAPI::GetFactoryInstance();
  extensions::SyncAPI::GetFactoryInstance();
  extensions::VivaldiAccountAPI::GetFactoryInstance();
  extensions::VivaldiExtensionInit::GetFactoryInstance();
  extensions::VivaldiPrefsApiNotificationFactory::GetInstance();
  extensions::PageActionsAPI::GetFactoryInstance();
  extensions::ReadingListPrivateAPI::GetFactoryInstance();
  extensions::RuntimeAPI::Init();
  extensions::SessionsPrivateAPI::GetFactoryInstance();
  extensions::VivaldiUtilitiesAPI::GetFactoryInstance();
  extensions::VivaldiWindowsAPI::Init();
  extensions::ZoomAPI::GetFactoryInstance();
  extensions::HistoryPrivateAPI::GetFactoryInstance();
  extensions::TranslateHistoryAPI::GetFactoryInstance();

  extensions::VivaldiRootDocumentHandlerFactory::GetInstance();

#endif
  VivaldiAdverseAdFilterListFactory::GetFactoryInstance();

#if !BUILDFLAG(IS_ANDROID)
  vivaldi::LazyLoadServiceFactory::GetInstance();
#endif

  VivaldiTranslateClient::LoadTranslationScript();
}

void VivaldiBrowserMainExtraParts::PreProfileInit() {
  EnsureBrowserContextKeyedServiceFactoriesBuilt();

  vivaldi::ClientHintsBrandRegisterProfilePrefs(
      g_browser_process->local_state());
}

void VivaldiBrowserMainExtraParts::PostProfileInit(Profile* profile,
                                                   bool is_initial_profile) {
  content::WebUIControllerFactory::RegisterFactory(
      vivaldi::VivaldiWebUIControllerFactory::GetInstance());

  if (vivaldi::IsVivaldiRunning()) {
    translate_language_list_ =
        std::make_unique<translate::VivaldiTranslateLanguageList>();
  }

#if !BUILDFLAG(IS_ANDROID)
  base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  vivaldi::CommandLineAppendSwitchNoDup(cmd_line, switches::kSavePageAsMHTML);

  if (cmd_line.HasSwitch(switches::kAppId)) {
    std::string extension_app_id =
        cmd_line.GetSwitchValueASCII(switches::kAppId);
    if (vivaldi::IsVivaldiApp(extension_app_id)) {
      // --app-id with our appId breaks a lot of stuff, so
      // catch it early and remove it.
      cmd_line.RemoveSwitch(switches::kAppId);
    }
  }

  // Sanetize contentsettings visible in our web-ui.
  std::list<ContentSettingsType> ui_exposed_settings = {
      ContentSettingsType::AUTOPLAY,
      ContentSettingsType::BLUETOOTH_SCANNING,
      ContentSettingsType::GEOLOCATION,
      ContentSettingsType::MEDIASTREAM_CAMERA,
      ContentSettingsType::MEDIASTREAM_MIC,
      ContentSettingsType::MIDI_SYSEX,
      ContentSettingsType::NOTIFICATIONS,
      ContentSettingsType::POPUPS,
      ContentSettingsType::SENSORS,
      ContentSettingsType::SOUND};

  HostContentSettingsMap* content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);

  std::list<ContentSettingsType>::iterator it;
  for (it = ui_exposed_settings.begin(); it != ui_exposed_settings.end();
       it++) {
    ContentSettingsType content_type = *it;
    ContentSetting default_setting =
        HostContentSettingsMapFactory::GetForProfile(profile)
            ->GetDefaultContentSetting(content_type, nullptr);
    const content_settings::ContentSettingsInfo* info =
        content_settings::ContentSettingsRegistry::GetInstance()->Get(
            content_type);

    bool is_valid_settings_value = info->IsDefaultSettingValid(default_setting);
    if (!is_valid_settings_value) {
      LOG(INFO)
          << "Vivaldi changed invalid default setting "
          << extensions::content_settings_helpers::ContentSettingsTypeToString(
                 content_type);
      default_setting = CONTENT_SETTING_DEFAULT;
      content_settings_map->SetDefaultContentSetting(content_type,
                                                     default_setting);
    }
  }
#else
  // Disable background media suspend
  PrefService* prefs = profile->GetPrefs();
  if (prefs->GetBoolean(vivaldiprefs::kBackgroundMediaPlaybackAllowed)) {
    base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
    cmd_line.AppendSwitch(switches::kDisableBackgroundMediaSuspend);
  }
#endif  // IS_ANDROID

  vivaldi::StartGitIgnoreCheck();
}

void VivaldiBrowserMainExtraParts::PreMainMessageLoopRun() {
  // The stats reporter must not be initialized earlier than this, because
  // some platforms may not have their screen information available before this
  // point.
  base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  if (!cmd_line.HasSwitch(switches::kAutoTestMode))
    stats_reporter_ = vivaldi::StatsReporter::CreateInstance();
}

void VivaldiBrowserMainExtraParts::PostMainMessageLoopRun() {
  vivaldi::ClientHintsBrandRegisterProfilePrefs(nullptr);

#if !BUILDFLAG(IS_ANDROID)
  extensions::AutoUpdateAPI::Shutdown();
#endif  // IS_ANDROID
}