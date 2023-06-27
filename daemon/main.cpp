#include <QCoreApplication>
#include <QtDBus/QDBusConnection>

#include "bgmrpcconsoledaemon.h"
#include "daemon_adaptor.h"

int
main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    QCoreApplication::setOrganizationName("BGStudio");
    QCoreApplication::setOrganizationDomain("bgstudio.org");
    QCoreApplication::setApplicationName("whisp");

    BGMRPCConsoleDaemon* daemon = new BGMRPCConsoleDaemon();

    new BGMRPCConsoleDaemonAdaptor(daemon);
    auto connection = QDBusConnection::sessionBus();
    connection.registerObject("/daemon", daemon);
    connection.registerService("BGStudio.BGMRPCConsoleDaemon");
    return a.exec();
}
