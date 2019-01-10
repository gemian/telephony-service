/*
 * Copyright (C) 2019 Adam Boardman
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

#ifndef COVERUI_H
#define COVERUI_H

#include <QObject>
#include <QThread>
#include <QContact>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QtDBus/QDBusInterface>
#include <unistd.h>
#include <sys/types.h>
#include <indicator/notificationdata.h>
#include <TelepathyQt/Types>

QTCONTACTS_USE_NAMESPACE

class CoverUIWorker : public QObject {
Q_OBJECT
public:
    CoverUIWorker(QObject *parent = 0);

public Q_SLOTS:

    void displayIncomingCall(const QContact &contact);

    void hideIncomingCall();

    void displayIncomingMessage();

    void hideIncomingMessage();

private:
};

class CoverUI : public QObject {
Q_OBJECT
public:
    ~CoverUI();

    static CoverUI *instance();

public Q_SLOTS:

    void displayIncomingCall(const QContact &contact);

    void hideIncomingCall();

    void displayIncomingMessage();

    void hideIncomingMessage();

private:
    explicit CoverUI(QObject *parent = 0);

    CoverUIWorker *mWorker;
    QThread mThread;
};

#endif // COVERUI_H
