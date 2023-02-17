// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_coordinator.h"

#import "base/feature_list.h"
#import "base/metrics/field_trial_params.h"
#import "base/metrics/histogram_functions.h"
#import "base/metrics/user_metrics.h"
#import "base/metrics/user_metrics_action.h"
#import "base/time/time.h"
#import "components/feed/core/v2/public/ios/pref_names.h"
#import "components/pref_registry/pref_registry_syncable.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#import "components/prefs/pref_change_registrar.h"
#import "components/prefs/pref_service.h"
#import "components/search_engines/default_search_manager.h"
#import "components/search_engines/template_url.h"
#import "components/search_engines/template_url_service.h"
#import "components/signin/public/base/signin_metrics.h"
#import "components/signin/public/identity_manager/objc/identity_manager_observer_bridge.h"
#import "ios/chrome/app/application_delegate/app_state.h"
#import "ios/chrome/app/tests_hook.h"
#import "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/discover_feed/discover_feed_observer_bridge.h"
#import "ios/chrome/browser/discover_feed/discover_feed_service.h"
#import "ios/chrome/browser/discover_feed/discover_feed_service_factory.h"
#import "ios/chrome/browser/discover_feed/feed_constants.h"
#import "ios/chrome/browser/discover_feed/feed_model_configuration.h"
#import "ios/chrome/browser/follow/follow_browser_agent.h"
#import "ios/chrome/browser/follow/followed_web_site.h"
#import "ios/chrome/browser/main/browser.h"
#import "ios/chrome/browser/ntp/features.h"
#import "ios/chrome/browser/ntp/new_tab_page_tab_helper.h"
#import "ios/chrome/browser/prefs/pref_names.h"
#import "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#import "ios/chrome/browser/search_engines/template_url_service_factory.h"
#import "ios/chrome/browser/signin/authentication_service.h"
#import "ios/chrome/browser/signin/authentication_service_factory.h"
#import "ios/chrome/browser/signin/chrome_account_manager_service_factory.h"
#import "ios/chrome/browser/signin/identity_manager_factory.h"
#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/browser_coordinator_commands.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/lens_commands.h"
#import "ios/chrome/browser/ui/commands/omnibox_commands.h"
#import "ios/chrome/browser/ui/commands/open_new_tab_command.h"
#import "ios/chrome/browser/ui/commands/show_signin_command.h"
#import "ios/chrome/browser/ui/commands/snackbar_commands.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_coordinator.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_feature.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_commands.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_synchronizer.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view_controller.h"
#import "ios/chrome/browser/ui/content_suggestions/ntp_home_mediator.h"
#import "ios/chrome/browser/ui/content_suggestions/ntp_home_metrics.h"
#import "ios/chrome/browser/ui/context_menu/link_preview/link_preview_coordinator.h"
#import "ios/chrome/browser/ui/main/layout_guide_util.h"
#import "ios/chrome/browser/ui/main/scene_state.h"
#import "ios/chrome/browser/ui/main/scene_state_browser_agent.h"
#import "ios/chrome/browser/ui/main/scene_state_observer.h"
#import "ios/chrome/browser/ui/ntp/discover_feed_constants.h"
#import "ios/chrome/browser/ui/ntp/discover_feed_preview_delegate.h"
#import "ios/chrome/browser/ui/ntp/feed_control_delegate.h"
#import "ios/chrome/browser/ui/ntp/feed_delegate.h"
#import "ios/chrome/browser/ui/ntp/feed_header_view_controller.h"
#import "ios/chrome/browser/ui/ntp/feed_management/feed_management_coordinator.h"
#import "ios/chrome/browser/ui/ntp/feed_management/feed_management_navigation_delegate.h"
#import "ios/chrome/browser/ui/ntp/feed_menu_commands.h"
#import "ios/chrome/browser/ui/ntp/feed_promos/feed_sign_in_promo_coordinator.h"
#import "ios/chrome/browser/ui/ntp/feed_sign_in_promo_delegate.h"
#import "ios/chrome/browser/ui/ntp/feed_top_section/feed_top_section_coordinator.h"
#import "ios/chrome/browser/ui/ntp/feed_wrapper_view_controller.h"
#import "ios/chrome/browser/ui/ntp/incognito_view_controller.h"
#import "ios/chrome/browser/ui/ntp/metrics/feed_metrics_recorder.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_content_delegate.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_coordinator+private.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_delegate.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_feature.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_follow_delegate.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_view_controller.h"
#import "ios/chrome/browser/ui/overscroll_actions/overscroll_actions_controller.h"
#import "ios/chrome/browser/ui/settings/utils/pref_backed_boolean.h"
#import "ios/chrome/browser/ui/ui_feature_flags.h"
#import "ios/chrome/browser/ui/util/named_guide.h"
#import "ios/chrome/browser/ui/util/util_swift.h"
#import "ios/chrome/browser/url_loading/url_loading_browser_agent.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/public/provider/chrome/browser/ui_utils/ui_utils_api.h"
#import "ios/web/public/navigation/navigation_context.h"
#import "ios/web/public/navigation/navigation_item.h"
#import "ios/web/public/navigation/navigation_manager.h"
#import "ios/web/public/web_state_observer_bridge.h"
#import "ui/base/l10n/l10n_util_mac.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "base/strings/sys_string_conversions.h"
#import "base/strings/utf_string_conversions.h"
#import "components/bookmarks/vivaldi_bookmark_kit.h"
#import "ios/chrome/app/application_delegate/url_opener.h"
#import "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/ui/commands/popup_menu_commands.h"
#import "ios/chrome/browser/ui/ntp/vivaldi_new_tab_page_view_controller.h"
#import "ios/chrome/browser/url_loading/url_loading_browser_agent.h"
#import "ios/chrome/browser/url_loading/url_loading_params.h"
#import "ios/chrome/browser/url_loading/url_loading_util.h"
#import "ios/chrome/browser/web_state_list/all_web_state_observation_forwarder.h"
#import "ios/ui/helpers/vivaldi_snapshot_store_helper.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"
#import "ios/web/public/web_state_observer_bridge.h"
#import "url/gurl.h"

using bookmarks::BookmarkModel;
using vivaldi_bookmark_kit::SetNodeThumbnail;
using vivaldi::IsVivaldiRunning;
// End Vivaldi

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NewTabPageCoordinator () <AppStateObserver,
                                     BooleanObserver,
                                     ContentSuggestionsHeaderCommands,
                                     DiscoverFeedObserverBridgeDelegate,
                                     DiscoverFeedPreviewDelegate,
                                     FeedControlDelegate,
                                     FeedDelegate,
                                     FeedManagementNavigationDelegate,
                                     FeedMenuCommands,
                                     FeedSignInPromoDelegate,
                                     FeedWrapperViewControllerDelegate,
                                     IdentityManagerObserverBridgeDelegate,
                                     NewTabPageContentDelegate,
                                     NewTabPageDelegate,
                                     NewTabPageFollowDelegate,
                                     OverscrollActionsControllerDelegate,
                                     PrefObserverDelegate,
                                     SceneStateObserver,
                                     CRWWebStateObserver,
                                     VivaldiNewTabPageViewControllerDelegate> {
  // Pref observer to track changes to prefs.
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  // Registrar for pref changes notifications.
  std::unique_ptr<PrefChangeRegistrar> _prefChangeRegistrar;

  // Observes changes in the IdentityManager.
  std::unique_ptr<signin::IdentityManagerObserverBridge>
      _identityObserverBridge;

  // Observes changes in the DiscoverFeed.
  std::unique_ptr<DiscoverFeedObserverBridge> _discoverFeedObserverBridge;

  // Vivaldi
  WebStateList* _webStateList;
  // Bridges C++ WebStateObserver methods to this new tab page coordinator.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;
  // Forwards observer methods for all WebStates in the WebStateList monitored
  // by the new tab page coordinator.
  std::unique_ptr<AllWebStateObservationForwarder>
      _allWebStateObservationForwarder;
  // End Vivaldi

}

