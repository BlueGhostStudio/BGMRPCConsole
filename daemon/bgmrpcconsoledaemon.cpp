#include "bgmrpcconsoledaemon.h"

#include <QJsonDocument>

BGMRPCConsoleDaemon::BGMRPCConsoleDaemon(QObject* parent) : QObject{ parent } {}

bool
BGMRPCConsoleDaemon::existWorkspace(const QString& workspace) {
    return m_workspace.contains(workspace);
}

QString
BGMRPCConsoleDaemon::begin(const QString& workspace, const QString& url) {
    t_workspace theWorkspace;

    if (!m_workspace.contains(workspace)) {
        theWorkspace.client = new NS_BGMRPCClient::BGMRPCClient(this);
        m_currentWorkspace = workspace;
        QObject::connect(theWorkspace.client,
                         &NS_BGMRPCClient::BGMRPCClient::stateChanged, this,
                         [=](QAbstractSocket::SocketState state) {
                             if (state == QAbstractSocket::ConnectedState &&
                                 !m_workspace.contains(workspace))
                                 m_workspace[workspace] = theWorkspace;

                             emit stateChanged(workspace, state);
                         });
        QObject::connect(theWorkspace.client,
                         &NS_BGMRPCClient::BGMRPCClient::remoteSignal, this,
                         [=](const QString& obj, const QString& sig,
                             const QJsonArray& args) {
                             emit remoteSignal(workspace, obj, sig,
                                               QJsonDocument(args).toJson(
                                                   QJsonDocument::Compact));
                         });
        QObject::connect(theWorkspace.client,
                         &NS_BGMRPCClient::BGMRPCClient::pong, this,
                         [=]() { emit pong(workspace); });
    } else
        theWorkspace = m_workspace[workspace];

    if (!theWorkspace.client->isConnected())
        theWorkspace.client->connectToHost(QUrl(url));
    else
        emit stateChanged(workspace, QAbstractSocket::ConnectedState);

    return workspace;
}

bool
BGMRPCConsoleDaemon::end(const QString& workspace) {
    if (m_workspace.contains(workspace)) {
        NS_BGMRPCClient::BGMRPCClient* client = m_workspace[workspace].client;
        QObject::connect(client, &NS_BGMRPCClient::BGMRPCClient::disconnected,
                         this, [=]() {
                             QObject::disconnect(client, 0, 0, 0);
                             m_workspace.remove(workspace);
                             client->deleteLater();
                         });
        client->disconnectFromHost();
        return true;
    } else
        return false;
}

bool
BGMRPCConsoleDaemon::reconnect(const QString& workspace) {
    if (m_workspace.contains(workspace)) {
        m_workspace[workspace].client->connectToHost();
        return true;
    } else
        return false;
}

bool
BGMRPCConsoleDaemon::ping(const QString& workspace) {
    if (m_workspace.contains(workspace)) {
        m_workspace[workspace].client->sendPing();
        return true;
    } else
        return false;
}

QStringList
BGMRPCConsoleDaemon::workspaces() {
    QStringList theList;
    foreach (const QString& theWorkspace, m_workspace.keys()) {
        theList.append(theWorkspace);
    }

    return theList;
}

QString
BGMRPCConsoleDaemon::use(const QString& workspace) {
    if (workspace.isEmpty())
        return m_currentWorkspace;
    else if (m_workspace.contains(workspace)) {
        m_currentWorkspace = workspace;

        return workspace;
    } else
        return QString();
}

bool
BGMRPCConsoleDaemon::setWorkspaceOpt(const QString& workspace,
                                     const QVariantMap& opt) {
    if (m_workspace.contains(workspace)) {
        t_workspace& theWorkspace = m_workspace[workspace];
        if (opt.contains("group")) theWorkspace.group = opt["group"].toString();
        if (opt.contains("app")) theWorkspace.app = opt["app"].toString();

        return true;
    } else
        return false;
}

QVariantMap
BGMRPCConsoleDaemon::workspaceOpt(const QString& workspace) {
    if (m_workspace.contains(workspace)) {
        const t_workspace theWorkspace = m_workspace[workspace];
        return QVariantMap(
            { { "group", theWorkspace.group }, { "app", theWorkspace.app } });
    }

    return QVariantMap();
}

bool
BGMRPCConsoleDaemon::call(const QString& workspace, const QString& obj,
                          const QString& method, const QVariantList& args,
                          bool withPrefix) {
    if (m_workspace.contains(workspace)) {
        t_workspace theWorkspace = m_workspace[workspace];
        theWorkspace.client
            ->callMethod(RPCObjName(theWorkspace, obj, withPrefix), method,
                         args)
            ->then(
                [=](const QVariant& ret) {
                    emit returnData(QJsonDocument::fromVariant(ret).toJson());
                },
                [=](const QVariant& err) {
                    emit error(QJsonDocument::fromVariant(err).toJson());
                });

        return true;
    } else
        return false;
}

bool
BGMRPCConsoleDaemon::isConnected(const QString& workspace) const {
    if (m_workspace.contains(workspace))
        return m_workspace[workspace].client->isConnected();
    else
        return false;
}

QString
BGMRPCConsoleDaemon::RPCObjName(const QString& workspace, const QString& obj,
                                bool withPrefix) {
    if (m_workspace.contains(workspace))
        return RPCObjName(m_workspace[workspace], obj, withPrefix);
    else
        return QString();
}

QString
BGMRPCConsoleDaemon::RPCObjName(const t_workspace& workspace,
                                const QString& obj, bool withPrefix) {
    if (!withPrefix) return obj;

    QStringList sp = obj.split("::");
    const QString& group = workspace.group;
    const QString& app = workspace.app;

    if (sp.length() == 2)
        return (group.length() > 0 ? group + "::" : "") + obj;
    else if (sp.length() == 1) {
        if (group.length() > 0)
            return group + "::" + app + "::" + obj;
        else
            return (app.length() > 0 ? app + "::" : "") + obj;
    } else
        return obj;
}
