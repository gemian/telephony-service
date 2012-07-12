import QtQuick 1.1
import QtMobility.contacts 1.1
import TelephonyApp 0.1

/*
 * ContactWatcher in an element used to track changes to a specific
 * contact, based on its customId or phone number.
 * Pieces of code interested in a specific contact should create
 * an instance of ContactWatcher and set either "number" or "contactId".
 * If the contact is not available yet, this element will track the
 * contacts model events and populate the local "contact" property
 * when it becomes available.
 */

Item {
    property variant contact
    property string number
    property string contactId
    property bool __unknownContact: false

    Component.onCompleted: __checkContact()

    function __checkContact() {
        if(contactId && contactId != "") {
            contact = contactModel.contactFromCustomId(contactId);
            return;
        }
        // try to fill the contactId and avoid future queries
        if (number && (!contactId || contactId == "") && !__unknownContact) {
            contactId = contactModel.customIdFromPhoneNumber(number);
            if(contactId && contactId != "") {
                return;
            } else {
                __unknownContact = true;
                contact = null;
                return;
            }
        }
        // if this contact does not exist on database, 
        // dont waste time asking the backend about it.
        if(number && !__unknownContact) {
            contact = contactModel.contactFromPhoneNumber(number);
        }
    }
 
    Connections {
        target: contactModel
        onContactAdded: __checkContact()
        onContactRemoved: __checkContact()
    }

    onNumberChanged: {
        contactId = ""; 
        __unknownContact = false; 
        __checkContact();
    }

    onContactIdChanged: {
        __unknownContact = false; 
        __checkContact();
    }
}
