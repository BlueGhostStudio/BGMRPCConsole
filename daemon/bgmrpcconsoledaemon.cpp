#include "bgmrpcconsoledaemon.h"

#include <QJsonDocument>
#include <QSettings>

BGMRPCConsoleDaemon::BGMRPCConsoleDaemon(QObject* parent) : QObject{ parent } {}

bool
BGMRPCConsoleDaemon::existWorkspace(const QString& workspace) {
    return m_workspace.contains(workspace);
}

QString
BGMRPCConsoleDaemon::begin(const QString& workspace, const QString& url,
                           const QString& group, const QString& app,
                           const QString& api) {
    t_workspace* theWorkspace;

    if (!m_workspace.contains(workspace)) {
        t_workspace newWorkspace;
        newWorkspace.client = new NS_BGMRPCClient::BGMRPCClient(this);
        m_workspace[workspace] = newWorkspace;

        theWorkspace = &m_workspace[workspace];

        QObject::connect(theWorkspace->client,
                         &NS_BGMRPCClient::BGMRPCClient::stateChanged, this,
                         [=](QAbstractSocket::SocketState state) {
                             emit stateChanged(workspace, state);
                         });
        QObject::connect(theWorkspace->client,
                         &NS_BGMRPCClient::BGMRPCClient::remoteSignal, this,
                         [=](const QString& obj, const QString& sig,
                             const QJsonArray& args) {
                             emit remoteSignal(workspace, obj, sig,
                                               QJsonDocument(args).toJson(
                                                   QJsonDocument::Compact));
                         });
        QObject::connect(theWorkspace->client,
                         &NS_BGMRPCClient::BGMRPCClient::pong, this,
                         [=]() { emit pong(workspace); });

        emit workspaceChanged(workspace);
    } else
        theWorkspace = &m_workspace[workspace];

    m_currentWorkspace = workspace;

    if (!group.isEmpty()) theWorkspace->group = group;
    if (!app.isEmpty()) theWorkspace->app = app;
    if (!api.isEmpty()) theWorkspace->api = api;

    if (!theWorkspace->client->isConnected())
        theWorkspace->client->connectToHost(QUrl(url));
    else
        emit stateChanged(workspace, QAbstractSocket::ConnectedState);

    return workspace;
}

bool
BGMRPCConsoleDaemon::end(const QString& workspace) {
    if (m_workspace.contains(workspace)) {
        NS_BGMRPCClient::BGMRPCClient* client = m_workspace[workspace].client;
        QObject::connect(client, &NS_BGMRPCClient::BGMRPCClient::disconnected,
                         this, [=]() { removeWorkspace(workspace); });
        if (client->isConnected())
            client->disconnectFromHost();
        else
            removeWorkspace(workspace);
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
    if (workspace.isEmpty()) {
        return m_currentWorkspace;
    } else if (m_workspace.contains(workspace)) {
        if (m_currentWorkspace != workspace) {
            m_currentWorkspace = workspace;
            emit workspaceChanged(m_currentWorkspace);
        }

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
        if (opt.contains("api")) theWorkspace.api = opt["api"].toString();

        return true;
    } else
        return false;
}

QVariantMap
BGMRPCConsoleDaemon::workspaceOpt(const QString& workspace) {
    if (m_workspace.contains(workspace)) {
        const t_workspace theWorkspace = m_workspace[workspace];
        return QVariantMap({ { "group", theWorkspace.group },
                             { "app", theWorkspace.app },
                             { "api", theWorkspace.api } });
    }

    return QVariantMap();
}

bool
BGMRPCConsoleDaemon::call(const QString& workspace, const QString& obj,
                          const QString& method, const QByteArray& args,
                          bool withPrefix, const QString& group,
                          const QString& app) {
    if (m_workspace.contains(workspace)) {
        t_workspace theWorkspace = m_workspace[workspace];
        if (!group.isEmpty()) theWorkspace.group = group;
        if (!app.isEmpty()) theWorkspace.app = app;

        theWorkspace.client
            ->callMethod(RPCObjName(theWorkspace, obj, withPrefix), method,
                         QJsonDocument::fromJson(args).toVariant().toList())
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
                                bool withPrefix, const QString& group,
                                const QString& app) {
    if (m_workspace.contains(workspace)) {
        t_workspace theWorkspace = m_workspace[workspace];
        if (!group.isEmpty()) theWorkspace.group = group;
        if (!app.isEmpty()) theWorkspace.app = app;

        return RPCObjName(theWorkspace, obj, withPrefix);
    } else
        return QString();
}

bool
BGMRPCConsoleDaemon::setEnv(const QString& workspace, const QString& name,
                            const QByteArray& data) {
    /*if (workspace.isEmpty()) {

    } else if (m_workspace.contains(workspace)) {
        if (data.isEmpty())
            m_workspace[workspace].env.remove(name);
        else
            m_workspace[workspace].env[name] = data;
        return true;
    } else
        return false;*/
    if (workspace.isEmpty()) {
        QSettings globalEnv;
        if (data.isEmpty())
            globalEnv.remove(name);
        else
            globalEnv.setValue(name, data);

        return true;
    } else if (m_workspace.contains(workspace)) {
        if (data.isEmpty())
            m_workspace[workspace].env.remove(name);
        else
            m_workspace[workspace].env[name] = data;

        return true;
    } else
        return false;
}

QByteArray
BGMRPCConsoleDaemon::env(const QString& workspace, const QString& name) {
    if (workspace.isEmpty()) {
        QSettings globalEnv;
        return globalEnv.value(name, "").toByteArray();
    } else if (m_workspace.contains(workspace) &&
               m_workspace[workspace].env.contains(name)) {
        return m_workspace[workspace].env[name];
    } else
        return "";
}

QVariantMap
BGMRPCConsoleDaemon::listEnv(const QString& workspace) {
    QVariantMap result;

    if (workspace.isEmpty()) {
        QSettings settings;
        foreach (const QString& key, settings.allKeys()) {
            result[key] = settings.value(key);
        }
    } else if (m_workspace.contains(workspace)) {
        QMap<QString, QByteArray> theEnv = m_workspace[workspace].env;
        foreach (const QString& key, theEnv.keys()) {
            result[key] = theEnv[key];
        }
    }

    return result;
}

QString
BGMRPCConsoleDaemon::RPCObjName(const t_workspace& workspace,
                                const QString& obj, bool withPrefix) {
    if (!withPrefix) return obj;

    QString group = workspace.group;
    QString app = workspace.app;

    QString objName;

    if (!group.isEmpty()) objName += group + "::";
    if (!app.isEmpty())
        objName += app + "::";
    else if (!app.isEmpty())
        objName += "::";

    objName += obj;

    return objName;
}

void
BGMRPCConsoleDaemon::removeWorkspace(const QString& workspace) {
    NS_BGMRPCClient::BGMRPCClient* client = m_workspace[workspace].client;
    QObject::disconnect(client, 0, 0, 0);
    client->deleteLater();
    m_workspace.remove(workspace);
    emit workspaceEnded(workspace);
}
