#include "bgmrpcclientdaemon.h"

BGMRPCClientDaemon::BGMRPCClientDaemon(QObject* parent) : QObject{ parent } {}

QString
BGMRPCClientDaemon::connectToHost(const QString& scenario, const QString& url) {
}

void
BGMRPCClientDaemon::setScenarioOpt(const QVariantMap& opt) {}

void
BGMRPCClientDaemon::call(const QString& scenario, const QString& obj,
                         const QVariantList& args) {}

bool
BGMRPCClientDaemon::isConnected(const QString& scenario) const {}