// Coordinator for the ContentSuggestions.
@property(nonatomic, strong)
    ContentSuggestionsCoordinator* contentSuggestionsCoordinator;

// View controller for the regular NTP.
@property(nonatomic, strong) NewTabPageViewController* ntpViewController;

// Mediator owned by this Coordinator.
@property(nonatomic, strong) NTPHomeMediator* ntpMediator;

// View controller wrapping the feed.
@property(nonatomic, strong)
    FeedWrapperViewController* feedWrapperViewController;

// View controller for the incognito NTP.
@property(nonatomic, strong) IncognitoViewController* incognitoViewController;

// The timetick of the last time the NTP was displayed.
@property(nonatomic, assign) base::TimeTicks didAppearTime;

// Tracks the visibility of the NTP to report NTP usage metrics.
// True if the NTP view is currently displayed to the user.
@property(nonatomic, assign) BOOL visible;

// Whether the view is new tab view is currently presented (possibly in
// background). Used to report NTP usage metrics.
@property(nonatomic, assign) BOOL viewPresented;

// Wheter the scene is currently in foreground.
@property(nonatomic, assign) BOOL sceneInForeground;

// Handles interactions with the content suggestions header and the fake
// omnibox.
@property(nonatomic, strong)
    ContentSuggestionsHeaderSynchronizer* headerSynchronizer;

// The ViewController displayed by this Coordinator. This is the returned
// ViewController and will contain the `containedViewController` (Which can
// change depending on Feed visibility).
@property(nonatomic, strong) UIViewController* containerViewController;

// The coordinator contained ViewController.
@property(nonatomic, strong) UIViewController* containedViewController;

// PrefService used by this Coordinator.
@property(nonatomic, assign) PrefService* prefService;

// Whether the feed is expanded or collapsed. Collapsed
// means the feed header is shown, but not any of the feed content.
@property(nonatomic, strong) PrefBackedBoolean* feedExpandedPref;

// The view controller representing the selected feed, such as the Discover or
// Following feed.
@property(nonatomic, weak) UIViewController* feedViewController;

// The Coordinator to display previews for Discover feed websites. It also
// handles the actions related to them.
@property(nonatomic, strong) LinkPreviewCoordinator* linkPreviewCoordinator;

// The Coordinator to display Sign In promo UI.
@property(nonatomic, strong)
    FeedSignInPromoCoordinator* feedSignInPromoCoordinator;

// The view controller representing the NTP feed header.
@property(nonatomic, strong) FeedHeaderViewController* feedHeaderViewController;

// Alert coordinator for handling the feed header menu.
@property(nonatomic, strong) ActionSheetCoordinator* alertCoordinator;

// Authentication Service for the user's signed-in state.
@property(nonatomic, assign) AuthenticationService* authService;

// TemplateURL used to get the search engine.
@property(nonatomic, assign) TemplateURLService* templateURLService;

// DiscoverFeed Service to display the Feed.
@property(nonatomic, assign) DiscoverFeedService* discoverFeedService;

// Metrics recorder for actions relating to the feed.
@property(nonatomic, strong) FeedMetricsRecorder* feedMetricsRecorder;

// The header view controller containing the fake omnibox and logo.
@property(nonatomic, strong)
    ContentSuggestionsHeaderViewController* headerController;

// The coordinator for handling feed management.
@property(nonatomic, strong)
    FeedManagementCoordinator* feedManagementCoordinator;

// Coordinator for Feed top section.
@property(nonatomic, strong)
    FeedTopSectionCoordinator* feedTopSectionCoordinator;

// Currently selected feed. Redefined to readwrite.
@property(nonatomic, assign, readwrite) FeedType selectedFeed;

// Vivaldi
// Bookmarks model that holds all the bookmarks data
@property (assign,nonatomic) BookmarkModel* bookmarks;
// The user's browser state model used.
@property(nonatomic, assign) ChromeBrowserState* browserState;
// The item that was tapped from speed dial page
@property(nonatomic, strong) VivaldiSpeedDialItem* currentItem;
// Should take snapshot for the current web state.
@property(nonatomic, assign) BOOL captureSnapshot;
// End Vivaldi

@end

@implementation NewTabPageCoordinator

// Synthesize NewTabPageConfiguring properties.
@synthesize shouldScrollIntoFeed = _shouldScrollIntoFeed;
@synthesize baseViewController = _baseViewController;

#pragma mark - ChromeCoordinator

- (instancetype)initWithBrowser:(Browser*)browser {
  DCHECK(browser);
  self = [super initWithBaseViewController:nil browser:browser];
  if (self) {
    _containerViewController = [[UIViewController alloc] init];

    // Vivaldi
    _browserState =
        browser->GetBrowserState()->GetOriginalChromeBrowserState();
    _bookmarks = ios::BookmarkModelFactory::GetForBrowserState(_browserState);

    _webStateList = browser->GetWebStateList();
    _webStateObserver = std::make_unique<web::WebStateObserverBridge>(self);
    _allWebStateObservationForwarder =
        std::make_unique<AllWebStateObservationForwarder>(
            _webStateList, _webStateObserver.get());
    // End Vivaldi

  }
  return self;
}

- (void)start {
  if (self.started) {
    return;
  }

  DCHECK(self.browser);
  DCHECK(self.webState);
  DCHECK(self.toolbarDelegate);
  DCHECK(!self.contentSuggestionsCoordinator);

  // Configures incognito NTP if user is in incognito mode.
  if (self.browser->GetBrowserState()->IsOffTheRecord()) {
    DCHECK(!self.incognitoViewController);
    UrlLoadingBrowserAgent* URLLoader =
        UrlLoadingBrowserAgent::FromBrowser(self.browser);
    self.incognitoViewController =
        [[IncognitoViewController alloc] initWithUrlLoader:URLLoader];
    self.started = YES;
    return;
  }

  if (IsVivaldiRunning()) {
    [self initializeServices];
    [self configureVivaldiNTPViewController];
  } else {
  [self initializeServices];
  [self initializeNTPComponents];
  [self startObservers];

  // Updates feed asynchronously if the account is subject to parental controls.
  [self updateFeedVisibilityForSupervision];

  // Configures NTP components.
  if ([self isFeedHeaderVisible]) {
    [self configureFeedAndHeader];
  }
  [self configureHeaderController];
  [self configureNTPMediator];
  [self configureContentSuggestionsCoordinator];
  [self configureFeedMetricsRecorder];
  [self configureNTPViewController];
  } // End Vivaldi

  self.started = YES;
}

