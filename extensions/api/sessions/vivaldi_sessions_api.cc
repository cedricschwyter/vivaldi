// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright (c) 2016 Vivaldi Technologies AS. All rights reserved.
//

#include "extensions/api/sessions/vivaldi_sessions_api.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "app/vivaldi_constants.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "browser/sessions/vivaldi_session_service.h"
#include "browser/vivaldi_browser_finder.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/sessions/vivaldi_session_service_commands.h"
#include "content/public/browser/navigation_entry.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/schema/vivaldi_sessions.h"
#include "ui/vivaldi_browser_window.h"

using extensions::vivaldi::sessions_private::SessionOpenOptions;

namespace {
enum SessionErrorCodes {
  kNoError,
  kErrorMissingName,
  kErrorWriting,
  kErrorFileMissing,
  kErrorDeleteFailure
};

const base::FilePath::StringType kSessionPath = FILE_PATH_LITERAL("Sessions");
const int kNumberBufferSize = 16;

base::FilePath GenerateFilename(Profile* profile,
                                const std::string session_name,
                                bool unique_name) {
  // PathExists() triggers IO restriction.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  std::string temp_session_name(session_name);
  int cnt = 2;
  char number_string[kNumberBufferSize];

  do {
    base::FilePath path(profile->GetPath());
    path = path.Append(kSessionPath)
#if BUILDFLAG(IS_POSIX)
               .Append(temp_session_name)
#elif BUILDFLAG(IS_WIN)
               .Append(base::UTF8ToWide(temp_session_name))
#endif
               .AddExtension(FILE_PATH_LITERAL(".bin"));
    if (unique_name) {
      if (base::PathExists(path)) {
        temp_session_name.assign(session_name);
        base::snprintf(number_string, kNumberBufferSize, " (%d)", cnt++);
        temp_session_name.append(number_string);
      } else {
        return path;
      }
    } else {
      return path;
    }
    // Just to avoid any endless loop, which is highly unlikely, but still.
  } while (cnt < 1000);

  NOTREACHED();
  return base::FilePath();
}

}  // namespace

namespace extensions {

ExtensionFunction::ResponseAction SessionsPrivateSaveOpenTabsFunction::Run() {
  using vivaldi::sessions_private::SaveOpenTabs::Params;
  namespace Results = vivaldi::sessions_private::SaveOpenTabs::Results;

  std::unique_ptr<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int error_code = SessionErrorCodes::kNoError;
  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::vivaldi::VivaldiSessionService service(profile);

  int save_window_id = params->options.save_only_window_id;
  std::vector<int> ids;
  if (params->options.ids.has_value())
    ids = params->options.ids.value();

  if (params->name.empty()) {
    error_code = SessionErrorCodes::kErrorMissingName;
  } else {
    base::FilePath path = GenerateFilename(profile, params->name, true);

    for (auto* browser : *BrowserList::GetInstance()) {
      // Make sure the browser has tabs and a window. Browser's destructor
      // removes itself from the BrowserList. When a browser is closed the
      // destructor is not necessarily run immediately. This means it's possible
      // for us to get a handle to a browser that is about to be removed. If
      // the tab count is 0 or the window is NULL, the browser is about to be
      // deleted, so we ignore it.
      if (service.ShouldTrackWindow(browser) &&
          browser->tab_strip_model()->count() && browser->window()) {
        if (save_window_id == 0 ||
            (save_window_id && browser->session_id().id() == save_window_id)) {
          service.BuildCommandsForBrowser(browser, ids);
        }
      }
    }
    bool success = service.Save(path);
    if (!success) {
      error_code = SessionErrorCodes::kErrorWriting;
    }
  }

  return RespondNow(ArgumentList(Results::Create(error_code)));
}

SessionsPrivateGetAllFunction::SessionEntry::SessionEntry() {}

SessionsPrivateGetAllFunction::SessionEntry::~SessionEntry() {}

ExtensionFunction::ResponseAction SessionsPrivateGetAllFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath path(profile->GetPath());
  path = path.Append(kSessionPath);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&SessionsPrivateGetAllFunction::RunOnFileThread, this,
                     path),
      base::BindOnce(&SessionsPrivateGetAllFunction::SendResponse, this));

  return RespondLater();
}

