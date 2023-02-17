// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.privacy.settings;

import static org.chromium.components.content_settings.PrefNames.COOKIE_CONTROLS_MODE;

import android.os.Build;
import android.os.Bundle;
import android.text.SpannableString;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.vectordrawable.graphics.drawable.VectorDrawableCompat;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.feedback.HelpAndFeedbackLauncherImpl;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingSwitchPreference;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.prefetch.settings.PreloadPagesSettingsFragment;
import org.chromium.chrome.browser.privacy.secure_dns.SecureDnsSettings;
import org.chromium.chrome.browser.privacy_guide.PrivacyGuideInteractions;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxBridge;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxReferrer;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxSettingsBaseFragment;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.safe_browsing.metrics.SettingsAccessPoint;
import org.chromium.chrome.browser.safe_browsing.settings.SafeBrowsingSettingsFragment;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.signin.services.IdentityServicesProvider;
import org.chromium.chrome.browser.sync.settings.GoogleServicesSettings;
import org.chromium.chrome.browser.sync.settings.ManageSyncSettings;
import org.chromium.chrome.browser.usage_stats.UsageStatsConsentDialog;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.ManagedPreferenceDelegate;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.site_settings.ContentSettingsResources;
import org.chromium.components.browser_ui.site_settings.SingleCategorySettings;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.signin.identitymanager.ConsentLevel;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

// Vivaldi
import org.chromium.chrome.browser.ChromeApplicationImpl;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchManager;
import org.vivaldi.browser.preferences.VivaldiPreferences;

/**
 * Fragment to keep track of the all the privacy related preferences.
 */
