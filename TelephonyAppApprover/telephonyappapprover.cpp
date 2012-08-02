/*
 * Copyright (C) 2012 Canonical, Ltd.
 *
 * Authors:
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
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

#include "telephonyappapprover.h"

#include <QDebug>

#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/CallChannel>
#include <TelepathyQt/TextChannel>

#define TELEPHONY_APP_CLIENT TP_QT_IFACE_CLIENT + ".TelephonyApp"
#define ANDROID_DBUS_ADDRESS "com.canonical.Android"
#define ANDROID_TELEPHONY_DBUS_PATH "/com/canonical/android/telephony/Telephony"
#define ANDROID_TELEPHONY_DBUS_IFACE "com.canonical.android.telephony.Telephony"

TelephonyAppApprover::TelephonyAppApprover()
: Tp::AbstractClientApprover(channelFilters()),
  mPendingSnapDecision(NULL)
{
    // Setup a DBus watcher to check if the telephony-app is running
    mTelephonyAppWatcher.setConnection(QDBusConnection::sessionBus());
    mTelephonyAppWatcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    mTelephonyAppWatcher.addWatchedService(TELEPHONY_APP_CLIENT);

    connect(&mTelephonyAppWatcher,
            SIGNAL(serviceRegistered(const QString&)),
            SLOT(onServiceRegistered(const QString&)));
    connect(&mTelephonyAppWatcher,
            SIGNAL(serviceUnregistered(const QString&)),
            SLOT(onServiceUnregistered(const QString&)));

    QDBusReply<bool> reply = QDBusConnection::sessionBus().interface()->isServiceRegistered(TELEPHONY_APP_CLIENT);
    if (reply.isValid()) {
        mTelephonyAppRunning = reply.value();
    } else {
        mTelephonyAppRunning = false;
    }
}

TelephonyAppApprover::~TelephonyAppApprover()
{
}

Tp::ChannelClassSpecList TelephonyAppApprover::channelFilters() const
{
    Tp::ChannelClassSpecList specList;
    specList << Tp::ChannelClassSpec::audioCall();
    specList << Tp::ChannelClassSpec::textChat();

    return specList;
}

Tp::ChannelDispatchOperationPtr TelephonyAppApprover::dispatchOperation(Tp::PendingOperation *op)
{
    Tp::ChannelPtr channel = Tp::ChannelPtr::dynamicCast(mChannels[op]);
    QString accountId = channel->property("accountId").toString();
    foreach (Tp::ChannelDispatchOperationPtr dispatchOperation, mDispatchOps) {
        if (dispatchOperation->account()->uniqueIdentifier() == accountId) {
            return dispatchOperation;
        }
    }
    return Tp::ChannelDispatchOperationPtr();
}

void TelephonyAppApprover::addDispatchOperation(const Tp::MethodInvocationContextPtr<> &context,
                                        const Tp::ChannelDispatchOperationPtr &dispatchOperation)
{
    bool willHandle = false;

    QList<Tp::ChannelPtr> channels = dispatchOperation->channels();
    foreach (Tp::ChannelPtr channel, channels) {

        // Call Channel
        Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(channel);
        if (!callChannel.isNull()) {
            Tp::PendingReady *pr = callChannel->becomeReady(Tp::Features()
                                  << Tp::CallChannel::FeatureCore
                                  << Tp::CallChannel::FeatureCallState);
            mChannels[pr] = callChannel;

            connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onChannelReady(Tp::PendingOperation*)));
            callChannel->setProperty("accountId", QVariant(dispatchOperation->account()->uniqueIdentifier()));
            willHandle = true;
            continue;
        }

        // Text Channel
        Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(channel);
        if (!textChannel.isNull()) {
            // right now we are not using any of the text channel's features in the approver
            // so no need to call becomeReady() on it.
            willHandle = true;
        }
    }

    if (willHandle) {
        mDispatchOps.append(dispatchOperation);
    }

    context->setFinished();

    // check if we need to approve channels already or if we should wait.
    processChannels();
}

class EventData {
public:
    TelephonyAppApprover* self;
    Tp::ChannelDispatchOperationPtr dispatchOp;
    Tp::ChannelPtr channel;
    Tp::PendingReady *pr;
};

void action_accept(NotifyNotification* notification,
                   char*               action,
                   gpointer            data)
{
    Q_UNUSED(notification);
    Q_UNUSED(action);

    EventData* eventData = (EventData*) data;
    TelephonyAppApprover* approver = (TelephonyAppApprover*) eventData->self;
    if (NULL != approver) {
        approver->onApproved((Tp::ChannelDispatchOperationPtr) eventData->dispatchOp,
                             (Tp::PendingReady *) eventData->pr);
        QDBusInterface androidIf(ANDROID_DBUS_ADDRESS,
                                 ANDROID_TELEPHONY_DBUS_PATH,
                                 ANDROID_TELEPHONY_DBUS_IFACE);
        androidIf.call("turnOnSpeaker", true, false);
    }
}

void action_reject(NotifyNotification* notification,
                   char*               action,
                   gpointer            data)
{
    Q_UNUSED(notification);
    Q_UNUSED(action);

    EventData* eventData = (EventData*) data;
    TelephonyAppApprover* approver = (TelephonyAppApprover*) eventData->self;
    if (NULL != approver) {
        approver->onRejected((Tp::ChannelDispatchOperationPtr) eventData->dispatchOp,
                             (Tp::ChannelPtr) eventData->channel);
    }
}

void delete_event_data(gpointer data) {
    if (NULL != data)
    delete (EventData*) data;
}

void TelephonyAppApprover::onChannelReady(Tp::PendingOperation *op)
{
    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    Tp::ChannelPtr channel = Tp::ChannelPtr::dynamicCast(mChannels[pr]);
    QString accountId = channel->property("accountId").toString();

    Tp::ContactPtr contact = channel->initiatorContact();
    Tp::ChannelDispatchOperationPtr dispatchOp = dispatchOperation(op);
    
    if (!dispatchOp) {
        return;
    }

    Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(mChannels[pr]);
    if (!callChannel) {
        return;
    }

    bool isIncoming = channel->initiatorContact() != dispatchOp->connection()->selfContact();

    if (isIncoming && !callChannel->isRequested() && callChannel->callState() == Tp::CallStateInitialised) {
        callChannel->setRinging();
    } else {
        onApproved(dispatchOp, NULL);
        return;
    }

    connect(channel.data(),
            SIGNAL(callStateChanged(Tp::CallState)),
            SLOT(onCallStateChanged(Tp::CallState)));

    NotifyNotification* notification;

    /* initial notification */

    EventData* data = new EventData();
    data->self = this;
    data->dispatchOp = dispatchOp;
    data->channel = channel;
    data->pr = pr;

    // if the contact is not known, the alias and the number will be the same
    bool unknown = true;
    QString title;
    if (contact->alias() != contact->id()) {
        unknown = false;
        title = contact->alias();
    } else {
        title = "Unknown caller";
    }

    QString body;
    if (!contact->id().isEmpty()) {
        body = QString("Calling from %1").arg(contact->id());
    } else {
        body = "Caller number is not available";
    }

    QString icon;
    if (!contact->avatarData().fileName.isEmpty()) {
        icon = contact->avatarData().fileName;
    } else {
        if (unknown) {
            icon = "notification-unknown-call";
        } else {
            icon = "notification-unavailable-image-call.svg";
        }
    }

    if (icon != contact->avatarData().fileName) {
        notification = notify_notification_new (title.toStdString().c_str(),
                                                body.toStdString().c_str(),
                                                icon.toStdString().c_str());
    } else {
        notification = notify_notification_new (title.toStdString().c_str(),
                                                body.toStdString().c_str(),
                                                NULL);
        notify_notification_set_hint_string(notification,
                                            "image_path",
                                            icon.toStdString().c_str());
    }
    notify_notification_set_hint_string(notification,
                                        "x-canonical-snap-decisions",
                                        "true");
    notify_notification_set_hint_string(notification,
                                        "x-canonical-private-button-tint",
                                        "true");

    notify_notification_add_action (notification,
                                    "action_accept",
                                    "Accept",
                                    action_accept,
                                    data,
                                    delete_event_data);
    notify_notification_add_action (notification,
                                    "action_decline_1",
                                    "Decline",
                                    action_reject,
                                    data,
                                    delete_event_data);

    mPendingSnapDecision = notification;

    GError *error = NULL;
    if (!notify_notification_show(notification, &error)) {
        qWarning() << "Failed to show snap decision:" << error->message;
        g_error_free (error);
    }
}