std::vector<std::unique_ptr<SessionsPrivateGetAllFunction::SessionEntry>>
SessionsPrivateGetAllFunction::RunOnFileThread(base::FilePath path) {
  using extensions::vivaldi::sessions_private::SessionItem;
  namespace Results = vivaldi::sessions_private::GetAll::Results;
  std::vector<std::unique_ptr<SessionEntry>> sessions;
  ::vivaldi::VivaldiSessionService service;

  base::FileEnumerator iter(path, false, base::FileEnumerator::FILES,
                            FILE_PATH_LITERAL("*.bin"));
  for (base::FilePath name = iter.Next(); !name.empty(); name = iter.Next()) {
    std::unique_ptr<SessionEntry> entry = std::make_unique<SessionEntry>();
    entry->item = std::make_unique<SessionItem>();
    SessionItem* new_item = entry->item.get();
#if BUILDFLAG(IS_POSIX)
    std::string filename = name.BaseName().value();
#elif BUILDFLAG(IS_WIN)
    std::string filename = base::WideToUTF8(name.BaseName().value());
#endif
    size_t ext = filename.find_last_of('.');
    if (ext != std::string::npos) {
      // Get rid of the extension
      filename.erase(ext, std::string::npos);
    }
    new_item->name.assign(filename);

    base::FileEnumerator::FileInfo info = iter.GetInfo();
    base::Time modified = info.GetLastModifiedTime();
    new_item->create_date_js = modified.ToJsTime();
    auto commands = service.LoadSettingInfo(name);
    entry->commands.swap(commands);

    sessions.push_back(std::move(entry));
  }
  return sessions;
}

void SessionsPrivateGetAllFunction::SendResponse(
    std::vector<std::unique_ptr<SessionEntry>> sessions) {
  namespace Results = vivaldi::sessions_private::GetAll::Results;
  using extensions::vivaldi::sessions_private::SessionItem;
  std::vector<SessionItem> retval;

  for (auto& session_entry : sessions) {
    sessions::IdToSessionTab tabs;
    sessions::TokenToSessionTabGroup tab_groups;
    sessions::IdToSessionWindow windows;
    SessionID active_window_id = SessionID::InvalidValue();

    if (sessions::VivaldiCreateTabsAndWindows(session_entry->commands, &tabs,
                                              &tab_groups, &windows,
                                              &active_window_id)) {
      session_entry->item->tabs = tabs.size();
      session_entry->item->windows = windows.size();

      retval.push_back(std::move(*session_entry->item));
    }
  }
  Respond(ArgumentList(Results::Create(retval)));
}

ExtensionFunction::ResponseAction SessionsPrivateOpenFunction::Run() {
  using vivaldi::sessions_private::Open::Params;
  namespace Results = vivaldi::sessions_private::Open::Results;

  std::unique_ptr<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params.get());

  VivaldiBrowserWindow* window =
      VivaldiBrowserWindow::FromId(params->window_id);
  if (!window) {
    return RespondNow(Error("No such window"));
  }

  base::ThreadRestrictions::ScopedAllowIO allow_io;

  ::vivaldi::SessionOptions opts;
  if (params->options.has_value()) {
    opts.openInNewWindow_ = params->options->open_in_new_window;
  }

  Profile* profile = window->GetProfile();

  // TODO(igor@vivaldi.com): consider switching to lastError error reporting.
  int error_code = SessionErrorCodes::kErrorFileMissing;
  base::FilePath path = GenerateFilename(profile, params->name, false);
  do {
    if (!base::PathExists(path))
      break;

    ::vivaldi::VivaldiSessionService service(profile);
    if (!service.Load(path, window->browser(), opts))
      break;

    error_code = SessionErrorCodes::kNoError;
  } while (false);
  return RespondNow(ArgumentList(Results::Create(error_code)));
}

ExtensionFunction::ResponseAction SessionsPrivateDeleteFunction::Run() {
  using vivaldi::sessions_private::Delete::Params;
  namespace Results = vivaldi::sessions_private::Delete::Results;

  std::unique_ptr<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params.get());

  base::ThreadRestrictions::ScopedAllowIO allow_io;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  int error_code = SessionErrorCodes::kNoError;
  base::FilePath path = GenerateFilename(profile, params->name, false);
  if (!base::PathExists(path)) {
    error_code = kErrorFileMissing;
  } else {
    if (!base::DeleteFile(path)) {
      error_code = kErrorDeleteFailure;
    }
  }
  return RespondNow(ArgumentList(Results::Create(error_code)));
}

}  // namespace extensions