- (void)stop {
  if (!self.started)
    return;

  if (self.browser->GetBrowserState()->IsOffTheRecord()) {
    self.incognitoViewController = nil;
    self.started = NO;
    return;
  }
  self.viewPresented = NO;
  [self updateVisible];

  SceneState* sceneState =
      SceneStateBrowserAgent::FromBrowser(self.browser)->GetSceneState();
  [sceneState removeObserver:self];

  [self.feedManagementCoordinator stop];
  self.feedManagementCoordinator = nil;
  [self.contentSuggestionsCoordinator stop];
  self.contentSuggestionsCoordinator = nil;
  self.headerSynchronizer = nil;
  self.headerController = nil;
  // Remove before nil to ensure View Hierarchy doesn't hold last strong
  // reference.
  [self.containedViewController willMoveToParentViewController:nil];
  [self.containedViewController.view removeFromSuperview];
  [self.containedViewController removeFromParentViewController];
  self.containedViewController = nil;
  self.ntpViewController.feedHeaderViewController = nil;
  self.ntpViewController = nil;
  self.feedHeaderViewController.ntpDelegate = nil;
  self.feedHeaderViewController = nil;
  self.feedTopSectionCoordinator.ntpDelegate = nil;
  [self.feedTopSectionCoordinator stop];
  self.feedTopSectionCoordinator = nil;

  if (self.feedSignInPromoCoordinator) {
    [self.feedSignInPromoCoordinator stop];
    self.feedSignInPromoCoordinator = nil;
  }

  [self.linkPreviewCoordinator stop];
  self.linkPreviewCoordinator = nil;

  self.alertCoordinator = nil;
  self.authService = nil;
  self.templateURLService = nil;

  [self.ntpMediator shutdown];
  self.ntpMediator = nil;

  if (self.feedViewController) {
    self.discoverFeedService->RemoveFeedViewController(self.feedViewController);
  }
  self.feedWrapperViewController = nil;
  self.feedViewController = nil;
  self.feedMetricsRecorder = nil;

  [self.feedExpandedPref setObserver:nil];
  self.feedExpandedPref = nil;

  _prefChangeRegistrar.reset();
  _prefObserverBridge.reset();
  _discoverFeedObserverBridge.reset();
  _identityObserverBridge.reset();

  self.started = NO;
}

#pragma mark - Public

- (void)setWebState:(web::WebState*)webState {
  if (_webState == webState) {
    return;
  }
  self.contentSuggestionsCoordinator.webState = webState;
  self.ntpMediator.webState = webState;
  _webState = webState;
}

- (void)stopScrolling {
  if (!self.contentSuggestionsCoordinator) {
    return;
  }
  [self.ntpViewController stopScrolling];
}

- (BOOL)isScrolledToTop {
  return [self.ntpViewController isNTPScrolledToTop];
}

- (void)willUpdateSnapshot {
  if (self.contentSuggestionsCoordinator.started) {
    [self.ntpViewController willUpdateSnapshot];
  }
}

- (void)focusFakebox {
  if (self.feedViewController) {
    [self.ntpViewController focusFakebox];
  } else {
    [self.headerController focusFakebox];
  }
}

- (void)reload {
  if (self.browser->GetBrowserState()->IsOffTheRecord()) {
    return;
  }
  self.discoverFeedService->RefreshFeed();
  [self reloadContentSuggestions];
}

- (void)locationBarDidBecomeFirstResponder {
  [self.contentSuggestionsCoordinator locationBarDidBecomeFirstResponder];
}

- (void)locationBarDidResignFirstResponder {
  [self.contentSuggestionsCoordinator locationBarDidResignFirstResponder];
}

- (void)constrainDiscoverHeaderMenuButtonNamedGuide {
  if (self.browser->GetBrowserState()->IsOffTheRecord()) {
    return;
  }
  [LayoutGuideCenterForBrowser(self.browser)
      referenceView:self.feedHeaderViewController.menuButton
          underName:kDiscoverFeedHeaderMenuGuide];
}

- (void)updateFollowingFeedHasUnseenContent:(BOOL)hasUnseenContent {
  if (![self isFollowingFeedAvailable] ||
      !IsDotEnabledForNewFollowedContent()) {
    return;
  }
  if ([self doesFollowingFeedHaveContent]) {
    [self.feedHeaderViewController
        updateFollowingSegmentDotForUnseenContent:hasUnseenContent];
  }
}

- (void)handleFeedModelDidEndUpdates:(FeedType)feedType {
  DCHECK(self.ntpViewController);
  if (!self.feedViewController || !self.ntpViewController.viewDidAppear) {
    return;
  }
  // When the visible feed has been updated, recalculate the minimum NTP height.
  if (feedType == self.selectedFeed) {
    [self.ntpViewController updateFeedInsetsForMinimumHeight];
    [self.ntpViewController updateStickyElements];
  }
}

- (void)ntpDidChangeVisibility:(BOOL)visible {
  if (!self.browser->GetBrowserState()->IsOffTheRecord()) {
    [self updateStartForVisibilityChange:visible];
    if (visible && self.started) {
      if ([self isFollowingFeedAvailable]) {
        self.ntpViewController.shouldScrollIntoFeed = self.shouldScrollIntoFeed;
        self.shouldScrollIntoFeed = NO;
        // Reassign the sort type in case it changed in another tab.
        self.feedHeaderViewController.followingFeedSortType =
            self.followingFeedSortType;
        // Update the header so that it's synced with the currently selected
        // feed, which could have been changed when a new web state was
        // inserted.
        [self.feedHeaderViewController updateForSelectedFeed];
        self.feedMetricsRecorder.feedControlDelegate = self;
        self.feedMetricsRecorder.followDelegate = self;
      }
    }
    if (!visible) {
      // Unfocus omnibox, to prevent it from lingering when it should be
      // dismissed (for example, when navigating away or when changing feed
      // visibility). Do this after the MVC classes are deallocated so no reset
      // animations are fired in response to this cancel.
      id<OmniboxCommands> omniboxCommandHandler = HandlerForProtocol(
          self.browser->GetCommandDispatcher(), OmniboxCommands);
      [omniboxCommandHandler cancelOmniboxEdit];
    }
    // Check if feed is visible before reporting NTP visibility as the feed
    // needs to be visible in order to use for metrics.
    // TODO(crbug.com/1373650) Move isFeedVisible check to the metrics recorder
    if (IsGoodVisitsMetricEnabled()) {
      if (self.started && [self isFeedVisible]) {
        [self.feedMetricsRecorder recordNTPDidChangeVisibility:visible];
      }
    }
  }

  self.viewPresented = visible;
  [self updateVisible];
}

#pragma mark - Initializers

// Gets all NTP services from the browser state.
- (void)initializeServices {
  self.authService = AuthenticationServiceFactory::GetForBrowserState(
      self.browser->GetBrowserState());
  self.templateURLService = ios::TemplateURLServiceFactory::GetForBrowserState(
      self.browser->GetBrowserState());
  self.discoverFeedService = DiscoverFeedServiceFactory::GetForBrowserState(
      self.browser->GetBrowserState());
  self.prefService =
      ChromeBrowserState::FromBrowserState(self.browser->GetBrowserState())
          ->GetPrefs();
}

// Starts all NTP observers.
- (void)startObservers {
  DCHECK(self.prefService);
  DCHECK(self.headerController);

  _prefChangeRegistrar = std::make_unique<PrefChangeRegistrar>();
  _prefChangeRegistrar->Init(self.prefService);
  _prefObserverBridge = std::make_unique<PrefObserverBridge>(self);
  _prefObserverBridge->ObserveChangesForPreference(
      prefs::kArticlesForYouEnabled, _prefChangeRegistrar.get());
  _prefObserverBridge->ObserveChangesForPreference(
      prefs::kNTPContentSuggestionsEnabled, _prefChangeRegistrar.get());
  _prefObserverBridge->ObserveChangesForPreference(
      prefs::kNTPContentSuggestionsForSupervisedUserEnabled,
      _prefChangeRegistrar.get());
  _prefObserverBridge->ObserveChangesForPreference(
      DefaultSearchManager::kDefaultSearchProviderDataPrefName,
      _prefChangeRegistrar.get());
  self.feedExpandedPref = [[PrefBackedBoolean alloc]
      initWithPrefService:self.prefService
                 prefName:feed::prefs::kArticlesListVisible];
  // Observer is necessary for multiwindow NTPs to remain in sync.
  [self.feedExpandedPref setObserver:self];

  // Start observing IdentityManager.
  signin::IdentityManager* identityManager =
      IdentityManagerFactory::GetForBrowserState(
          self.browser->GetBrowserState());
  _identityObserverBridge =
      std::make_unique<signin::IdentityManagerObserverBridge>(identityManager,
                                                              self);

  // Start observing DiscoverFeedService.
  _discoverFeedObserverBridge = std::make_unique<DiscoverFeedObserverBridge>(
      self, self.discoverFeedService);

  SceneState* sceneState =
      SceneStateBrowserAgent::FromBrowser(self.browser)->GetSceneState();
  [sceneState addObserver:self];
  self.sceneInForeground =
      sceneState.activationLevel >= SceneActivationLevelForegroundInactive;

  AppState* appState = sceneState.appState;
  [appState addObserver:self];

  // Do not focus on omnibox for voice over if there are other screens to
  // show.
  if (appState.initStage < InitStageFinal) {
    self.headerController.focusOmniboxWhenViewAppears = NO;
  }
}