public class PrivacySettings
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    private static final String PREF_CAN_MAKE_PAYMENT = "can_make_payment";
    private static final String PREF_PRELOAD_PAGES = "preload_pages";
    private static final String PREF_HTTPS_FIRST_MODE = "https_first_mode";
    private static final String PREF_SECURE_DNS = "secure_dns";
    private static final String PREF_USAGE_STATS = "usage_stats_reporting";
    private static final String PREF_DO_NOT_TRACK = "do_not_track";
    private static final String PREF_SAFE_BROWSING = "safe_browsing";
    private static final String PREF_SYNC_AND_SERVICES_LINK = "sync_and_services_link";
    private static final String PREF_CLEAR_BROWSING_DATA = "clear_browsing_data";
    private static final String PREF_PRIVACY_SANDBOX = "privacy_sandbox";
    private static final String PREF_PRIVACY_GUIDE = "privacy_guide";
    private static final String PREF_INCOGNITO_LOCK = "incognito_lock";
    private static final String PREF_THIRD_PARTY_COOKIES = "third_party_cookies";

    // Vivaldi
    private static final String PREF_CLEAR_SESSION_BROWSING_DATA = "clear_session_browsing_data";
    private static final String PREF_CONTEXTUAL_SEARCH = "contextual_search";
    private static final String PREF_WEBRTC_BROADCAST_IP = "webrtc_broadcast_ip";
    private static final String PREF_PHONE_AS_A_SECURITY_KEY = "phone_as_a_security_key";

    private ManagedPreferenceDelegate mManagedPreferenceDelegate;
    private IncognitoLockSettings mIncognitoLockSettings;

    /**
     * Vivaldi
     * Strings here must match what is defined in
     * third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.cc
     */
    private static final String WEBRTC_IP_HANDLING_POLICY_DEFAULT =
            "default";
    private static final String WEBRTC_IP_HANDLING_POLICY_DISABLE_NON_PROXIED_UDP =
            "disable_non_proxied_udp";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        PrivacyPreferencesManagerImpl privacyPrefManager =
                PrivacyPreferencesManagerImpl.getInstance();
        getActivity().setTitle(R.string.prefs_privacy_security);

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.PRIVACY_SANDBOX_SETTINGS_4)) {
            SettingsUtils.addPreferencesFromResource(this, R.xml.privacy_preferences_v2);
        } else {
            SettingsUtils.addPreferencesFromResource(this, R.xml.privacy_preferences);
        }

        if (!ChromeApplicationImpl.isVivaldi()) {
        Preference sandboxPreference = findPreference(PREF_PRIVACY_SANDBOX);
        if (PrivacySandboxBridge.isPrivacySandboxRestricted()) {
            // Hide the Privacy Sandbox if it is restricted.
            getPreferenceScreen().removePreference(sandboxPreference);
        } else {
            // Overwrite the click listener to pass a correct referrer to the fragment.
            sandboxPreference.setOnPreferenceClickListener(preference -> {
                PrivacySandboxSettingsBaseFragment.launchPrivacySandboxSettings(getContext(),
                        new SettingsLauncherImpl(), PrivacySandboxReferrer.PRIVACY_SETTINGS);
                return true;
            });
        }
        } // Vivaldi

        Preference privacyGuidePreference = findPreference(PREF_PRIVACY_GUIDE);
        // Record the launch of PG from the S&P link-row entry point
        privacyGuidePreference.setOnPreferenceClickListener(preference -> {
            RecordUserAction.record("Settings.PrivacyGuide.StartPrivacySettings");
            RecordHistogram.recordEnumeratedHistogram("Settings.PrivacyGuide.EntryExit",
                    PrivacyGuideInteractions.SETTINGS_LINK_ROW_ENTRY,
                    PrivacyGuideInteractions.MAX_VALUE);
            return false;
        });
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.PRIVACY_GUIDE)) {
            getPreferenceScreen().removePreference(privacyGuidePreference);
        }

        IncognitoReauthSettingSwitchPreference incognitoReauthPreference =
                (IncognitoReauthSettingSwitchPreference) findPreference(PREF_INCOGNITO_LOCK);
        mIncognitoLockSettings = new IncognitoLockSettings(incognitoReauthPreference);
        mIncognitoLockSettings.setUpIncognitoReauthPreference(getActivity());

        Preference safeBrowsingPreference = findPreference(PREF_SAFE_BROWSING);
        safeBrowsingPreference.setSummary(
                SafeBrowsingSettingsFragment.getSafeBrowsingSummaryString(getContext()));
        safeBrowsingPreference.setOnPreferenceClickListener((preference) -> {
            preference.getExtras().putInt(
                    SafeBrowsingSettingsFragment.ACCESS_POINT, SettingsAccessPoint.PARENT_SETTINGS);
            return false;
        });

        setHasOptionsMenu(true);

        mManagedPreferenceDelegate = createManagedPreferenceDelegate();

        ChromeSwitchPreference canMakePaymentPref =
                (ChromeSwitchPreference) findPreference(PREF_CAN_MAKE_PAYMENT);
        canMakePaymentPref.setOnPreferenceChangeListener(this);

        ChromeSwitchPreference httpsFirstModePref =
                (ChromeSwitchPreference) findPreference(PREF_HTTPS_FIRST_MODE);
        httpsFirstModePref.setVisible(
                ChromeFeatureList.isEnabled(ChromeFeatureList.HTTPS_FIRST_MODE));
        httpsFirstModePref.setOnPreferenceChangeListener(this);
        httpsFirstModePref.setManagedPreferenceDelegate(mManagedPreferenceDelegate);
        httpsFirstModePref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                              .getBoolean(Pref.HTTPS_ONLY_MODE_ENABLED));

        Preference secureDnsPref = findPreference(PREF_SECURE_DNS);
        if (ChromeApplicationImpl.isVivaldi())
            secureDnsPref.setVisible(true);
        else
        secureDnsPref.setVisible(SecureDnsSettings.isUiEnabled());

        if (ChromeApplicationImpl.isVivaldi()) {
            getPreferenceScreen().removePreference(findPreference(PREF_SYNC_AND_SERVICES_LINK));
            Preference prefSafeBrowsing = findPreference(PREF_SAFE_BROWSING);
            if (prefSafeBrowsing != null) {
                getActivity().setTitle(R.string.prefs_privacy_security);
                getPreferenceScreen().removePreference(prefSafeBrowsing);
            }
            Preference phoneAsASecurityKeyPreference = findPreference(PREF_PHONE_AS_A_SECURITY_KEY);
            if (phoneAsASecurityKeyPreference != null)
                getPreferenceScreen().removePreference(phoneAsASecurityKeyPreference);

        } else {
        Preference syncAndServicesLink = findPreference(PREF_SYNC_AND_SERVICES_LINK);
        syncAndServicesLink.setSummary(buildSyncAndServicesLink());
        }

        Preference thirdPartyCookies = findPreference(PREF_THIRD_PARTY_COOKIES);
        if (thirdPartyCookies != null) {
            thirdPartyCookies.getExtras().putString(
                    SingleCategorySettings.EXTRA_CATEGORY, thirdPartyCookies.getKey());
            thirdPartyCookies.getExtras().putString(
                    SingleCategorySettings.EXTRA_TITLE, thirdPartyCookies.getTitle().toString());
        }

        // Vivaldi
        ChromeSwitchPreference webRtcBroadcastIpPref =
                (ChromeSwitchPreference) findPreference(PREF_WEBRTC_BROADCAST_IP);
        if (webRtcBroadcastIpPref != null) {
            webRtcBroadcastIpPref.setOnPreferenceChangeListener(this);
            String policy = UserPrefs.get(Profile.getLastUsedRegularProfile()).getString(
                    Pref.WEB_RTCIP_HANDLING_POLICY);
            webRtcBroadcastIpPref.setChecked(policy.equals(WEBRTC_IP_HANDLING_POLICY_DEFAULT));
            webRtcBroadcastIpPref.setSummaryOn(
                    R.string.prefs_vivaldi_webrtc_broadcast_ip_toggle_on_label);
            webRtcBroadcastIpPref.setSummaryOff(
                    R.string.prefs_vivaldi_webrtc_broadcast_ip_toggle_off_label);
        }

        updatePreferences();
    }

    private SpannableString buildSyncAndServicesLink() {
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        NoUnderlineClickableSpan servicesLink = new NoUnderlineClickableSpan(getContext(), v -> {
            settingsLauncher.launchSettingsActivity(getActivity(), GoogleServicesSettings.class);
        });
        if (IdentityServicesProvider.get()
                        .getIdentityManager(Profile.getLastUsedRegularProfile())
                        .getPrimaryAccountInfo(ConsentLevel.SYNC)
                == null) {
            // Sync is off, show the string with one link to "Google Services".
            return SpanApplier.applySpans(
                    getString(R.string.privacy_sync_and_services_link_sync_off),
                    new SpanApplier.SpanInfo("<link>", "</link>", servicesLink));
        }
        // Otherwise, show the string with both links to "Sync" and "Google Services".
        NoUnderlineClickableSpan syncLink = new NoUnderlineClickableSpan(getContext(), v -> {
            settingsLauncher.launchSettingsActivity(getActivity(), ManageSyncSettings.class,
                    ManageSyncSettings.createArguments(false));
        });
        return SpanApplier.applySpans(getString(R.string.privacy_sync_and_services_link_sync_on),
                new SpanApplier.SpanInfo("<link1>", "</link1>", syncLink),
                new SpanApplier.SpanInfo("<link2>", "</link2>", servicesLink));
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_CAN_MAKE_PAYMENT.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(Pref.CAN_MAKE_PAYMENT_ENABLED, (boolean) newValue);
        } else if (PREF_HTTPS_FIRST_MODE.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(Pref.HTTPS_ONLY_MODE_ENABLED, (boolean) newValue);
        }
        // Vivaldi
        else if (PREF_WEBRTC_BROADCAST_IP.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setString(
                    Pref.WEB_RTCIP_HANDLING_POLICY, ((boolean) newValue)
                        ? WEBRTC_IP_HANDLING_POLICY_DEFAULT
                        : WEBRTC_IP_HANDLING_POLICY_DISABLE_NON_PROXIED_UDP);
        }

        return true;
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferences();
    }

    /**
     * Updates the preferences.
     */
    public void updatePreferences() {
        PrefService prefService = UserPrefs.get(Profile.getLastUsedRegularProfile());

        ChromeSwitchPreference canMakePaymentPref =
                (ChromeSwitchPreference) findPreference(PREF_CAN_MAKE_PAYMENT);
        if (canMakePaymentPref != null) {
            canMakePaymentPref.setChecked(prefService.getBoolean(Pref.CAN_MAKE_PAYMENT_ENABLED));
        }

        Preference doNotTrackPref = findPreference(PREF_DO_NOT_TRACK);
        if (doNotTrackPref != null) {
            doNotTrackPref.setSummary(prefService.getBoolean(Pref.ENABLE_DO_NOT_TRACK)
                            ? R.string.text_on
                            : R.string.text_off);
        }

        Preference preloadPagesPreference = findPreference(PREF_PRELOAD_PAGES);
        if (preloadPagesPreference != null) {
            preloadPagesPreference.setSummary(
                    PreloadPagesSettingsFragment.getPreloadPagesSummaryString(getContext()));
        }

        Preference secureDnsPref = findPreference(PREF_SECURE_DNS);
        if (secureDnsPref != null && secureDnsPref.isVisible()) {
            secureDnsPref.setSummary(SecureDnsSettings.getSummary(getContext()));
        }

        Preference safeBrowsingPreference = findPreference(PREF_SAFE_BROWSING);
        if (safeBrowsingPreference != null && safeBrowsingPreference.isVisible()) {
            safeBrowsingPreference.setSummary(
                    SafeBrowsingSettingsFragment.getSafeBrowsingSummaryString(getContext()));
        }

        Preference usageStatsPref = findPreference(PREF_USAGE_STATS);
        if (usageStatsPref != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q
                    && prefService.getBoolean(Pref.USAGE_STATS_ENABLED)) {
                usageStatsPref.setOnPreferenceClickListener(preference -> {
                    UsageStatsConsentDialog
                            .create(getActivity(), true,
                                    (didConfirm) -> {
                                        if (didConfirm) {
                                            updatePreferences();
                                        }
                                    })
                            .show();
                    return true;
                });
            } else {
                getPreferenceScreen().removePreference(usageStatsPref);
            }
        }

        if (!ChromeApplicationImpl.isVivaldi()) {
        Preference privacySandboxPreference = findPreference(PREF_PRIVACY_SANDBOX);
        if (privacySandboxPreference != null
                && !ChromeFeatureList.isEnabled(ChromeFeatureList.PRIVACY_SANDBOX_SETTINGS_4)) {
            privacySandboxPreference.setSummary(
                    PrivacySandboxSettingsBaseFragment.getStatusString(getContext()));
        }
        } // Vivaldi

        mIncognitoLockSettings.updateIncognitoReauthPreferenceIfNeeded(getActivity());

        Preference thirdPartyCookies = findPreference(PREF_THIRD_PARTY_COOKIES);
        if (thirdPartyCookies != null) {
            thirdPartyCookies.setSummary(ContentSettingsResources.getThirdPartyCookieListSummary(
                    prefService.getInteger(COOKIE_CONTROLS_MODE)));
        }

        // Vivaldi
        Preference contextualPref = findPreference(PREF_CONTEXTUAL_SEARCH);
        if (contextualPref != null)
            contextualPref.setSummary(
                    ContextualSearchManager.isContextualSearchDisabled() ?
                            R.string.text_off : R.string.text_on);

        Preference clearSessionBrowsingDataPref = findPreference(PREF_CLEAR_SESSION_BROWSING_DATA);
        if (clearSessionBrowsingDataPref != null)
            // check if the option is enabled or not
            clearSessionBrowsingDataPref.setSummary(
                    VivaldiPreferences.getSharedPreferencesManager().readBoolean(
                            VivaldiPreferences.CLEAR_SESSION_BROWSING_DATA, false) ?
                            R.string.text_on : R.string.text_off);
    }

    private ChromeManagedPreferenceDelegate createManagedPreferenceDelegate() {
        return preference -> {
            String key = preference.getKey();
            if (PREF_HTTPS_FIRST_MODE.equals(key)) {
                return UserPrefs.get(Profile.getLastUsedRegularProfile())
                        .isManagedPreference(Pref.HTTPS_ONLY_MODE_ENABLED);
            }
            return false;
        };
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        menu.clear();
        MenuItem help =
                menu.add(Menu.NONE, R.id.menu_id_targeted_help, Menu.NONE, R.string.menu_help);
        help.setIcon(VectorDrawableCompat.create(
                getResources(), R.drawable.ic_help_and_feedback, getActivity().getTheme()));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.menu_id_targeted_help) {
            HelpAndFeedbackLauncherImpl.getInstance().show(getActivity(),
                    getString(R.string.help_context_privacy), Profile.getLastUsedRegularProfile(),
                    null);
            return true;
        }
        return false;
    }
}
