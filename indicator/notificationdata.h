/*
 * Copyright (C) 2012-2017 Canonical, Ltd.
 *
 * Authors:
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 *  Tiago Salem Herrmann <tiago.herrman@canonical.com>
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

#ifndef NOTIFICATIONDATA_H
#define NOTIFICATIONDATA_H

#include <QObject>
#include <QMap>
#include <QDBusInterface>
#include <QDateTime>
#include <libnotify/notify.h>

class TextChannelObserver;

class NotificationData {
public:
    NotificationData() : targetType(0), observer(NULL), notificationList(NULL) {}
    QString accountId;
    QString senderId;
    QString targetId;
    uint targetType;
    QStringList participantIds;
    QDateTime timestamp;
    QString messageText;
    QString encodedEventId;
    QString alias;
    QString roomName;
    QString icon;
    QString notificationTitle;
    QString messageReply;
    TextChannelObserver *observer;
    QMap<NotifyNotification*, NotificationData*> *notificationList;
};

#endif // NOTIFICATIONDATA_H