// Creates all the NTP components.
- (void)initializeNTPComponents {
  self.ntpViewController = [[NewTabPageViewController alloc] init];
  self.ntpMediator = [[NTPHomeMediator alloc]
           initWithWebState:self.webState
         templateURLService:self.templateURLService
                  URLLoader:UrlLoadingBrowserAgent::FromBrowser(self.browser)
                authService:self.authService
            identityManager:IdentityManagerFactory::GetForBrowserState(
                                self.browser->GetBrowserState())
      accountManagerService:ChromeAccountManagerServiceFactory::
                                GetForBrowserState(
                                    self.browser->GetBrowserState())
                 logoVendor:ios::provider::CreateLogoVendor(self.browser,
                                                            self.webState)];
  self.headerController = [[ContentSuggestionsHeaderViewController alloc] init];
  self.headerSynchronizer = [[ContentSuggestionsHeaderSynchronizer alloc]
      initWithCollectionController:self.ntpViewController
                  headerController:self.headerController];
  self.contentSuggestionsCoordinator = [[ContentSuggestionsCoordinator alloc]
      initWithBaseViewController:nil
                         browser:self.browser];
  self.feedMetricsRecorder = self.discoverFeedService->GetFeedMetricsRecorder();
}

#pragma mark - Configurators

// Creates and configures the feed and feed header based on user prefs.
- (void)configureFeedAndHeader {
  DCHECK([self isFeedHeaderVisible]);
  DCHECK(self.ntpViewController);

  self.ntpViewController.feedHeaderViewController =
      self.feedHeaderViewController;

  // Requests feeds here if the correct flags and prefs are enabled.
  if ([self shouldFeedBeVisible]) {
    if ([self isFollowingFeedAvailable]) {
      switch (self.selectedFeed) {
        case FeedTypeDiscover:
          self.feedViewController = [self discoverFeed];
          break;
        case FeedTypeFollowing:
          self.feedViewController = [self followingFeed];
          break;
      }
    } else {
      self.feedViewController = [self discoverFeed];
    }
  }

  // Feed top section visibility is based on feed visibility, so this should
  // always be below the block that sets `feedViewController`.
  if ([self isFeedTopSectionVisible]) {
    self.feedTopSectionCoordinator = [self createFeedTopSectionCoordinator];
  }
}

// Configures `self.headerController`.
- (void)configureHeaderController {
  DCHECK(self.headerController);
  DCHECK(self.ntpMediator);
  DCHECK(self.headerSynchronizer);

  self.headerController.isGoogleDefaultSearchEngine =
      [self isGoogleDefaultSearchEngine];
  // TODO(crbug.com/1045047): Use HandlerForProtocol after commands protocol
  // clean up.
  self.headerController.dispatcher =
      static_cast<id<ApplicationCommands, BrowserCommands, OmniboxCommands,
                     FakeboxFocuser, LensCommands>>(
          self.browser->GetCommandDispatcher());
  self.headerController.commandHandler = self;
  self.headerController.delegate = self.ntpMediator;
  self.headerController.layoutGuideCenter =
      LayoutGuideCenterForBrowser(self.browser);
  self.headerController.readingListModel =
      ReadingListModelFactory::GetForBrowserState(
          self.browser->GetBrowserState());
  self.headerController.toolbarDelegate = self.toolbarDelegate;
  self.headerController.baseViewController = self.baseViewController;
  self.headerController.collectionSynchronizer = self.headerSynchronizer;
  if (NewTabPageTabHelper::FromWebState(self.webState)
          ->ShouldShowStartSurface()) {
    self.headerController.isStartShowing = YES;
  }
}

// Configures `self.ntpMediator`.
- (void)configureNTPMediator {
  DCHECK(self.ntpMediator);
  self.ntpMediator.headerCollectionInteractionHandler = self.headerSynchronizer;
  self.ntpMediator.browser = self.browser;
  self.ntpMediator.ntpViewController = self.ntpViewController;
  self.ntpMediator.feedControlDelegate = self;
  self.ntpMediator.consumer = self.headerController;
}

// Configures `self.contentSuggestionsCoordiantor`.
- (void)configureContentSuggestionsCoordinator {
  DCHECK(self.contentSuggestionsCoordinator);
  self.contentSuggestionsCoordinator.webState = self.webState;
  self.contentSuggestionsCoordinator.ntpMediator = self.ntpMediator;
  self.contentSuggestionsCoordinator.ntpDelegate = self;
  self.contentSuggestionsCoordinator.feedDelegate = self;
  [self.contentSuggestionsCoordinator start];
}

// Configures `self.feedMetricsRecorder`.
- (void)configureFeedMetricsRecorder {
  self.feedMetricsRecorder.feedControlDelegate = self;
  self.feedMetricsRecorder.followDelegate = self;
}

// Configures `self.ntpViewController` and sets it up as the main ViewController
// managed by this Coordinator.
- (void)configureNTPViewController {
  DCHECK(self.ntpViewController);

  self.ntpViewController.contentSuggestionsViewController =
      self.contentSuggestionsCoordinator.viewController;

  self.ntpViewController.panGestureHandler = self.panGestureHandler;
  self.ntpViewController.feedVisible = [self isFeedVisible];
  self.ntpViewController.headerSynchronizer = self.headerSynchronizer;

  self.feedWrapperViewController = [[FeedWrapperViewController alloc]
        initWithDelegate:self
      feedViewController:self.feedViewController];

  if ([self isFeedTopSectionVisible]) {
    self.ntpViewController.feedTopSectionViewController =
        self.feedTopSectionCoordinator.viewController;
  }

  self.ntpViewController.feedWrapperViewController =
      self.feedWrapperViewController;
  self.ntpViewController.overscrollDelegate = self;
  self.ntpViewController.ntpContentDelegate = self;

  self.ntpViewController.headerController = self.headerController;

  [self configureMainViewControllerUsing:self.ntpViewController];
  self.ntpViewController.feedMetricsRecorder = self.feedMetricsRecorder;
  self.ntpViewController.bubblePresenter = self.bubblePresenter;
}

// Configures the main ViewController managed by this Coordinator.
- (void)configureMainViewControllerUsing:
    (UIViewController*)containedViewController {

  [containedViewController
      willMoveToParentViewController:self.containerViewController];
  [self.containerViewController addChildViewController:containedViewController];
  [self.containerViewController.view addSubview:containedViewController.view];
  [containedViewController
      didMoveToParentViewController:self.containerViewController];

  containedViewController.view.translatesAutoresizingMaskIntoConstraints = NO;
  AddSameConstraints(containedViewController.view,
                     self.containerViewController.view);

  self.containedViewController = containedViewController;
}

#pragma mark - Properties

