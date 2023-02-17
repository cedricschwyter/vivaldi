// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

class ParentAccessError extends PolymerElement {
  static get is() {
    return 'parent-access-error';
  }

  static get template() {
    return html`{__html_template__}`;
  }
}
customElements.define(ParentAccessError.is, ParentAccessError);
