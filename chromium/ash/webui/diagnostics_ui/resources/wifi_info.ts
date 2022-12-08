// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './data_point.js';
import './diagnostics_shared.css.js';

import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {assertNotReached} from 'chrome://resources/js/assert_ts.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getSignalStrength} from './diagnostics_utils.js';
import {convertFrequencyToChannel} from './frequency_channel_utils.js';
import {Network, SecurityType} from './network_health_provider.mojom-webui.js';
import {getTemplate} from './wifi_info.html.js';


/**
 * @fileoverview
 * 'wifi-info' is responsible for displaying data points related
 * to a WiFi network.
 */

const WifiInfoElementBase = I18nMixin(PolymerElement);


export class WifiInfoElement extends WifiInfoElementBase {
  static get is() {
    return 'wifi-info';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /** @type {!Network} */
      network: {
        type: Object,
      },

      security_: {
        type: String,
        computed: 'computeSecurity_(network.typeProperties.wifi.security)',
      },

      signalStrength_: {
        type: String,
        computed:
            'computeSignalStrength(network.typeProperties.wifi.signalStrength)',
      },

    };
  }

  network: Network;
  protected security_: string;
  protected signalStrength_: string;

  /**
   * Builds channel text based frequency conversion. If value of frequency is
   * undefined or zero, then an empty string is returned. Otherwise, if value
   * returned by conversion function is null, then we display a question mark
   * for channel value. Frequency used to calculate converted from MHz to  GHz
   * for display.
   * @param frequency Given in MHz.
   */
  protected getChannelDescription_(frequency: number): string {
    if (!frequency || frequency === 0) {
      return '';
    }
    const channel = convertFrequencyToChannel(frequency);
    const ghz = (frequency / 1000).toFixed(3);
    return `${channel || '?'} (${ghz} GHz)`;
  }

  protected computeSecurity_(): string {
    if (!this.network.typeProperties) {
      return '';
    }

    switch (this.network.typeProperties?.wifi?.security) {
      case SecurityType.kNone:
        return loadTimeData.getString('networkSecurityNoneLabel');
      case SecurityType.kWep8021x:
        return loadTimeData.getString('networkSecurityWep8021xLabel');
      case SecurityType.kWepPsk:
        return loadTimeData.getString('networkSecurityWepPskLabel');
      case SecurityType.kWpaEap:
        return loadTimeData.getString('networkSecurityWpaEapLabel');
      case SecurityType.kWpaPsk:
        return loadTimeData.getString('networkSecurityWpaPskLabel');
    }
    assertNotReached();
  }

  private computeSignalStrength(): string {
    if (this.network.typeProperties && this.network.typeProperties.wifi) {
      return getSignalStrength(this.network.typeProperties.wifi.signalStrength);
    }
    return '';
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'wifi-info': WifiInfoElement;
  }
}

customElements.define(WifiInfoElement.is, WifiInfoElement);
