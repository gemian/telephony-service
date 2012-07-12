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

#include "chatmanager.h"
#include "telepathyhelper.h"

#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>

ChatManager::ChatManager(QObject *parent)
: QObject(parent)
{
}

bool ChatManager::isChattingToContact(const QString &contactId)
{
    return mChannels.contains(contactId);
}

void ChatManager::startChat(const QString &contactId)
{
    if (!mChannels.contains(contactId)) {
        // Request the contact to start chatting to
        Tp::AccountPtr account = TelepathyHelper::instance()->account();
        connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << contactId),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactsAvailable(Tp::PendingOperation*)));
    }
}

void ChatManager::endChat(const QString &contactId)
{
    // if the chat we are ending was the current one, clear the property
    if (mActiveChat == contactId) {
        setActiveChat("");
    }

    if (!mChannels.contains(contactId))
        return;

    mChannels[contactId]->requestClose();
    mChannels.remove(contactId);
    mContacts.remove(contactId);
}

void ChatManager::sendMessage(const QString &contactId, const QString &message)
{
    if (!mChannels.contains(contactId))
        return;

    mChannels[contactId]->send(message);
    emit messageSent(contactId, message);
}

void ChatManager::acknowledgeMessages(const QString &contactId)
{
    if (!mChannels.contains(contactId))
        return;

    Tp::TextChannelPtr channel = mChannels[contactId];
    channel->acknowledge(channel->messageQueue());
}

QString ChatManager::activeChat() const
{
    return mActiveChat;
}

void ChatManager::setActiveChat(const QString &value)
{
    if (value != mActiveChat) {
        mActiveChat = value;
        acknowledgeMessages(mActiveChat);
        emit activeChatChanged();
    }
}

void ChatManager::onTextChannelAvailable(Tp::TextChannelPtr channel)
{
    mChannels[channel->targetContact()->id()] = channel;

    emit chatReady(channel->targetContact()->id());
}

void ChatManager::onMessageReceived(const Tp::ReceivedMessage &message)
{
    emit messageReceived(message.sender()->id(), message.text());

    // if the message belongs to an active conversation, mark it as read
    if (message.sender()->id() == mActiveChat) {
        acknowledgeMessages(mActiveChat);
    }
}

void ChatManager::onContactsAvailable(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(op);

    if (!pc) {
        qCritical() << "The pending object is not a Tp::PendingContacts";
        return;
    }

    Tp::AccountPtr account = TelepathyHelper::instance()->account();

    // start chatting to the contacts
    Q_FOREACH(Tp::ContactPtr contact, pc->contacts()) {
        account->ensureTextChat(contact, QDateTime::currentDateTime(), "org.freedesktop.Telepathy.Client.TelephonyApp");

        // hold the ContactPtr to make sure its refcounting stays bigger than 0
        mContacts[contact->id()] = contact;
    }
}
