/*
 * Copyright (C) 2012 Canonical, Ltd.
 *
 * Authors:
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "messagelogmodel.h"
#include "contactmanager.h"
#include <TelepathyLoggerQt4/Event>
#include <TelepathyLoggerQt4/TextEvent>
#include <TelepathyLoggerQt4/Entity>

QVariant MessageLogEntry::data(int role) const
{
    switch (role) {
    case MessageLogModel::Message:
        return message;
    case MessageLogModel::Date:
        return timestamp.date().toString(Qt::DefaultLocaleLongDate);
    default:
        return LogEntry::data(role);
    }
}

MessageLogModel::MessageLogModel(QObject *parent) :
    AbstractLoggerModel(parent)
{
    // set the role names
    QHash<int, QByteArray> roles = roleNames();
    roles[Message] = "message";
    roles[Date] = "date";
    setRoleNames(roles);
}


QString MessageLogModel::phoneNumber() const
{
    return mPhoneNumber;
}

void MessageLogModel::setPhoneNumber(QString value)
{
    if (mPhoneNumber != value) {
        invalidateRequests();
        mPhoneNumber = value;

        if (!mPhoneNumber.isEmpty()) {
            fetchLog(Tpl::EventTypeMaskText);
        }
        emit phoneNumberChanged();
    }
}

void MessageLogModel::appendMessage(const QString &number, const QString &message, bool incoming)
{
    MessageLogEntry *entry = new MessageLogEntry();
    entry->incoming = incoming;
    entry->phoneNumber = number;
    entry->message = message;
    entry->timestamp = QDateTime::currentDateTime();
    QContact contact = ContactManager::instance()->contactForNumber(number);
    if (!contact.isEmpty()) {
        fillContactInfo(entry, contact);
    }
    appendEntry(entry);
}

void MessageLogModel::onMessageReceived(const QString &number, const QString &message)
{
    // FIXME: find a better way to compare phone numbers
    if (number == mPhoneNumber) {
        appendMessage(number, message, true);
    }
}

void MessageLogModel::onMessageSent(const QString &number, const QString &message)
{
    // FIXME: find a better way to compare phone numbers
    if (number == mPhoneNumber) {
        appendMessage(number, message, false);
    }
}

LogEntry *MessageLogModel::createEntry(const Tpl::EventPtr &event)
{
    MessageLogEntry *entry = new MessageLogEntry();
    Tpl::TextEventPtr textEvent = event.dynamicCast<Tpl::TextEvent>();

    if (!textEvent) {
        qWarning() << "The event" << event << "is not a Tpl::TextEvent!";
    }

    entry->message = textEvent->message();
    return entry;
}

void MessageLogModel::handleEntities(const Tpl::EntityPtrList &entities)
{
    // search for the entity that matches the phone number for this conversation
    // FIXME: we probably need a more reliable way than string matching for comparing phone numbers
    Q_FOREACH(const Tpl::EntityPtr &entity, entities) {
        if (entity->identifier() == mPhoneNumber) {
            requestDatesForEntities(Tpl::EntityPtrList() << entity);
            return;
        }
    }
}