- (UIViewController*)viewController {
  // TODO(crbug.com/1348459): Stop lazy loading in NTPCoordinator.
  [self start];
  if (self.browser->GetBrowserState()->IsOffTheRecord()) {
    return self.incognitoViewController;
  } else {
    return self.containerViewController;
  }
}

- (id<ThumbStripSupporting>)thumbStripSupporting {
  return self.ntpViewController;
}

#pragma mark - NewTabPageConfiguring

- (void)selectFeedType:(FeedType)feedType {
  if (!self.ntpViewController.viewDidAppear ||
      ![self isFollowingFeedAvailable]) {
    self.selectedFeed = feedType;
    return;
  }
  [self handleFeedSelected:feedType];
}

#pragma mark - ContentSuggestionsHeaderCommands

- (void)updateForHeaderSizeChange {
  [self updateFeedLayout];
}

#pragma mark - FeedMenuCommands

- (void)openFeedMenu {
  [self.alertCoordinator stop];
  self.alertCoordinator = nil;

  // This button is in a container view that itself is in FeedHeaderVC's main
  // view. In order to anchor the alert view correctly, we need to provide the
  // button's frame as well as its superview which is the coordinate system
  // for the frame.
  UIButton* menuButton = self.feedHeaderViewController.menuButton;
  self.alertCoordinator = [[ActionSheetCoordinator alloc]
      initWithBaseViewController:self.ntpViewController
                         browser:self.browser
                           title:nil
                         message:nil
                            rect:menuButton.frame
                            view:menuButton.superview];
  __weak NewTabPageCoordinator* weakSelf = self;

  // Item for toggling the feed on/off.
  if ([self.feedExpandedPref value]) {
    [self.alertCoordinator
        addItemWithTitle:l10n_util::GetNSString(
                             IDS_IOS_DISCOVER_FEED_MENU_TURN_OFF_ITEM)
                  action:^{
                    [weakSelf setFeedVisibleFromHeader:NO];
                  }
                   style:UIAlertActionStyleDestructive];
  } else {
    [self.alertCoordinator
        addItemWithTitle:l10n_util::GetNSString(
                             IDS_IOS_DISCOVER_FEED_MENU_TURN_ON_ITEM)
                  action:^{
                    [weakSelf setFeedVisibleFromHeader:YES];
                  }
                   style:UIAlertActionStyleDefault];
  }

  // Items for signed-in users.
  if (self.authService->HasPrimaryIdentity(signin::ConsentLevel::kSignin)) {
    if ([self isFollowingFeedAvailable]) {
      [self.alertCoordinator
          addItemWithTitle:l10n_util::GetNSString(
                               IDS_IOS_DISCOVER_FEED_MENU_MANAGE_ITEM)
                    action:^{
                      [weakSelf handleFeedManageTapped];
                    }
                     style:UIAlertActionStyleDefault];
    } else {
      [self.alertCoordinator
          addItemWithTitle:l10n_util::GetNSString(
                               IDS_IOS_DISCOVER_FEED_MENU_MANAGE_ACTIVITY_ITEM)
                    action:^{
                      [weakSelf.ntpMediator handleFeedManageActivityTapped];
                    }
                     style:UIAlertActionStyleDefault];

      [self.alertCoordinator
          addItemWithTitle:l10n_util::GetNSString(
                               IDS_IOS_DISCOVER_FEED_MENU_MANAGE_INTERESTS_ITEM)
                    action:^{
                      [weakSelf.ntpMediator handleFeedManageInterestsTapped];
                    }
                     style:UIAlertActionStyleDefault];
    }
  }

  // Items for all users.
  [self.alertCoordinator
      addItemWithTitle:l10n_util::GetNSString(
                           IDS_IOS_DISCOVER_FEED_MENU_LEARN_MORE_ITEM)
                action:^{
                  [weakSelf.ntpMediator handleFeedLearnMoreTapped];
                }
                 style:UIAlertActionStyleDefault];

  [self.alertCoordinator start];
}

#pragma mark - DiscoverFeedPreviewDelegate

- (UIViewController*)discoverFeedPreviewWithURL:(const GURL)URL {
  std::string referrerURL = base::GetFieldTrialParamValueByFeature(
      kOverrideFeedSettings, kFeedSettingDiscoverReferrerParameter);
  if (referrerURL.empty()) {
    referrerURL = kDefaultDiscoverReferrer;
  }

  self.linkPreviewCoordinator =
      [[LinkPreviewCoordinator alloc] initWithBrowser:self.browser URL:URL];
  self.linkPreviewCoordinator.referrer =
      web::Referrer(GURL(referrerURL), web::ReferrerPolicyDefault);
  [self.linkPreviewCoordinator start];
  return [self.linkPreviewCoordinator linkPreviewViewController];
}

- (void)didTapDiscoverFeedPreview {
  DCHECK(self.linkPreviewCoordinator);
  [self.linkPreviewCoordinator handlePreviewAction];
  [self.linkPreviewCoordinator stop];
  self.linkPreviewCoordinator = nil;
}

#pragma mark - FeedControlDelegate

- (FollowingFeedSortType)followingFeedSortType {
  if (IsFollowingFeedDefaultSortTypeEnabled()) {
    BOOL hasDefaultBeenChanged = self.prefService->GetBoolean(
        prefs::kDefaultFollowingFeedSortTypeChanged);
    if (hasDefaultBeenChanged) {
      return (FollowingFeedSortType)self.prefService->GetInteger(
          prefs::kNTPFollowingFeedSortType);
    } else {
      return IsDefaultFollowingFeedSortTypeGroupedByPublisher()
                 ? FollowingFeedSortTypeByPublisher
                 : FollowingFeedSortTypeByLatest;
    }
  }
  // TODO(crbug.com/1352935): Add a DCHECK to make sure the coordinator isn't
  // stopped when we check this. That would require us to use the NTPHelper to
  // get this information.
  return (FollowingFeedSortType)self.prefService->GetInteger(
      prefs::kNTPFollowingFeedSortType);
}

- (void)handleFeedSelected:(FeedType)feedType {
  DCHECK([self isFollowingFeedAvailable]);

  if (self.selectedFeed == feedType) {
    return;
  }

  self.selectedFeed = feedType;

  // Saves scroll position before changing feed.
  CGFloat scrollPosition = [self.ntpViewController scrollPosition];

  if (feedType == FeedTypeFollowing && IsDotEnabledForNewFollowedContent()) {
    // Clears dot and notifies service that the Following feed content has
    // been seen.
    [self.feedHeaderViewController
        updateFollowingSegmentDotForUnseenContent:NO];
    self.discoverFeedService->SetFollowingFeedContentSeen();
  }

  [self updateNTPForFeed];

  // Scroll position resets when changing the feed, so we set it back to what it
  // was.
  [self.ntpViewController setContentOffsetToTopOfFeed:scrollPosition];
}

- (void)handleSortTypeForFollowingFeed:(FollowingFeedSortType)sortType {
  DCHECK([self isFollowingFeedAvailable]);

  if (self.feedHeaderViewController.followingFeedSortType == sortType) {
    return;
  }

  // Save the scroll position before changing sort type.
  CGFloat scrollPosition = [self.ntpViewController scrollPosition];

  [self.feedMetricsRecorder recordFollowingFeedSortTypeSelected:sortType];
  self.prefService->SetInteger(prefs::kNTPFollowingFeedSortType, sortType);
  self.prefService->SetBoolean(prefs::kDefaultFollowingFeedSortTypeChanged,
                               true);
  self.discoverFeedService->SetFollowingFeedSortType(sortType);
  self.feedHeaderViewController.followingFeedSortType = sortType;

  [self updateNTPForFeed];

  // Scroll position resets when changing the feed, so we set it back to what it
  // was.
  [self.ntpViewController setContentOffsetToTopOfFeed:scrollPosition];
}

