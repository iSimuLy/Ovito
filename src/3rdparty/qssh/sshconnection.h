/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

#include "ssherrors.h"

#include "ssh_global.h"

#include <QByteArray>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QHostAddress>

namespace QSsh {
class SftpChannel;
class SshRemoteProcess;

namespace Internal {
class SshConnectionPrivate;
} // namespace Internal

class QSSH_EXPORT SshConnectionParameters
{
public:
    enum ProxyType { DefaultProxy, NoProxy };
    enum AuthenticationType { AuthenticationByPassword, AuthenticationByKey };
    SshConnectionParameters();

    QString host;
    QString userName;
    QString password;
    QString privateKeyFile;
    int timeout; // In seconds.
    AuthenticationType authenticationType;
    quint16 port;
    ProxyType proxyType;
};

QSSH_EXPORT bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2);
QSSH_EXPORT bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2);

class QSSH_EXPORT SshConnectionInfo
{
public:
    SshConnectionInfo() : localPort(0), peerPort(0) {}
    SshConnectionInfo(const QHostAddress &la, quint16 lp, const QHostAddress &pa, quint16 pp)
        : localAddress(la), localPort(lp), peerAddress(pa), peerPort(pp) {}

    QHostAddress localAddress;
    quint16 localPort;
    QHostAddress peerAddress;
    quint16 peerPort;
};

class QSSH_EXPORT SshConnection : public QObject
{
    Q_OBJECT

public:
    enum State { Unconnected, Connecting, Connected };

    explicit SshConnection(const SshConnectionParameters &serverInfo, QObject *parent = 0);

    void connectToHost();
    void disconnectFromHost();
    State state() const;
    SshError errorState() const;
    QString errorString() const;
    SshConnectionParameters connectionParameters() const;
    SshConnectionInfo connectionInfo() const;
    ~SshConnection();

    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();

    // -1 if an error occurred, number of channels closed otherwise.
    int closeAllChannels();

    int channelCount() const;

Q_SIGNALS:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void error(QSsh::SshError);

private:
    Internal::SshConnectionPrivate *d;
};

} // namespace QSsh

#endif // SSHCONNECTION_H
