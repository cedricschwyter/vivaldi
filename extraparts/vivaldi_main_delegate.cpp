// Copyright (c) 2015 Vivaldi Technologies AS. All rights reserved

#include "extraparts/vivaldi_main_delegate.h"

#include "build/build_config.h"
#include "components/version_info/version_info.h"

#include "app/vivaldi_apptools.h"
#include "extraparts/vivaldi_content_browser_client.h"

VivaldiMainDelegate::VivaldiMainDelegate()
#if !BUILDFLAG(IS_ANDROID)
    : VivaldiMainDelegate(base::TimeTicks())
#endif
{
}

#if !BUILDFLAG(IS_ANDROID)
VivaldiMainDelegate::VivaldiMainDelegate(base::TimeTicks exe_entry_point_ticks)
    : ChromeMainDelegate(exe_entry_point_ticks) {}
#endif

VivaldiMainDelegate::~VivaldiMainDelegate() {}

content::ContentBrowserClient*
VivaldiMainDelegate::CreateContentBrowserClient() {
  if (!vivaldi::IsVivaldiRunning() && !vivaldi::ForcedVivaldiRunning()) {
    return ChromeMainDelegate::CreateContentBrowserClient();
  }

  if (chrome_content_browser_client_ == nullptr) {
    chrome_content_browser_client_ =
        std::make_unique<VivaldiContentBrowserClient>();
  }
  return chrome_content_browser_client_.get();
}

absl::optional<int> VivaldiMainDelegate::BasicStartupComplete() {
  constexpr const char chromium_version_switch[] = "chromium-version";
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(chromium_version_switch)) {
    printf("%s\n", version_info::GetVersionNumber().c_str());
    return 0;
  }

  #if BUILDFLAG(IS_ANDROID)
    return ChromeMainDelegateAndroid::BasicStartupComplete();
  #else
    return ChromeMainDelegate::BasicStartupComplete();
  #endif
}

#if BUILDFLAG(IS_WIN)
bool VivaldiTestMainDelegate::ShouldHandleConsoleControlEvents() {
  return false;
}
#endif
