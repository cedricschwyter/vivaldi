// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#import "ios/notes/note_home_mediator.h"

#include "base/check.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/sync/driver/sync_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/sync_service_factory.h"
#import "ios/chrome/browser/ui/authentication/cells/table_view_signin_promo_item.h"
#import "ios/chrome/browser/ui/authentication/enterprise/enterprise_utils.h"
#import "ios/chrome/browser/ui/authentication/signin_presenter.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"
#import "ios/chrome/browser/sync/sync_observer_bridge.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_text_item.h"
#import "ios/chrome/browser/ui/table_view/table_view_model.h"
#import "ios/chrome/browser/ui/ui_feature_flags.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/notes/note_home_consumer.h"
#import "ios/notes/note_home_shared_state.h"
#import "ios/notes/note_model_bridge_observer.h"
#import "ios/notes/cells/note_home_node_item.h"
#include "notes/notes_model.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using vivaldi::NoteNode;

namespace {
// Maximum number of entries to fetch when searching.
const int kMaxNotesSearchResults = 50;
}  // namespace

namespace notes {
class NoteModelBridge;
}

@interface NoteHomeMediator () <NoteModelBridgeObserver,
                                    PrefObserverDelegate,
                                    SigninPresenter,
                                    SyncObserverModelBridge> {
  // Bridge to register for note changes.
  std::unique_ptr<notes::NoteModelBridge> _modelBridge;

  // Observer to keep track of the signin and syncing status.
  //std::unique_ptr<sync_vivaldi::SyncedNotesObserverBridge>
  //    _syncedNotesObserver;

  // Pref observer to track changes to prefs.
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  // Registrar for pref changes notifications.
  std::unique_ptr<PrefChangeRegistrar> _prefChangeRegistrar;
}

// Shared state between Note home classes.
@property(nonatomic, strong) NoteHomeSharedState* sharedState;

// The browser state for this mediator.
@property(nonatomic, assign) ChromeBrowserState* browserState;

// Sync service.
@property(nonatomic, assign) syncer::SyncService* syncService;

@end

@implementation NoteHomeMediator
@synthesize browserState = _browserState;
@synthesize consumer = _consumer;
@synthesize sharedState = _sharedState;

- (instancetype)initWithSharedState:(NoteHomeSharedState*)sharedState
                       browserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    _sharedState = sharedState;
    _browserState = browserState;
  }
  return self;
}

- (void)startMediating {
  DCHECK(self.consumer);
  DCHECK(self.sharedState);

  // Set up observers.
  _modelBridge = std::make_unique<notes::NoteModelBridge>(
      self, self.sharedState.notesModel);
  /*_syncedNotesObserver =
      std::make_unique<sync_vivaldi::SyncedNotesObserverBridge>(
          self, self.browserState);*/

  _prefChangeRegistrar = std::make_unique<PrefChangeRegistrar>();
  _prefChangeRegistrar->Init(self.browserState->GetPrefs());
  _prefObserverBridge.reset(new PrefObserverBridge(self));

  // TODO_prefObserverBridge->ObserveChangesForPreference(
  //    prefs::kEditNotesEnabled, _prefChangeRegistrar.get());

  _syncService = SyncServiceFactory::GetForBrowserState(self.browserState);

  [self computeNoteTableViewData];
}

- (void)disconnect {
  _modelBridge = nullptr;
  //_syncedNotesObserver = nullptr;
  self.browserState = nullptr;
  self.consumer = nil;
  self.sharedState = nil;
  _prefChangeRegistrar.reset();
  _prefObserverBridge.reset();
}

#pragma mark - Initial Model Setup

// Computes the notes table view based on the current root node.
- (void)computeNoteTableViewData {
  [self deleteAllItemsOrAddSectionWithIdentifier:
            NoteHomeSectionIdentifierNotes];
  [self deleteAllItemsOrAddSectionWithIdentifier:
            NoteHomeSectionIdentifierMessages];

  // Regenerate the list of all notes.
  if (!self.sharedState.notesModel->loaded() ||
      !self.sharedState.tableViewDisplayedRootNode) {
    [self updateTableViewBackground];
    return;
  }

  if (self.sharedState.tableViewDisplayedRootNode ==
      self.sharedState.notesModel->root_node()) {
    [self generateTableViewDataForRootNode];
    [self updateTableViewBackground];
    return;
  }
  [self generateTableViewData];
  [self updateTableViewBackground];
}

// Generate the table view data when the current root node is a child node.
- (void)generateTableViewData {
  if (!self.sharedState.tableViewDisplayedRootNode) {
    return;
  }
  // Add all notes and folders of the current root node to the table.
  for (const auto& child :
       self.sharedState.tableViewDisplayedRootNode->children()) {
    NoteHomeNodeItem* nodeItem =
        [[NoteHomeNodeItem alloc] initWithType:NoteHomeItemTypeNote
                                      noteNode:child.get()];
    [self.sharedState.tableViewModel
                        addItem:nodeItem
        toSectionWithIdentifier:NoteHomeSectionIdentifierNotes];
  }
}

