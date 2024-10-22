// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://webui-test/mojo_webui_test_support.js';
import 'chrome://bookmarks-side-panel.top-chrome/power_bookmarks_context_menu.js';

import {BookmarksApiProxyImpl} from 'chrome://bookmarks-side-panel.top-chrome/bookmarks_api_proxy.js';
import {PowerBookmarksContextMenuElement} from 'chrome://bookmarks-side-panel.top-chrome/power_bookmarks_context_menu.js';
import {PowerBookmarksService} from 'chrome://bookmarks-side-panel.top-chrome/power_bookmarks_service.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals} from 'chrome://webui-test/chai_assert.js';

import {TestBookmarksApiProxy} from './test_bookmarks_api_proxy.js';
import {TestPowerBookmarksDelegate} from './test_power_bookmarks_delegate.js';

suite('SidePanelPowerBookmarksContextMenuTest', () => {
  let powerBookmarksContextMenu: PowerBookmarksContextMenuElement;
  let bookmarksApi: TestBookmarksApiProxy;
  let delegate: TestPowerBookmarksDelegate;
  let service: PowerBookmarksService;

  const folders: chrome.bookmarks.BookmarkTreeNode[] = [
    {
      id: '2',
      parentId: '0',
      title: 'Other Bookmarks',
      children: [
        {
          id: '3',
          parentId: '2',
          title: 'First child bookmark',
          url: 'http://child/bookmark/1/',
          dateAdded: 1,
        },
        {
          id: '4',
          parentId: '2',
          title: 'Second child bookmark',
          url: 'http://child/bookmark/2/',
          dateAdded: 3,
        },
        {
          id: '5',
          parentId: '2',
          title: 'Child folder',
          dateAdded: 2,
          children: [
            {
              id: '6',
              parentId: '5',
              title: 'Nested bookmark',
              url: 'http://nested/bookmark/',
              dateAdded: 4,
            },
          ],
        },
      ],
    },
  ];

  setup(async () => {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;

    bookmarksApi = new TestBookmarksApiProxy();
    bookmarksApi.setFolders(JSON.parse(JSON.stringify(folders)));
    BookmarksApiProxyImpl.setInstance(bookmarksApi);

    delegate = new TestPowerBookmarksDelegate();
    service = new PowerBookmarksService(delegate);
    service.startListening();

    loadTimeData.overrideValues({
      menuOpenNewTab: 'Open in new tab',
      menuOpenNewTabWithCount: 'Open all',
      menuOpenNewWindow: 'Open in new window',
      menuOpenNewWindowWithCount: 'Open all in new window',
      menuOpenIncognito: 'Open in Incognito window',
      menuOpenIncognitoWithCount: 'Open all in Incognito window',
      menuMoveToBookmarksBar: 'Move to Bookmarks Bar folder',
      menuTrackPrice: 'Track price',
      menuRename: 'Rename',
      tooltipDelete: 'Delete',
    });

    powerBookmarksContextMenu =
        document.createElement('power-bookmarks-context-menu');
    document.body.appendChild(powerBookmarksContextMenu);

    await delegate.whenCalled('onBookmarksLoaded');
  });

  test('ShowsMenuItemsForSingleSelect', () => {
    const selection = [service.findBookmarkWithId('3')!];
    powerBookmarksContextMenu.showAtPosition(
        new MouseEvent('click'), selection, false, false);

    flush();

    const menuItems = powerBookmarksContextMenu.shadowRoot!.querySelectorAll(
        '.dropdown-item');
    assertEquals(menuItems.length, 6);
    assertEquals(
        menuItems[0]!.textContent!.includes(
            loadTimeData.getString('menuOpenNewTab')),
        true);
    assertEquals(
        menuItems[1]!.textContent!.includes(
            loadTimeData.getString('menuOpenNewWindow')),
        true);
    assertEquals(
        menuItems[2]!.textContent!.includes(
            loadTimeData.getString('menuOpenIncognito')),
        true);
    assertEquals(
        menuItems[3]!.textContent!.includes(
            loadTimeData.getString('menuMoveToBookmarksBar')),
        true);
    assertEquals(
        menuItems[4]!.textContent!.includes(
            loadTimeData.getString('menuRename')),
        true);
    assertEquals(
        menuItems[5]!.textContent!.includes(
            loadTimeData.getString('tooltipDelete')),
        true);
  });

  test('ShowsMenuItemsForMultiSelect', () => {
    const selection =
        [service.findBookmarkWithId('3')!, service.findBookmarkWithId('4')!];
    powerBookmarksContextMenu.showAtPosition(
        new MouseEvent('click'), selection, false, false);

    flush();

    const menuItems = powerBookmarksContextMenu.shadowRoot!.querySelectorAll(
        '.dropdown-item');
    assertEquals(menuItems.length, 3);
    assertEquals(
        menuItems[0]!.textContent!.includes(
            loadTimeData.getString('menuOpenNewTabWithCount')),
        true);
    assertEquals(
        menuItems[1]!.textContent!.includes(
            loadTimeData.getString('menuOpenNewWindowWithCount')),
        true);
    assertEquals(
        menuItems[2]!.textContent!.includes(
            loadTimeData.getString('menuOpenIncognitoWithCount')),
        true);
  });

  test('ShowsMenuItemsForPriceTracking', () => {
    const selection = [service.findBookmarkWithId('4')!];
    powerBookmarksContextMenu.showAtPosition(
        new MouseEvent('click'), selection, true, true);

    flush();

    const menuItems = powerBookmarksContextMenu.shadowRoot!.querySelectorAll(
        '.dropdown-item');
    assertEquals(menuItems.length, 7);
    assertEquals(
        menuItems[0]!.textContent!.includes(
            loadTimeData.getString('menuOpenNewTab')),
        true);
    assertEquals(
        menuItems[1]!.textContent!.includes(
            loadTimeData.getString('menuOpenNewWindow')),
        true);
    assertEquals(
        menuItems[2]!.textContent!.includes(
            loadTimeData.getString('menuOpenIncognito')),
        true);
    assertEquals(
        menuItems[3]!.textContent!.includes(
            loadTimeData.getString('menuMoveToBookmarksBar')),
        true);
    assertEquals(
        menuItems[4]!.textContent!.includes(
            loadTimeData.getString('menuTrackPrice')),
        true);
    assertEquals(
        menuItems[5]!.textContent!.includes(
            loadTimeData.getString('menuRename')),
        true);
    assertEquals(
        menuItems[6]!.textContent!.includes(
            loadTimeData.getString('tooltipDelete')),
        true);
  });
});