- (BOOL)shouldFeedBeVisible {
  return [self isFeedHeaderVisible] && [self.feedExpandedPref value] &&
         !IsFeedAblationEnabled();
}

- (BOOL)isFollowingFeedAvailable {
  return IsWebChannelsEnabled() && self.authService &&
         self.authService->HasPrimaryIdentity(signin::ConsentLevel::kSignin);
}

- (NSUInteger)lastVisibleFeedCardIndex {
  return [self.feedWrapperViewController lastVisibleFeedCardIndex];
}

#pragma mark - FeedDelegate

- (void)contentSuggestionsWasUpdated {
  [self updateFeedLayout];
}

- (void)returnToRecentTabWasAdded {
  [self updateFeedLayout];
  [self setContentOffsetToTop];
}

#pragma mark - FeedManagementNavigationDelegate

- (void)handleNavigateToActivity {
  [self.ntpMediator handleFeedManageActivityTapped];
}

- (void)handleNavigateToInterests {
  [self.ntpMediator handleFeedManageInterestsTapped];
}

- (void)handleNavigateToHidden {
  [self.ntpMediator handleFeedManageHiddenTapped];
}

- (void)handleNavigateToFollowedURL:(const GURL&)url {
  [self.ntpMediator handleVisitSiteFromFollowManagementList:url];
}

#pragma mark - FeedSignInPromoDelegate

- (void)showSignInPromoUI {
  // Show a sign-in promo half sheet.
  self.feedSignInPromoCoordinator = [[FeedSignInPromoCoordinator alloc]
      initWithBaseViewController:self.ntpViewController
                         browser:self.browser];
  [self.feedSignInPromoCoordinator start];
}

- (void)showSignInUI {
  // Show sign-in and sync page.
  const signin_metrics::AccessPoint access_point =
      signin_metrics::AccessPoint::ACCESS_POINT_NTP_FEED_BOTTOM_PROMO;
  id<ApplicationCommands> handler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), ApplicationCommands);
  ShowSigninCommand* command = [[ShowSigninCommand alloc]
      initWithOperation:AuthenticationOperationSigninAndSync
            accessPoint:access_point];
  signin_metrics::RecordSigninUserActionForAccessPoint(access_point);
  [handler showSignin:command baseViewController:self.ntpViewController];
}

#pragma mark - FeedWrapperViewControllerDelegate

- (void)updateTheme {
  self.discoverFeedService->UpdateTheme();
}

#pragma mark - NewTabPageContentDelegate

- (void)reloadContentSuggestions {
  // No need to reload ContentSuggestions since the mediator receives all
  // model state changes and immediately updates the consumer with the new
  // state.
  return;
}

- (BOOL)isContentHeaderSticky {
  return [self isFollowingFeedAvailable] && [self isFeedHeaderVisible] &&
         !IsStickyHeaderDisabledForFollowingFeed();
}

- (void)signinPromoHasChangedVisibility:(BOOL)visible {
  [self.feedTopSectionCoordinator signinPromoHasChangedVisibility:visible];
}

#pragma mark - NewTabPageDelegate

- (void)updateFeedLayout {
  // If this coordinator has not finished [self start], the below will start
  // viewDidLoad before the UI is ready, failing DCHECKS.
  if (!self.started) {
    return;
  }
  [self.containedViewController.view setNeedsLayout];
  [self.containedViewController.view layoutIfNeeded];
  [self.ntpViewController updateNTPLayout];
}

- (void)setContentOffsetToTop {
  [self.ntpViewController setContentOffsetToTop];
}

- (BOOL)isGoogleDefaultSearchEngine {
  DCHECK(self.templateURLService);
  const TemplateURL* defaultURL =
      self.templateURLService->GetDefaultSearchProvider();
  BOOL isGoogleDefaultSearchProvider =
      defaultURL &&
      defaultURL->GetEngineType(self.templateURLService->search_terms_data()) ==
          SEARCH_ENGINE_GOOGLE;
  return isGoogleDefaultSearchProvider;
}

- (BOOL)isStartSurface {
  DCHECK(self.webState);
  NewTabPageTabHelper* NTPHelper =
      NewTabPageTabHelper::FromWebState(self.webState);
  return NTPHelper && NTPHelper->ShouldShowStartSurface();
}

- (void)handleFeedTopSectionClosed {
  [self.ntpViewController updateScrollPositionForFeedTopSectionClosed];
}

#pragma mark - NewTabPageFollowDelegate

- (NSUInteger)followedPublisherCount {
  return self.followedWebSites.count;
}

- (BOOL)doesFollowingFeedHaveContent {
  for (FollowedWebSite* web_site in self.followedWebSites) {
    if (web_site.available)
      return YES;
  }

  return NO;
}

- (NSArray<FollowedWebSite*>*)followedWebSites {
  FollowBrowserAgent* followBrowserAgent =
      FollowBrowserAgent::FromBrowser(self.browser);

  // Return an empty list if the BrowserAgent is null (which can happen
  // if e.g. the Browser is off-the-record).
  if (!followBrowserAgent)
    return @[];

  return followBrowserAgent->GetFollowedWebSites();
}

#pragma mark - LogoAnimationControllerOwnerOwner

- (id<LogoAnimationControllerOwner>)logoAnimationControllerOwner {
  return [self.headerController logoAnimationControllerOwner];
}

#pragma mark - OverscrollActionsControllerDelegate

- (void)overscrollActionsController:(OverscrollActionsController*)controller
                   didTriggerAction:(OverscrollAction)action {
  id<ApplicationCommands> applicationCommandsHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), ApplicationCommands);
  id<BrowserCoordinatorCommands> browserCoordinatorCommandsHandler =
      HandlerForProtocol(self.browser->GetCommandDispatcher(),
                         BrowserCoordinatorCommands);

  switch (action) {
    case OverscrollAction::NEW_TAB: {
      [applicationCommandsHandler openURLInNewTab:[OpenNewTabCommand command]];
    } break;
    case OverscrollAction::CLOSE_TAB: {
      [browserCoordinatorCommandsHandler closeCurrentTab];
      base::RecordAction(base::UserMetricsAction("OverscrollActionCloseTab"));
    } break;
    case OverscrollAction::REFRESH:
      [self reload];
      break;
    case OverscrollAction::NONE:
      NOTREACHED();
      break;
  }
}

- (BOOL)shouldAllowOverscrollActionsForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  return YES;
}

- (UIView*)toolbarSnapshotViewForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  return
      [[self.headerController toolBarView] snapshotViewAfterScreenUpdates:NO];
}

- (UIView*)headerViewForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  return self.feedWrapperViewController.view;
}

- (CGFloat)headerInsetForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  return [self.ntpViewController heightAboveFeed];
}

- (CGFloat)headerHeightForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  CGFloat height = [self.headerController toolBarView].bounds.size.height;
  CGFloat topInset = self.feedWrapperViewController.view.safeAreaInsets.top;
  return height + topInset;
}

- (CGFloat)initialContentOffsetForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  return -[self headerInsetForOverscrollActionsController:controller];
}

- (FullscreenController*)fullscreenControllerForOverscrollActionsController:
    (OverscrollActionsController*)controller {
  // Fullscreen isn't supported here.
  return nullptr;
}

#pragma mark - AppStateObserver

- (void)appState:(AppState*)appState
    didTransitionFromInitStage:(InitStage)previousInitStage {
  if (previousInitStage == InitStageFirstRun) {
    self.headerController.focusOmniboxWhenViewAppears = YES;
    [self.headerController focusAccessibilityOnOmnibox];

    [appState removeObserver:self];
  }
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  [self handleFeedVisibilityDidChange];
}

#pragma mark - DiscoverFeedObserverBridge

