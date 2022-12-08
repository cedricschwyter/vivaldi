// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.DeviceFormFactor;

import org.vivaldi.browser.panels.PanelUtils;
import org.chromium.chrome.browser.ChromeApplicationImpl;

/**
 * Utility methods for the browsing history manager.
 */
public class HistoryManagerUtils {
    /**
     * Opens the browsing history manager.
     *
     * @param activity The {@link Activity} that owns the {@link HistoryManager}.
     * @param tab The {@link Tab} to used to display the native page version of the
     *            {@link HistoryManager}.
     * @param isIncognitoSelected Whether the incognito {@TabModelSelector} is selected.
     */
    public static void showHistoryManager(Activity activity, Tab tab, boolean isIncognitoSelected) {
        if (ChromeApplicationImpl.isVivaldi()) {
            showHistoryManagerForVivaldi((ChromeActivity) activity, isIncognitoSelected);
            return;
        }
        Context appContext = ContextUtils.getApplicationContext();
        if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity)) {
            // History shows up as a tab on tablets.
            LoadUrlParams params = new LoadUrlParams(UrlConstants.NATIVE_HISTORY_URL);
                tab.loadUrl(params);
        } else {
            Intent intent = new Intent();
            intent.setClass(appContext, HistoryActivity.class);
            intent.putExtra(IntentHandler.EXTRA_PARENT_COMPONENT, activity.getComponentName());
            intent.putExtra(IntentHandler.EXTRA_INCOGNITO_MODE, isIncognitoSelected);
            activity.startActivity(intent);
        }
    }

    private static void showHistoryManagerForVivaldi(ChromeActivity activity,
                                                     boolean isIncognitoSelected) {
        PanelUtils.showPanel(activity, UrlConstants.NATIVE_HISTORY_URL, isIncognitoSelected);
    }
}
