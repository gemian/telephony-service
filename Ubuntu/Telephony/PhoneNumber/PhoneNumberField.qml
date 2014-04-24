/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * Authors:
 *  Renato Araujo Oliveira Filho <renato.filho@canonical.com>
 *
 * This file is part of telephony-service.
 *
 * telephony-service is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * telephony-service is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Telephony.PhoneNumber 0.1


/*!
    \qmltype PhoneNumberField
    \inqmlmodule Ubuntu.Telephony.PhoneNumber 0.1
    \brief The PhoneNumberField element allows to format a phone-number as you type

    \b{This component is under heavy development.}

    Example:
    \qml
    Item {
        PhoneNumberField {
            autoFormat: true
            defaultRegion: "US"
        }
    \endqml
*/
TextField {
    id: phoneNumberField

    /*!
      Specifies whether the phone number format is enabled or not.

      \qmlproperty bool autoFormat
    */
    property alias autoFormat: formatter.enabled

    /*!
      Two letters region code to be used if the number does not provide a country code (+<country-code>).
      These must be provided using ISO 3166-1 two-letter country-code format. The list of the
      codes can be found here: http://www.iso.org/iso/english_country_names_and_code_elements

      \qmlproperty string defaultRegion
    */
    property alias defaultRegion: formatter.defaultRegionCode

    // private
    property bool _enableFormating: false

    AsYouTypeFormatter {
        id: formatter

        text: phoneNumberField.text
    }

    Binding {
        target: phoneNumberField
        when: phoneNumberField.autoFormat && phoneNumberField._enableFormating
        property: "text"
        value: formatter.formattedText
    }

    // only enabled formating after the first keypress to avoid problems with text property initialization
    Keys.onPressed: _enableFormating = true
}
