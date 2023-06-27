#ifndef BGMRPCCONSOLEDAEMON_H
#define BGMRPCCONSOLEDAEMON_H

#include <QDBusVariant>
#include <QObject>

#include "bgmrpcclient.h"

class BGMRPCConsoleDaemon : public QObject {
    Q_OBJECT

public:
    explicit BGMRPCConsoleDaemon(QObject* parent = nullptr);

    struct t_workspace {
        NS_BGMRPCClient::BGMRPCClient* client;
        QString group;
        QString app;
        QString api;
        QMap<QString, QByteArray> env;
    };

    Q_INVOKABLE bool existWorkspace(const QString& workspace);

    Q_INVOKABLE QString begin(const QString& workspace, const QString& url,
                              const QString& group, const QString& app,
                              const QString& api);
    Q_INVOKABLE bool end(const QString& workspace);
    Q_INVOKABLE bool reconnect(const QString& workspace);

    Q_INVOKABLE bool ping(const QString& workspace);

    Q_INVOKABLE QStringList workspaces();
    Q_INVOKABLE QString use(const QString& workspace = QString());

    Q_INVOKABLE bool setWorkspaceOpt(const QString& workspace,
                                     const QVariantMap& opt);
    Q_INVOKABLE QVariantMap workspaceOpt(const QString& workspace);

    Q_INVOKABLE bool call(const QString& workspace, const QString& obj,
                          const QString& method, const QByteArray& args,
                          bool withPrefix = true,
                          const QString& group = QString(),
                          const QString& app = QString());

    Q_INVOKABLE bool isConnected(const QString& workspace) const;

    Q_INVOKABLE QString RPCObjName(const QString& workspace, const QString& obj,
                                   bool withPrefix = true,
                                   const QString& group = QString(),
                                   const QString& app = QString());

    Q_INVOKABLE bool setEnv(const QString& workspace, const QString& name,
                            const QByteArray& data);
    Q_INVOKABLE QByteArray env(const QString& workspace, const QString& name);

    Q_INVOKABLE QVariantMap listEnv(const QString& workspace);

signals:
    void workspaceChanged(const QString& workspace);
    void workspaceEnded(const QString& workspace);
    void stateChanged(const QString& workspace, int state);
    void pong(const QString& workspace);
    void remoteSignal(const QString& workspace, const QString& obj,
                      const QString& sig, const QString& args);
    void returnData(const QString& json);
    void error(const QString& json);

public slots:

private:
    QString RPCObjName(const t_workspace& workspace, const QString& obj,
                       bool withPrefix);
    void removeWorkspace(const QString& workspace);

private:
    QMap<QString, t_workspace> m_workspace;
    QMap<QString, QByteArray> m_globalEnv;
    QString m_currentWorkspace;
};

#endif  // BGMRPCCONSOLEDAEMON_H
