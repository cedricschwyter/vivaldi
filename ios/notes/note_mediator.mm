// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#import "ios/notes/note_mediator.h"

#import <MaterialComponents/MaterialSnackbar.h>

#import "base/strings/sys_string_conversions.h"
#import "components/pref_registry/pref_registry_syncable.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/prefs/pref_names.h"
#import "ios/chrome/browser/ui/default_promo/default_browser_utils.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/url_with_title.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/notes/note_utils_ios.h"
#import "ios/notes/notes_factory.h"
#import "notes/note_node.h"
#import "notes/note_node.h"
#import "notes/notes_model.h"
#import "notes/notes_model.h"
#import "prefs/vivaldi_pref_names.h"
#import "ui/base/l10n/l10n_util.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using vivaldi::NotesModel;
using vivaldi::NoteNode;

namespace {
const int64_t kLastUsedFolderNone = -1;
}  // namespace

@interface NoteMediator ()

// BrowserState for this mediator.
@property(nonatomic, assign) ChromeBrowserState* browserState;

@end

@implementation NoteMediator

@synthesize browserState = _browserState;

+ (void)registerBrowserStatePrefs:(user_prefs::PrefRegistrySyncable*)registry {
  registry->RegisterInt64Pref(vivaldiprefs::kVivaldiNoteFolderDefault,
                              kLastUsedFolderNone);
}

+ (const NoteNode*)folderForNewNotesInBrowserState:
    (ChromeBrowserState*)browserState {
  vivaldi::NotesModel* notes =
      vivaldi::NotesModelFactory::GetForBrowserState(browserState);
  const NoteNode* defaultFolder = notes->main_node();

  PrefService* prefs = browserState->GetPrefs();
  int64_t node_id = prefs->GetInt64(vivaldiprefs::kVivaldiNoteFolderDefault);
  if (node_id == kLastUsedFolderNone)
    node_id = defaultFolder->id();
  const NoteNode* result = notes->GetNoteNodeByID(node_id);

  if (result)
    return result;

  return defaultFolder;
}

+ (void)setFolderForNewNotes:(const NoteNode*)folder
                  inBrowserState:(ChromeBrowserState*)browserState {
  DCHECK(folder && folder->is_folder());
  browserState->GetPrefs()->SetInt64(vivaldiprefs::kVivaldiNoteFolderDefault,
                                     folder->id());
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  self = [super init];
  if (self) {
    _browserState = browserState;
  }
  return self;
}

@end
