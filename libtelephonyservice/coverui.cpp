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

#include "greetercontacts.h"
#include "coverui.h"
#include <QFileInfo>

CoverUIWorker::CoverUIWorker(QObject *parent) :
        QObject(parent) {
}

void CoverUIWorker::displayIncomingCall() {
    qDebug("CoverUIWorker::displayIncomingCall");
    QDBusInterface iface("org.thinkglobally.Gemian.LEDs",
                         "/org/thinkglobally/Gemian/LEDs",
                         "org.thinkglobally.Gemian.LEDs",
                         QDBusConnection::AS_BUSNAME());
    iface.asyncCall("ClearLEDBlockAnimation");
    iface.asyncCall("SetLEDBlockStep", 4u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 5u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 5u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 4u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 4u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 3u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 3u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 2u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 2u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 1u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 2u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 2u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 3u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);//next frame
    iface.asyncCall("SetLEDBlockStep", 3u, 1u, 2u, 200u);
    iface.asyncCall("SetLEDBlockStep", 4u, 1u, 1u, 200u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 20u);
    iface.asyncCall("PushLEDBlockAnimation");
}

void CoverUIWorker::hideIncomingCall() {
    qDebug("CoverUIWorker::hideIncomingCall");
    QDBusInterface iface("org.thinkglobally.Gemian.LEDs",
                         "/org/thinkglobally/Gemian/LEDs",
                         "org.thinkglobally.Gemian.LEDs",
                         QDBusConnection::AS_BUSNAME());
    iface.asyncCall("ClearLEDBlockAnimation");
    iface.asyncCall("PushLEDBlockAnimation");
}

void CoverUIWorker::displayIncomingMessage() {
    qDebug("CoverUIWorker::displayIncomingMessage");
    QDBusInterface iface("org.thinkglobally.Gemian.LEDs",
                         "/org/thinkglobally/Gemian/LEDs",
                         "org.thinkglobally.Gemian.LEDs",
                         QDBusConnection::AS_BUSNAME());
    iface.asyncCall("ClearLEDBlockAnimation");
    iface.asyncCall("SetLEDBlockStep", 3u, 1u, 1u, 255u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 10u);
    iface.asyncCall("SetLEDBlockStep", 2u, 1u, 1u, 60u);
    iface.asyncCall("SetLEDBlockStep", 4u, 1u, 1u, 60u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 30u);
    iface.asyncCall("SetLEDBlockStep", 3u, 1u, 2u, 255u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 10u);
    iface.asyncCall("SetLEDBlockStep", 2u, 1u, 2u, 255u);
    iface.asyncCall("SetLEDBlockStep", 4u, 1u, 2u, 255u);
    iface.asyncCall("SetLEDBlockStep", 1u, 0u, 3u, 60u);
    iface.asyncCall("PushLEDBlockAnimation");
}

void CoverUIWorker::hideIncomingMessage() {
    qDebug("CoverUIWorker::hideIncomingMessage");
    QDBusInterface iface("org.thinkglobally.Gemian.LEDs",
                         "/org/thinkglobally/Gemian/LEDs",
                         "org.thinkglobally.Gemian.LEDs",
                         QDBusConnection::AS_BUSNAME());
    iface.asyncCall("ClearLEDBlockAnimation");
    iface.asyncCall("PushLEDBlockAnimation");
}

CoverUI::CoverUI(QObject *parent) : QObject(parent) {
    mWorker = new CoverUIWorker();
    mWorker->moveToThread(&mThread);
    mThread.start();
}

CoverUI::~CoverUI() {
    mThread.quit();
    mThread.wait();
}

CoverUI *CoverUI::instance() {
    static auto *self = new CoverUI();
    return self;
}

void CoverUI::displayIncomingCall() {
    QMetaObject::invokeMethod(mWorker, "displayIncomingCall", Qt::QueuedConnection);
}

void CoverUI::hideIncomingCall() {
    QMetaObject::invokeMethod(mWorker, "hideIncomingCall", Qt::QueuedConnection);
}

void CoverUI::displayIncomingMessage() {
    QMetaObject::invokeMethod(mWorker, "displayIncomingMessage", Qt::QueuedConnection);
}

void CoverUI::hideIncomingMessage() {
    QMetaObject::invokeMethod(mWorker, "hideIncomingMessage", Qt::QueuedConnection);
}
