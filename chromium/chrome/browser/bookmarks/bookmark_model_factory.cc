// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/bookmark_model_factory.h"

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/values.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "chrome/browser/sync/bookmark_sync_service_factory.h"
#include "chrome/browser/ui/webui/bookmarks/bookmarks_ui.h"
#include "chrome/browser/undo/bookmark_undo_service_factory.h"
#include "chrome/common/chrome_switches.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sync_bookmarks/bookmark_sync_service.h"
#include "components/undo/bookmark_undo_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#include "sync/file_sync/file_store_factory.h"

namespace {

using bookmarks::BookmarkModel;

std::unique_ptr<KeyedService> BuildBookmarkModel(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  auto bookmark_model =
      std::make_unique<BookmarkModel>(std::make_unique<ChromeBookmarkClient>(
          profile, ManagedBookmarkServiceFactory::GetForProfile(profile),
          BookmarkSyncServiceFactory::GetForProfile(profile)));
  bookmark_model->set_vivaldi_synced_file_store(
      SyncedFileStoreFactory::GetForBrowserContext(context));
  bookmark_model->Load(profile->GetPrefs(), profile->GetPath());
  BookmarkUndoServiceFactory::GetForProfile(profile)->Start(
      bookmark_model.get());
  return bookmark_model;
}

}  // namespace

// static
BookmarkModel* BookmarkModelFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BookmarkModel*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BookmarkModel* BookmarkModelFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return static_cast<BookmarkModel*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

// static
BookmarkModelFactory* BookmarkModelFactory::GetInstance() {
  return base::Singleton<BookmarkModelFactory>::get();
}

// static
BrowserContextKeyedServiceFactory::TestingFactory
BookmarkModelFactory::GetDefaultFactory() {
  return base::BindRepeating(&BuildBookmarkModel);
}

BookmarkModelFactory::BookmarkModelFactory()
    : ProfileKeyedServiceFactory(
          "BookmarkModel",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Use OTR profile for Guest session.
              // (Bookmarks can be enabled in Guest sessions under some
              // enterprise policies.)
              .WithGuest(ProfileSelection::kRedirectedToOriginal)
              // No service for system profile.
              .WithSystem(ProfileSelection::kNone)
              // ChromeOS creates various profiles (login, lock screen...) that
              // do not have/need access to bookmarks.
              .WithAshInternals(ProfileSelection::kNone)
              .Build()) {
  DependsOn(BookmarkUndoServiceFactory::GetInstance());
  DependsOn(ManagedBookmarkServiceFactory::GetInstance());
  DependsOn(BookmarkSyncServiceFactory::GetInstance());
  DependsOn(SyncedFileStoreFactory::GetInstance());
}

BookmarkModelFactory::~BookmarkModelFactory() {
}

KeyedService* BookmarkModelFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return BuildBookmarkModel(context).release();
}

void BookmarkModelFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  bookmarks::RegisterProfilePrefs(registry);
}

bool BookmarkModelFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