// Generate the table view data when the current root node is the outermost
// root.
- (void)generateTableViewDataForRootNode {
  // If all the permanent nodes are empty, do not create items for any of them.
  if (![self hasNotesOrFolders]) {
    return;
  }

  // Add "Mobile Notes" to the table.
  const NoteNode* mobileNode =
      self.sharedState.notesModel->main_node();
  NoteHomeNodeItem* mobileItem =
      [[NoteHomeNodeItem alloc] initWithType:NoteHomeItemTypeNote
                                    noteNode:mobileNode];
  [self.sharedState.tableViewModel
                      addItem:mobileItem
      toSectionWithIdentifier:NoteHomeSectionIdentifierNotes];

  const NoteNode* trashNotes =
      self.sharedState.notesModel->trash_node();
  if (!trashNotes->children().empty()) {
    NoteHomeNodeItem* trashItem =
        [[NoteHomeNodeItem alloc] initWithType:NoteHomeItemTypeNote
                                      noteNode:trashNotes];
    [self.sharedState.tableViewModel
                        addItem:trashItem
        toSectionWithIdentifier:NoteHomeSectionIdentifierNotes];
  }
}

- (void)computeNoteTableViewDataMatching:(NSString*)searchText
                  orShowMessageWhenNoResults:(NSString*)noResults {
  [self deleteAllItemsOrAddSectionWithIdentifier:
            NoteHomeSectionIdentifierNotes];
  [self deleteAllItemsOrAddSectionWithIdentifier:
            NoteHomeSectionIdentifierMessages];

  std::vector<const NoteNode*> nodes;
  std::vector<std::pair<int, NoteNode::Type>> results;
  self.sharedState.notesModel->GetNotesMatching(
        base::SysNSStringToUTF16(searchText),
        kMaxNotesSearchResults,
        &results);
  for (const std::pair<int, NoteNode::Type>& node : results) {
      const NoteNode* noteNode = self.sharedState.notesModel->GetNoteNodeByID(
            static_cast<int64_t>(node.first));
      nodes.push_back(noteNode);
  }
  int count = 0;
  for (const NoteNode* node : nodes) {
    NoteHomeNodeItem* nodeItem =
        [[NoteHomeNodeItem alloc] initWithType:NoteHomeItemTypeNote
                                      noteNode:node];
    [self.sharedState.tableViewModel
                        addItem:nodeItem
        toSectionWithIdentifier:NoteHomeSectionIdentifierNotes];
    count++;
  }

  if (count == 0) {
    TableViewTextItem* item =
        [[TableViewTextItem alloc] initWithType:NoteHomeItemTypeMessage];
    item.textAlignment = NSTextAlignmentLeft;
    item.textColor = [UIColor colorNamed:kTextPrimaryColor];
    item.text = noResults;
    [self.sharedState.tableViewModel
                        addItem:item
        toSectionWithIdentifier:NoteHomeSectionIdentifierMessages];
    return;
  }

  [self updateTableViewBackground];
}

- (void)updateTableViewBackground {
  // If the current root node is the outermost root, check if we need to show
  // the spinner backgound.  Otherwise, check if we need to show the empty
  // background.
  if (self.sharedState.tableViewDisplayedRootNode ==
      self.sharedState.notesModel->root_node()) {
    /* TODO if (self.sharedState.notesModel->HasNoUserCreatedNotesOrFolders()
     &&   _syncedNotesObserver->IsPerformingInitialSync()) {
      [self.consumer
          updateTableViewBackgroundStyle:NoteHomeBackgroundStyleLoading];
    } else*/ if (![self hasNotesOrFolders]) {
      [self.consumer
          updateTableViewBackgroundStyle:NoteHomeBackgroundStyleEmpty];
    } else {
      [self.consumer
          updateTableViewBackgroundStyle:NoteHomeBackgroundStyleDefault];
    }
    return;
  }

  if (![self hasNotesOrFolders] &&
      !self.sharedState.currentlyShowingSearchResults) {
    [self.consumer
        updateTableViewBackgroundStyle:NoteHomeBackgroundStyleEmpty];
  } else {
    [self.consumer
        updateTableViewBackgroundStyle:NoteHomeBackgroundStyleDefault];
  }
}

#pragma mark - NoteModelBridgeObserver Callbacks

// NoteModelBridgeObserver Callbacks
// Instances of this class automatically observe the note model.
// The note model has loaded.
- (void)noteModelLoaded {
  [self.consumer refreshContents];
}

// The node has changed, but not its children.
- (void)noteNodeChanged:(const NoteNode*)noteNode {
  // The root folder changed. Do nothing.
  if (noteNode == self.sharedState.tableViewDisplayedRootNode) {
    return;
  }

  // A specific cell changed. Reload, if currently shown.
  if ([self itemForNode:noteNode] != nil) {
    [self.consumer refreshContents];
  }
}