- (void)discoverFeedModelWasCreated {
  if (self.ntpViewController.viewDidAppear) {
    [self updateNTPForFeed];

    if (IsWebChannelsEnabled()) {
      [self.feedHeaderViewController updateForFollowingFeedVisibilityChanged];
      [self.ntpViewController updateNTPLayout];
      [self updateFeedLayout];
      [self.ntpViewController setContentOffsetToTop];
    }
  }
}

#pragma mark - IdentityManagerObserverBridgeDelegate

- (void)onPrimaryAccountChanged:
    (const signin::PrimaryAccountChangeEvent&)event {
  // An account change may trigger after the coordinator has been stopped.
  // In this case do not process the event.
  if (!self.started) {
    return;
  }
  switch (event.GetEventTypeFor(signin::ConsentLevel::kSignin)) {
    case signin::PrimaryAccountChangeEvent::Type::kSet:
    case signin::PrimaryAccountChangeEvent::Type::kCleared: {
      [self updateFeedVisibilityForSupervision];
      break;
    }
    case signin::PrimaryAccountChangeEvent::Type::kNone:
      break;
  }
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (!self.started) {
    return;
  }
  if (preferenceName == prefs::kArticlesForYouEnabled ||
      preferenceName == prefs::kNTPContentSuggestionsEnabled ||
      preferenceName == prefs::kNTPContentSuggestionsForSupervisedUserEnabled) {
    [self updateNTPForFeed];
    [self setContentOffsetToTop];
  }
  if (preferenceName ==
      DefaultSearchManager::kDefaultSearchProviderDataPrefName) {
    [self defaultSearchEngineDidChange];
  }
}

#pragma mark - SceneStateObserver

- (void)sceneState:(SceneState*)sceneState
    transitionedToActivationLevel:(SceneActivationLevel)level {
  self.sceneInForeground = level >= SceneActivationLevelForegroundInactive;
  [self updateVisible];
}

#pragma mark - Private

// Updates the feed visibility or content based on the supervision state
// of the account defined in `value`.
- (void)updateFeedWithIsSupervisedUser:(BOOL)value {
  // This may be called asynchronously after the NTP has
  // been stopped and the object has been stopped. Ignore
  // the invocation.
  PrefService* prefService = self.prefService;
  if (!prefService) {
    return;
  }

  prefService->SetBoolean(prefs::kNTPContentSuggestionsForSupervisedUserEnabled,
                          !value);
}

- (void)updateStartForVisibilityChange:(BOOL)visible {
  if (visible && self.started &&
      NewTabPageTabHelper::FromWebState(self.webState)
          ->ShouldShowStartSurface()) {
    // Start is being shown on an existing NTP, so configure it
    // appropriately.
    self.headerController.isStartShowing = YES;
    [self.contentSuggestionsCoordinator configureStartSurfaceIfNeeded];
  }
  if (!visible && NewTabPageTabHelper::FromWebState(self.webState)
                      ->ShouldShowStartSurface()) {
    // This means the NTP going away was showing Start. Reset configuration
    // since it should not show Start after disappearing.
    NewTabPageTabHelper::FromWebState(self.webState)
        ->SetShowStartSurface(false);
    self.headerController.isStartShowing = NO;
  }
}

// Updates the visible property based on viewPresented and sceneInForeground
// properties.
// Sends metrics when NTP becomes invisible.
- (void)updateVisible {
  BOOL visible = self.viewPresented && self.sceneInForeground;
  if (visible == self.visible) {
    return;
  }
  self.visible = visible;
  if (self.browser->GetBrowserState()->IsOffTheRecord()) {
    // Do not report metrics on incognito NTP.
    return;
  }
  if (visible) {
    self.didAppearTime = base::TimeTicks::Now();
    if ([self isFeedHeaderVisible]) {
      if ([self.feedExpandedPref value]) {
        ntp_home::RecordNTPImpression(ntp_home::REMOTE_SUGGESTIONS);
      } else {
        ntp_home::RecordNTPImpression(ntp_home::REMOTE_COLLAPSED);
      }
    } else {
      ntp_home::RecordNTPImpression(ntp_home::LOCAL_SUGGESTIONS);
    }
  } else {
    if (!self.didAppearTime.is_null()) {
      UmaHistogramMediumTimes("NewTabPage.TimeSpent",
                              base::TimeTicks::Now() - self.didAppearTime);
      self.didAppearTime = base::TimeTicks();
    }
  }
}

// Updates the NTP to take into account a new feed, or a change in feed
// visibility.
- (void)updateNTPForFeed {
  DCHECK(self.ntpViewController);

  if (!self.started) {
    return;
  }

  [self.ntpViewController resetViewHierarchy];

  if (self.feedViewController) {
    self.discoverFeedService->RemoveFeedViewController(self.feedViewController);
  }

  self.ntpViewController.feedWrapperViewController = nil;
  self.ntpViewController.feedTopSectionViewController = nil;
  self.feedWrapperViewController = nil;
  self.feedViewController = nil;
  self.feedTopSectionCoordinator = nil;

  // Fetches feed header and conditionally fetches feed. Feed can only be
  // visible if feed header is visible.
  if ([self isFeedHeaderVisible]) {
    [self configureFeedAndHeader];
  } else {
    self.ntpViewController.feedHeaderViewController = nil;
    self.feedHeaderViewController = nil;
  }

  if ([self isFeedTopSectionVisible]) {
    self.ntpViewController.feedTopSectionViewController =
        self.feedTopSectionCoordinator.viewController;
  }

  self.ntpViewController.feedVisible = [self isFeedVisible];

  self.feedWrapperViewController = [[FeedWrapperViewController alloc]
        initWithDelegate:self
      feedViewController:self.feedViewController];

  self.ntpViewController.feedWrapperViewController =
      self.feedWrapperViewController;

  [self.ntpViewController layoutContentInParentCollectionView];

  [self updateFeedLayout];
}

// Feed header is always visible unless it is disabled from the Chrome settings
// menu, or by an enterprise policy.
- (BOOL)isFeedHeaderVisible {
  return self.prefService->GetBoolean(prefs::kArticlesForYouEnabled) &&
         self.prefService->GetBoolean(prefs::kNTPContentSuggestionsEnabled) &&
         !IsFeedAblationEnabled() &&
         IsContentSuggestionsForSupervisedUserEnabled(self.prefService);
}

// Returns `YES` if the feed is currently visible on the NTP.
- (BOOL)isFeedVisible {
  return [self shouldFeedBeVisible] && self.feedViewController;
}

// Whether the feed top section, which contains all content between the feed
// header and the feed, is currently visible.
// TODO(crbug.com/1331010): The feed top section may include content that is not
// the signin promo, which may need to be visible when the user is signed in.
- (BOOL)isFeedTopSectionVisible {
  return IsDiscoverFeedTopSyncPromoEnabled() && [self isFeedVisible] &&
         self.authService &&
         !self.authService->HasPrimaryIdentity(signin::ConsentLevel::kSignin);
}

// Creates, configures and returns a Discover feed view controller.
- (UIViewController*)discoverFeed {
  if (tests_hook::DisableDiscoverFeed()) {
    return nil;
  }

  FeedModelConfiguration* discoverFeedConfiguration =
      [FeedModelConfiguration discoverFeedModelConfiguration];
  self.discoverFeedService->CreateFeedModel(discoverFeedConfiguration);

  UIViewController* discoverFeed =
      self.discoverFeedService->NewDiscoverFeedViewControllerWithConfiguration(
          [self feedViewControllerConfiguration]);
  return discoverFeed;
}