void TelephonyAppApprover::onApproved(Tp::ChannelDispatchOperationPtr dispatchOp,
                                      Tp::PendingReady *pr)
{
    dispatchOp->handleWith(TELEPHONY_APP_CLIENT);
    mDispatchOps.removeAll(dispatchOp);
    if (pr) {
        mChannels.remove(pr);
    }
    if (NULL != mPendingSnapDecision) {
        notify_notification_close(mPendingSnapDecision, NULL);
        mPendingSnapDecision = NULL;
    }
}

void TelephonyAppApprover::onRejected(Tp::ChannelDispatchOperationPtr dispatchOp,
                                      Tp::ChannelPtr channel)
{
    Tp::PendingOperation *claimop = dispatchOp->claim();
    mChannels[claimop] = channel;
    connect(claimop, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onClaimFinished(Tp::PendingOperation*)));
}

void TelephonyAppApprover::processChannels()
{
    // if the telephony app is not running, do not approve text channels
    if (!mTelephonyAppRunning) {
        return;
    }

    Q_FOREACH (Tp::ChannelDispatchOperationPtr dispatchOperation, mDispatchOps) {
        QList<Tp::ChannelPtr> channels = dispatchOperation->channels();
        Q_FOREACH (Tp::ChannelPtr channel, channels) {
            // approve only text channels
            Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(channel);
            if (textChannel.isNull()) {
                continue;
            }

            if (dispatchOperation->possibleHandlers().contains(TELEPHONY_APP_CLIENT)) {
                dispatchOperation->handleWith(TELEPHONY_APP_CLIENT);
                mDispatchOps.removeAll(dispatchOperation);
            }
            // FIXME: this shouldn't happen, but in any case, we need to check what to do when
            // the telephony app client is not available
        }
    }
}

