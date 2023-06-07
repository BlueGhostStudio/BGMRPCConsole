#ifndef BGMRPCClientDaemon_H
#define BGMRPCClientDaemon_H

#include <QObject>

class BGMRPCClientDaemon : public QObject {
    Q_OBJECT

public:
    explicit BGMRPCClientDaemon(QObject* parent = nullptr);

    Q_INVOKABLE QString connectToHost(const QString& scenario,
                                      const QString& url);
    Q_INVOKABLE void setScenarioOpt(const QVariantMap& opt);
    Q_INVOKABLE void call(const QString& scenario, const QString& obj,
                          const QVariantList& args);

    Q_INVOKABLE bool isConnected(const QString& scenario) const;

signals:
    void isConnectedChanged(const QString& scenario);
    void returnData(const QString& data);

public slots:

private:
};

#endif  // BGMRPCClientDaemon_H