// Creates, configures and returns a Following feed view controller.
- (UIViewController*)followingFeed {
  if (tests_hook::DisableDiscoverFeed()) {
    return nil;
  }

  FeedModelConfiguration* followingFeedConfiguration = [FeedModelConfiguration
      followingModelConfigurationWithSortType:self.followingFeedSortType];
  self.discoverFeedService->CreateFeedModel(followingFeedConfiguration);

  UIViewController* followingFeed =
      self.discoverFeedService->NewFollowingFeedViewControllerWithConfiguration(
          [self feedViewControllerConfiguration]);
  return followingFeed;
}

// Creates, configures and returns a feed view controller configuration.
- (DiscoverFeedViewControllerConfiguration*)feedViewControllerConfiguration {
  DiscoverFeedViewControllerConfiguration* viewControllerConfig =
      [[DiscoverFeedViewControllerConfiguration alloc] init];
  viewControllerConfig.browser = self.browser;
  viewControllerConfig.scrollDelegate = self.ntpViewController;
  viewControllerConfig.previewDelegate = self;
  viewControllerConfig.signInPromoDelegate = self;

  return viewControllerConfig;
}

// Updates the visibility of the content suggestions on the NTP if the account
// is subject to parental controls.
- (void)updateFeedVisibilityForSupervision {
  DCHECK(self.prefService);
  DCHECK(self.authService);

  ios::ChromeIdentityService* identity_service =
      ios::GetChromeBrowserProvider().GetChromeIdentityService();
  id<SystemIdentity> identity =
      self.authService->GetPrimaryIdentity(signin::ConsentLevel::kSignin);
  if (!identity) {
    [self updateFeedWithIsSupervisedUser:NO];
    return;
  }

  __weak NewTabPageCoordinator* weakSelf = self;
  identity_service->IsSubjectToParentalControls(
      identity, ^(ios::ChromeIdentityCapabilityResult result) {
        [weakSelf updateFeedWithIsSupervisedUser:
                      result == ios::ChromeIdentityCapabilityResult::kTrue];
      });
}

// Handles how the NTP reacts when the default search engine is changed.
- (void)defaultSearchEngineDidChange {
  [self.feedHeaderViewController updateForDefaultSearchEngineChanged];
  [self.ntpViewController updateNTPLayout];
  [self updateFeedLayout];
  [self.ntpViewController setContentOffsetToTop];
}

// Toggles feed visibility between hidden or expanded using the feed header
// menu. A hidden feed will continue to show the header, with a modified label.
// TODO(crbug.com/1304382): Modify this comment when Web Channels is launched.
- (void)setFeedVisibleFromHeader:(BOOL)visible {
  [self.feedExpandedPref setValue:visible];
  [self.feedMetricsRecorder recordDiscoverFeedVisibilityChanged:visible];
  [self handleFeedVisibilityDidChange];
}

// Configures and returns the feed top section coordinator.
- (FeedTopSectionCoordinator*)createFeedTopSectionCoordinator {
  DCHECK(self.ntpViewController);
  FeedTopSectionCoordinator* feedTopSectionCoordinator =
      [[FeedTopSectionCoordinator alloc]
          initWithBaseViewController:self.ntpViewController
                             browser:self.browser];
  feedTopSectionCoordinator.ntpDelegate = self;
  [feedTopSectionCoordinator start];
  return feedTopSectionCoordinator;
}

- (void)handleFeedManageTapped {
  [self.feedMetricsRecorder recordHeaderMenuManageTapped];
  [self.feedManagementCoordinator stop];
  self.feedManagementCoordinator = nil;

  self.feedManagementCoordinator = [[FeedManagementCoordinator alloc]
      initWithBaseViewController:self.ntpViewController
                         browser:self.browser];
  self.feedManagementCoordinator.navigationDelegate = self;
  self.feedManagementCoordinator.feedMetricsRecorder = self.feedMetricsRecorder;
  [self.feedManagementCoordinator start];
}

// Handles how the NTP should react when the feed visbility preference is
// changed.
- (void)handleFeedVisibilityDidChange {
  [self updateNTPForFeed];
  [self setContentOffsetToTop];
  [self.feedHeaderViewController updateForFeedVisibilityChanged];
}

#pragma mark - Getters

- (FeedHeaderViewController*)feedHeaderViewController {
  DCHECK(!self.browser->GetBrowserState()->IsOffTheRecord());
  if (!_feedHeaderViewController) {
    BOOL followingSegmentDotVisible = NO;
    if (IsDotEnabledForNewFollowedContent() && IsWebChannelsEnabled()) {
      // Only show the dot if the user follows available publishers.
      followingSegmentDotVisible =
          [self doesFollowingFeedHaveContent] &&
          self.discoverFeedService->GetFollowingFeedHasUnseenContent() &&
          self.selectedFeed != FeedTypeFollowing;
    }
    _feedHeaderViewController = [[FeedHeaderViewController alloc]
        initWithFollowingFeedSortType:self.followingFeedSortType
           followingSegmentDotVisible:followingSegmentDotVisible];
    _feedHeaderViewController.feedControlDelegate = self;
    _feedHeaderViewController.ntpDelegate = self;
    _feedHeaderViewController.feedMetricsRecorder = self.feedMetricsRecorder;
    [_feedHeaderViewController.menuButton
               addTarget:self
                  action:@selector(openFeedMenu)
        forControlEvents:UIControlEventTouchUpInside];
  }
  return _feedHeaderViewController;
}

#pragma mark - VIVALDI
- (void)configureVivaldiNTPViewController {
  VivaldiNewTabPageViewController* newVC =
      [[VivaldiNewTabPageViewController alloc] initWithBrowser:self.browser];
  [newVC
      willMoveToParentViewController:self.containerViewController];
  [self.containerViewController addChildViewController:newVC];
  [self.containerViewController.view addSubview:newVC.view];
  [newVC
      didMoveToParentViewController:self.containerViewController];

  newVC.delegate = self;
  newVC.popupMenuCommandsHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), PopupMenuCommands);
  [newVC.view fillSuperview];
  self.containedViewController = newVC;
}

- (void)didTapSpeedDial:(VivaldiSpeedDialItem*)item
       captureSnapshot:(BOOL)captureSnapshot {
  self.currentItem = item;
  self.captureSnapshot = captureSnapshot;
  // Resign the edit mode of omnibox
  id<OmniboxCommands> omniboxCommandHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), OmniboxCommands);
  [omniboxCommandHandler cancelOmniboxEdit];

  // Go to the website
  UrlLoadParams params = UrlLoadParams::InCurrentTab(item.url);
  params.web_params.transition_type = ui::PAGE_TRANSITION_AUTO_BOOKMARK;
  UrlLoadingBrowserAgent::FromBrowser(self.browser)->Load(params);
}

- (void)storeSpeedDialThumbnail:(UIImage*)snapshot {
  NSString* snapshotPath = [[NSFileManager defaultManager]
                            storeThumbnailFromSnapshot:snapshot
                            SDItem:_currentItem];
  const std::string imagePathU16 = base::SysNSStringToUTF8(snapshotPath);
  SetNodeThumbnail(self.bookmarks,
                   _currentItem.bookmarkNode,
                   imagePathU16);
  _captureSnapshot = NO;
}

#pragma mark - CRWWebStateObserver

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  if (!webState || !_captureSnapshot ||
      !_currentItem || !_currentItem.bookmarkNode)
    return;

  if (success) {
    web::WebState* currentWebState =
        self.browser->GetWebStateList()->GetActiveWebState();
    if (currentWebState) {
      SnapshotTabHelper::FromWebState(currentWebState)
      ->UpdateSnapshotWithCallback(^(UIImage* snapshot) {
        [self storeSpeedDialThumbnail:snapshot];
      });
    }
  }
}
// End Vivaldi

@end
