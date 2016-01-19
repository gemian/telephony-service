/*
 * Copyright (C) 2012-2013 Canonical, Ltd.
 *
 * Authors:
 *  Ugo Riboni <ugo.riboni@canonical.com>
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
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

#include "audiooutput.h"
#include "audioroutemanager.h"
#include "callhandler.h"
#include "handlerdbus.h"
#include "handleradaptor.h"
#include "texthandler.h"
#include "telepathyhelper.h"

// Qt
#include <QtDBus/QDBusConnection>

static const char* DBUS_SERVICE = "com.canonical.TelephonyServiceHandler";
static const char* DBUS_OBJECT_PATH = "/com/canonical/TelephonyServiceHandler";

HandlerDBus::HandlerDBus(QObject* parent) : QObject(parent), mCallIndicatorVisible(false)
{
    connect(CallHandler::instance(),
            SIGNAL(callPropertiesChanged(QString,QVariantMap)),
            SIGNAL(CallPropertiesChanged(QString,QVariantMap)));
    connect(CallHandler::instance(),
            SIGNAL(callHoldingFailed(QString)),
            SIGNAL(CallHoldingFailed(QString)));
    connect(CallHandler::instance(),
            SIGNAL(conferenceCallRequestFinished(bool)),
            SIGNAL(ConferenceCallRequestFinished(bool)));
    connect(AudioRouteManager::instance(),
            SIGNAL(audioOutputsChanged(AudioOutputDBusList)),
            SIGNAL(AudioOutputsChanged(AudioOutputDBusList)));
    connect(AudioRouteManager::instance(),
            SIGNAL(activeAudioOutputChanged(QString)),
            SIGNAL(ActiveAudioOutputChanged(QString)));

}

HandlerDBus::~HandlerDBus()
{
}

QVariantMap HandlerDBus::GetCallProperties(const QString &objectPath)
{
    return CallHandler::instance()->getCallProperties(objectPath);
}

bool HandlerDBus::HasCalls()
{
    return CallHandler::instance()->hasCalls();
}

QStringList HandlerDBus::AccountIds()
{
    return TelepathyHelper::instance()->accountIds();
}

bool HandlerDBus::IsReady()
{
    return TelepathyHelper::instance()->ready();
}

bool HandlerDBus::callIndicatorVisible() const
{
    return mCallIndicatorVisible;
}

void HandlerDBus::setCallIndicatorVisible(bool visible)
{
    mCallIndicatorVisible = visible;
    Q_EMIT CallIndicatorVisibleChanged(visible);
}

void HandlerDBus::setActiveAudioOutput(const QString &id)
{
    AudioRouteManager::instance()->setActiveAudioOutput(id);
}

QString HandlerDBus::activeAudioOutput() const
{
    return AudioRouteManager::instance()->activeAudioOutput();
}

AudioOutputDBusList HandlerDBus::AudioOutputs() const
{
    return AudioRouteManager::instance()->audioOutputs();
}

bool HandlerDBus::connectToBus()
{
    bool ok = QDBusConnection::sessionBus().registerService(DBUS_SERVICE);
    if (!ok) {
        return false;
    }
    new TelephonyServiceHandlerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(DBUS_OBJECT_PATH, this);

    return true;
}

void HandlerDBus::SendMessage(const QString &accountId, const QStringList &recipients, const QString &message, const AttachmentList &attachments, const QVariantMap &properties)
{
    TextHandler::instance()->sendMessage(accountId, recipients, message, attachments, properties);
}

void HandlerDBus::AcknowledgeMessages(const QStringList &numbers, const QStringList &messageIds, const QString &accountId)
{
    TextHandler::instance()->acknowledgeMessages(numbers, messageIds, accountId);
}

void HandlerDBus::StartChat(const QString &accountId, const QStringList &participants)
{
    TextHandler::instance()->startChat(participants, accountId);
}

void HandlerDBus::StartChatRoom(const QString &accountId, const QStringList &initialParticipants, const QVariantMap &properties)
{
    TextHandler::instance()->startChatRoom(accountId, initialParticipants, properties);
}

void HandlerDBus::AcknowledgeAllMessages(const QStringList &numbers, const QString &accountId)
{
    TextHandler::instance()->acknowledgeAllMessages(numbers, accountId);
}

void HandlerDBus::StartCall(const QString &number, const QString &accountId)
{
    CallHandler::instance()->startCall(number, accountId);
}

void HandlerDBus::HangUpCall(const QString &objectPath)
{
    CallHandler::instance()->hangUpCall(objectPath);
}

void HandlerDBus::SetHold(const QString &objectPath, bool hold)
{
    CallHandler::instance()->setHold(objectPath, hold);
}

void HandlerDBus::SetMuted(const QString &objectPath, bool muted)
{
    CallHandler::instance()->setMuted(objectPath, muted);
}

void HandlerDBus::SendDTMF(const QString &objectPath, const QString &key)
{
    CallHandler::instance()->sendDTMF(objectPath, key);
}

void HandlerDBus::CreateConferenceCall(const QStringList &objectPaths)
{
    CallHandler::instance()->createConferenceCall(objectPaths);
}

void HandlerDBus::MergeCall(const QString &conferenceObjectPath, const QString &callObjectPath)
{
    CallHandler::instance()->mergeCall(conferenceObjectPath, callObjectPath);
}

void HandlerDBus::SplitCall(const QString &objectPath)
{
    CallHandler::instance()->splitCall(objectPath);
}