void TelephonyAppApprover::onClaimFinished(Tp::PendingOperation* op)
{
    if(!op || op->isError()) {
        qDebug() << "onClaimFinished() error";
        // TODO do something
        return;
    }
    Tp::ChannelPtr channel = Tp::ChannelPtr::dynamicCast(mChannels[op]);
    Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(mChannels[op]);
    if (callChannel) {
        Tp::PendingOperation *hangupop = callChannel->hangup(Tp::CallStateChangeReasonUserRequested, TP_QT_ERROR_REJECTED, QString());
        mChannels[hangupop] = callChannel;
        connect(hangupop, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onHangupFinished(Tp::PendingOperation*)));
        return;
    }

    if (channel) {
        channel->requestClose();
    }
    mDispatchOps.removeAll(dispatchOperation(op));
    mChannels.remove(op);
}

void TelephonyAppApprover::onHangupFinished(Tp::PendingOperation* op)
{
    if(!op || op->isError()) {
        qDebug() << "onHangupFinished() error";
        // TODO do something
        return;
    }
    Tp::ChannelPtr channel = Tp::ChannelPtr::dynamicCast(mChannels[op]);
    if (channel) {
        channel->requestClose();
    }
    mDispatchOps.removeAll(dispatchOperation(op));
    mChannels.remove(op);
}

void TelephonyAppApprover::onCallStateChanged(Tp::CallState state)
{
    Tp::CallChannel *channel = qobject_cast<Tp::CallChannel*>(sender());
    Tp::ChannelDispatchOperationPtr dispatchOperation;
    Q_FOREACH(const Tp::ChannelDispatchOperationPtr &otherDispatchOperation, mDispatchOps) {
        Q_FOREACH(const Tp::ChannelPtr &otherChannel, otherDispatchOperation->channels()) {
            if (otherChannel.data() == channel) {
                dispatchOperation = otherDispatchOperation;
            }
        }
    }

    if(dispatchOperation.isNull()) {
        return;
    }

    if (state == Tp::CallStateEnded) {
        mDispatchOps.removeAll(dispatchOperation);
        // remove all channels and pending operations
        Q_FOREACH(const Tp::ChannelPtr &otherChannel, dispatchOperation->channels()) {
            Tp::PendingOperation* op = mChannels.key(otherChannel);
            if(op) {
                mChannels.remove(op);
            }
        }

        if (NULL != mPendingSnapDecision) {
            notify_notification_close(mPendingSnapDecision, NULL);
            mPendingSnapDecision = NULL;
        }
    } else if (state == Tp::CallStateActive) {
        onApproved(dispatchOperation, NULL);
    }
}

void TelephonyAppApprover::onServiceRegistered(const QString &serviceName)
{
    // for now we are only watching the telephony-app service, so no need to use/compare the
    // service name
    Q_UNUSED(serviceName)

    mTelephonyAppRunning = true;
    processChannels();
}

void TelephonyAppApprover::onServiceUnregistered(const QString &serviceName)
{
    // for now we are only watching the telephony-app service, so no need to use/compare the
    // service name
    Q_UNUSED(serviceName)

    mTelephonyAppRunning = false;
}

