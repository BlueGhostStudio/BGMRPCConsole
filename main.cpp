#include <QApplication>
#include <QtDebug>

#include "mainwindow.h"

MainWindow* w;

int
main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    w = new MainWindow;
    w->show();

    qInstallMessageHandler([](QtMsgType type,
                              const QMessageLogContext& /*context*/,
                              const QString& msg) {
        switch (type) {
        case QtMsgType::QtDebugMsg:
            w->message(msg);
            break;
        case QtMsgType::QtInfoMsg:
            w->message(msg);
            break;
        case QtMsgType::QtWarningMsg:
            w->message(msg);
            break;
        case QtMsgType::QtCriticalMsg:
            w->message(msg);
            break;
        case QtMsgType::QtFatalMsg:
            w->message(msg);
            break;
        }
    });
    return a.exec();
}