// The node has not changed, but its children have.
- (void)noteNodeChildrenChanged:(const NoteNode*)noteNode {
  // In search mode, we want to refresh any changes (like undo).
  if (self.sharedState.currentlyShowingSearchResults) {
    [self.consumer refreshContents];
  }
  // The current root folder's children changed. Reload everything.
  // (When adding new folder, table is already been updated. So no need to
  // reload here.)
  if (noteNode == self.sharedState.tableViewDisplayedRootNode &&
      !self.sharedState.addingNewFolder && !self.sharedState.addingNewNote) {
    [self.consumer refreshContents];
    return;
  }
}

// The node has moved to a new parent folder.
- (void)noteNode:(const NoteNode*)noteNode
     movedFromParent:(const NoteNode*)oldParent
            toParent:(const NoteNode*)newParent {
  if (oldParent == self.sharedState.tableViewDisplayedRootNode ||
      newParent == self.sharedState.tableViewDisplayedRootNode) {
    // A folder was added or removed from the current root folder.
    [self.consumer refreshContents];
  }
}

// |node| was deleted from |folder|.
- (void)noteNodeDeleted:(const NoteNode*)node
                 fromFolder:(const NoteNode*)folder {
  if (self.sharedState.currentlyShowingSearchResults) {
    [self.consumer refreshContents];
  } else if (self.sharedState.tableViewDisplayedRootNode == node) {
    self.sharedState.tableViewDisplayedRootNode = NULL;
    [self.consumer refreshContents];
  }
}

// All non-permanent nodes have been removed.
- (void)noteModelRemovedAllNodes {
  // TODO(crbug.com/695749) Check if this case is applicable in the new UI.
}

- (NoteHomeNodeItem*)itemForNode:
    (const vivaldi::NoteNode*)noteNode {
  NSArray<TableViewItem*>* items = [self.sharedState.tableViewModel
      itemsInSectionWithIdentifier:NoteHomeSectionIdentifierNotes];
  for (TableViewItem* item in items) {
    if (item.type == NoteHomeItemTypeNote) {
      NoteHomeNodeItem* nodeItem =
          base::mac::ObjCCastStrict<NoteHomeNodeItem>(item);
      if (nodeItem.noteNode == noteNode) {
        return nodeItem;
      }
    }
  }
  return nil;
}

#pragma mark - SigninPresenter

- (void)showSignin:(ShowSigninCommand*)command {
  // Proxy this call along to the consumer.
  [self.consumer showSignin:command];
}

#pragma mark - SyncObserverModelBridge

- (void)onSyncStateChanged {
  // Permanent nodes ("Notes Bar", "Other Notes") at the root node might
  // be added after syncing.  So we need to refresh here.
  if (self.sharedState.tableViewDisplayedRootNode ==
          self.sharedState.notesModel->root_node() ||
      self.isSyncDisabledByAdministrator) {
    [self.consumer refreshContents];
    return;
  }
  [self updateTableViewBackground];
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  // Editing capability may need to be updated on the notes UI.
  // Or managed notes contents may need to be updated.
  // TODO if (preferenceName == prefs::kEditNotesEnabled) {
    [self.consumer refreshContents];
  //}
}

#pragma mark - Private Helpers

- (BOOL)hasNotesOrFolders {
  if (self.sharedState.tableViewDisplayedRootNode ==
      self.sharedState.notesModel->root_node()) {
    // The root node always has its permanent nodes. If all the permanent nodes
    // are empty, we treat it as if the root itself is empty.
    const auto& childrenOfRootNode =
        self.sharedState.tableViewDisplayedRootNode->children();
    for (const auto& child : childrenOfRootNode) {
      if (!child->children().empty()) {
        return YES;
      }
    }
    return NO;
  }
  return self.sharedState.tableViewDisplayedRootNode &&
         !self.sharedState.tableViewDisplayedRootNode->children().empty();
}

// Delete all items for the given |sectionIdentifier| section, or create it
// if it doesn't exist, hence ensuring the section exists and is empty.
- (void)deleteAllItemsOrAddSectionWithIdentifier:(NSInteger)sectionIdentifier {
  if ([self.sharedState.tableViewModel
          hasSectionForSectionIdentifier:sectionIdentifier]) {
    [self.sharedState.tableViewModel
        deleteAllItemsFromSectionWithIdentifier:sectionIdentifier];
  } else {
    [self.sharedState.tableViewModel
        addSectionWithIdentifier:sectionIdentifier];
  }
}

// Returns YES if the user cannot turn on sync for enterprise policy reasons.
- (BOOL)isSyncDisabledByAdministrator {
  DCHECK(self.syncService);
    bool syncDisabledPolicy = YES; //self.syncService->GetDisableReasons().Has(
    //  syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY);
  return syncDisabledPolicy ;
}
@end
