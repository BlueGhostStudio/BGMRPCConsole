#include <QCoreApplication>
#include <QtDBus/QDBusConnection>

#include "bgmrpcclientdaemon.h"
#include "daemon_adaptor.h"

int
main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    BGMRPCClientDaemon* daemon = new BGMRPCClientDaemon();

    new BGMRPCClientDaemonAdaptor(daemon);
    auto connection = QDBusConnection::sessionBus();
    connection.registerObject("/daemon", daemon);
    connection.registerService("BGStudio.BGMRPCClientDaemon");
    return a.exec();
}
