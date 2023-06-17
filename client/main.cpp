#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>

#include "client_interface.h"

#define __SUCCESSFUL_EXIT__ 0
#define __FAIL_EXIT__ 1
#define __DELAY_EXIT__ 2
#define __WAIT_EVENT__ 3
#define __FAIL_CONNECT__ 127
#define __NO_EXIST_WORKSPACE__ 128
#define __FAIL_CALL__ 129
#define __STATE_CONNECTED__ 130
#define __STATE_UNCONNECTED__ 131

#define __USAGE_TMP1__                                                      \
    "If this option is omitted, the name of the current workspace will be " \
    "used (Use 'whisp use' to view the name of the current workspace). "
#define __USAGE_TMP2__ "Can't connect to Server. "
#define __USAGE_TMP3__ "The specified workspace does not exist. "
#define __USAGE_TMP4__ \
    "generate object names based on the options of the specified workspace. "
QString
wordWrap(int leftColumn, int width, const QString& in) {
    if (in.length() + leftColumn > width) {
        QString out;
        int column = leftColumn;
        QRegularExpression re(R"RX((\S*\s*))RX");

        for (const QRegularExpressionMatch& match : re.globalMatch(in)) {
            QString word = match.captured(1);
            column += word.length();

            if (column > width) {
                out += '\n' + QString(leftColumn, ' ');
                column = leftColumn;
            }

            out += word;
        }

        return out;
    } else
        return in;
}

BGStudio::BGMRPCConsoleDaemon* daemonIF;

QMap<QString, QStringList> Usages(
    { { "whisp",
        { "Usage: whisp <subcommand> [options]",
          "whisp is a console program that sends invocation requests to a "
          "remote BGMRPC server and receives and displays the processing "
          "results." } },
      { "begin",
        { "Usage: whisp begin <workspace> <url>", "Start a new workspace.",
          QString("\nOptions\n%1\n\nExit Code\n%2")
              .arg(R"(
    <workspace>     )" +
                   wordWrap(20, 80, "The name of the new workspace.") + R"(
          <url>     )" +
                   wordWrap(
                       20, 80,
                       "The URL of server associated with the new workspace."))
              .arg(R"(
            127     )" +
                   wordWrap(20, 80, __USAGE_TMP2__)) } },
      { "end",
        { "Usage: whisp end [workspace]", "End the specified workspace.",
          QString("\nOptions\n%1\n\nExit code%2")
              .arg(R"(
    [workspace]     )" +
                   wordWrap(20, 80,
                            "Ends the specified workspace by its "
                            "name. " __USAGE_TMP1__))
              .arg(R"(
            128     )" +
                   wordWrap(20, 80, __USAGE_TMP3__)) } },
      { "reconnect",
        { "Usage: whisp reconnect [workspace]",
          "Reconnect to the server associated with the specified workspace.",
          QString("\nOptions\n%1\n\nExit Code\n%2")
              .arg(R"(
    [workspace]     )" +
                   wordWrap(
                       20, 80,
                       "Reconnects the associated server of the workspace by "
                       "its name. " __USAGE_TMP1__))
              .arg(R"(
            127     )" +
                   wordWrap(20, 80, __USAGE_TMP2__) + R"(
            128     )" +
                   wordWrap(20, 80, __USAGE_TMP3__)) } },
      { "ping",
        { "Usage: whisp ping [workspace]",
          "Send a ping signal to the server associated with the specific "
          "workspace.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(
                  R"(
    [workspace]     )" +
                  wordWrap(20, 80,
                           "Send a ping signal to the associated server of "
                           "the workspace by its name. " __USAGE_TMP1__))
              .arg(R"(
            128     )" +
                   wordWrap(20, 80, __USAGE_TMP3__)) } },
      { "isConnected",
        { "Usage: whisp isConnect [workspace]",
          "Retrieve the connection status of the server associated with the "
          "specific workspace.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(
                  R"(
    [workspace]     )" +
                  wordWrap(
                      20, 80,
                      "The name of the workspace used to check the connection "
                      "status of the specified workspace. " __USAGE_TMP1__))
              .arg(R"(
            128     )" +
                   wordWrap(20, 80, __USAGE_TMP3__)) } },
      { "workspaces", { "Usage: whisp workspaces", "List all workspaces." } },
      { "use",
        { "Usage: whisp use [workspace]",
          "Set the selected workspace as the current workspace.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(R"(
    [workspace]     )" +
                   wordWrap(20, 80,
                            "The name of the workspace to select. If this "
                            "option is omitted, the name of the currently "
                            "used workspace will be displayed. "))
              .arg(R"(
            128     )" +
                   wordWrap(20, 80, __USAGE_TMP3__)) } },
      { "RPCObjName",
        { "Usage: whisp RPCObjName <obj> [-N|--no-prefix] [-w|--workspace="
          "<workspace>]",
          "Generate a RPC object name.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(R"(
<obj>                         )" +
                   wordWrap(30, 80, "Object Name.") + R"(
   -N|--no-prefix             )" +
                   wordWrap(30, 80, "Do not use a prefix. ") + R"(
   -w|--workspace=<workspace> )" +
                   wordWrap(30, 80, __USAGE_TMP4__ __USAGE_TMP1__))
              .arg(R"(
  128                         )" +
                   wordWrap(30, 80, __USAGE_TMP3__)) } },
      { "workspace",
        { "Usage: whisp workspace [-g|--group[=group]] "
          "[-a|--app[=app]] [-w|--workspace=<workspace>]",
          "View or set options for the specified workspace.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(R"(
 -g|--group[=group]           )" +
                   wordWrap(30, 80,
                            "Set the group attribute of the workspace. If no "
                            "parameter value is specified, restore this "
                            "attribute to an empty value.") +
                   R"(
 -a|--app[=app]               )" +
                   wordWrap(30, 80,
                            "Set the app attribute of the workspace. If no "
                            "parameter value is specified, restore this "
                            "attribute to an empty value.") +
                   R"(
 -w|--workspace=<workspace>   )" +
                   wordWrap(30, 80, "The specified workspace. " __USAGE_TMP1__))
              .arg(R"(
128                           )" +
                   wordWrap(30, 80, __USAGE_TMP3__)) } },
      { "call",
        { "Usage: whisp call <obj> <method> [-N|--no-prefix] [-w|workspace] "
          "-- "
          "[arg0 arg1 ...]",
          "Invoke a remote procedure.",
          QString("\nOptions\n%1\n\nExit code\n%2") } },
      { "watch",
        { "Usage: whisp watch < --pong | --signal [--obj=<object>] "
          "[--sig=<signal>] | --state > [--workspace=<workspace>] [-c <shell "
          "cmd>]",
          "Watch signals sent by remote procedures associated with the "
          "selected workspace, or monitor pong response signals from the "
          "associated server, or monitor changes in the connection status of "
          "the associated server." } } });

int
showHelp() {
    QStringList whispHelp = Usages.take("whisp");
    qInfo().noquote() << whispHelp[0];
    qInfo().noquote() << wordWrap(0, 80, whispHelp[1]);

    qInfo().noquote() << "\nSubcommand\n";

    QMap<QString, QStringList>::const_iterator it;
    for (it = Usages.constBegin(); it != Usages.constEnd(); ++it) {
        qInfo().noquote() << QString("%1     %2")
                                 .arg(it.key(), 15)
                                 .arg(wordWrap(20, 80, it.value()[1]));
    }

    return __SUCCESSFUL_EXIT__;
}

int
showHelp(const QString& cmd) {
    QStringList help = Usages[cmd];

    qInfo().noquote() << wordWrap(0, 80, help[0]);
    qInfo().noquote() << wordWrap(0, 80, help[1]);

    if (help.size() == 3) qInfo().noquote() << help[2];

    return __SUCCESSFUL_EXIT__;
}

bool
existWorkspace(const QString& workspace) {
    if (daemonIF->existWorkspace(workspace).value())
        return true;
    else if (workspace.isEmpty()) {
        qCritical().noquote() << "Unspecified workspace.";

        return false;
    } else {
        qCritical().noquote() << "No exist workspace";

        return false;
    }
}

void
onStateChanged(const QString& workspace,
               BGStudio::BGMRPCConsoleDaemon* daemonIF) {
    QObject::connect(
        daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
        [=](const QString& aWorkspace, int state) {
            if (aWorkspace == workspace) {
                switch (state) {
                case 0:
                    qCritical().noquote() << "\t"
                                          << "Unable to establish connection.";
                    break;
                case 1:
                    qInfo().noquote() << "\t"
                                      << "Performing a host name lookup...";
                    break;
                case 2:
                    qInfo().noquote() << "\t"
                                      << "Connecting...";
                    break;
                case 3:
                    qInfo().noquote() << "\t"
                                      << "Connected.";
                    break;
                case 4:
                    qInfo().noquote() << "\t"
                                      << "Bound to address and port.";
                    break;
                case 6:
                    qInfo().noquote() << "\t"
                                      << "Closing in progress...";
                    break;
                }
            }
        });
}

int
begin(int argc, char* argv[]) {
    if (argc < 4) {
        showHelp("begin");
        return __FAIL_EXIT__;
    }

    QString theWorkspace(argv[2]);
    QString url(argv[3]);

    qInfo().noquote() << "Workspace" << theWorkspace << " - begin";

    onStateChanged(theWorkspace, daemonIF);
    QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                     [=](const QString& aWorkspace, int state) {
                         if (state == 3)
                             qApp->exit();
                         else if (state == 0)
                             qApp->exit(__FAIL_CONNECT__);
                     });

    daemonIF->begin(theWorkspace, url);

    return __WAIT_EVENT__;
}

int
end(int argc, char* argv[]) {
    QString theWorkspace(argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace" << theWorkspace << " - end";

        onStateChanged(theWorkspace, daemonIF);
        QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                         [=](const QString& aWorkspace, int state) {
                             if (state == 0) qApp->exit();
                         });

        daemonIF->end(theWorkspace);

        return __WAIT_EVENT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
reconnect(int argc, char* argv[]) {
    QString theWorkspace(argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace " << theWorkspace << " - reconnect";

        qApp->setProperty("reconnect", true);
        onStateChanged(theWorkspace, daemonIF);
        QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                         [=](const QString& aWorkspace, int state) {
                             if (state == 3)
                                 qApp->exit();
                             else if (state == 0) {
                                 qDebug()
                                     << qApp->property("reconnect").toBool();
                                 if (qApp->property("reconnect").toBool())
                                     qApp->setProperty("reconnect", QVariant());
                                 else
                                     qApp->exit(__FAIL_CONNECT__);
                             }
                         });

        daemonIF->reconnect(theWorkspace);

        return __WAIT_EVENT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
ping(int argc, char* argv[]) {
    QString theWorkspace(argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace " << theWorkspace << " - ping";
        daemonIF->ping(theWorkspace);

        return __DELAY_EXIT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
isConnected(int argc, char* argv[]) {
    QString theWorkspace(argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace" << theWorkspace << " - isConnected";
        bool isConnected = daemonIF->isConnected(theWorkspace);
        qInfo().noquote() << "\t"
                          << (isConnected ? "connected" : "disconnected");

        return isConnected ? __STATE_CONNECTED__ : __STATE_UNCONNECTED__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
workspaces() {
    qInfo().noquote() << "Workspaces";
    foreach (const QString& sc, daemonIF->workspaces().value()) {
        qInfo().noquote() << "\t - " << sc;
    }

    return __SUCCESSFUL_EXIT__;
}

int
use(int argc, char* argv[]) {
    QString theWorkspace;

    if (argc >= 3) {
        theWorkspace = daemonIF->use(argv[2]);
        if (theWorkspace.isEmpty()) {
            qCritical().noquote() << "Specified workspace does not exist.";
            return __NO_EXIST_WORKSPACE__;
        }
    } else
        theWorkspace = daemonIF->use();

    if (!theWorkspace.isEmpty())
        qInfo().noquote() << "use - " << theWorkspace;
    else
        return __NO_EXIST_WORKSPACE__;

    return __SUCCESSFUL_EXIT__;
}

int
RPCObjName(int argc, char** argv) {
    if (argc < 3 || argv[2][0] == '-') {
        showHelp("RPCObjName");
        return __FAIL_EXIT__;
    }

    QString theWorkspace(daemonIF->use());
    QString theObj(argv[2]);

    static struct option long_options[] = {
        { "no-prefix", optional_argument, 0, 'N' },
        { "workspace", required_argument, 0, 'w' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    int opt;
    bool withPrefix = true;
    while ((opt = getopt_long(argc, argv, "Nw:", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'N':
            withPrefix = false;
            break;
        case 'w':
            if (optarg[0] == '-') {
                showHelp("RPCObjName");
                return __FAIL_EXIT__;
            } else
                theWorkspace = optarg;
            break;
        default:
            showHelp("RPCObjName");
            return __FAIL_EXIT__;
        }
    }

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace" << theWorkspace;
        qInfo().noquote() << "\t"
                          << daemonIF->RPCObjName(theWorkspace, theObj,
                                                  withPrefix);

        return __SUCCESSFUL_EXIT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
workspace(int argc, char* argv[]) {
    QString theWorkspace(daemonIF->use());

    int opt;
    QString group;
    QString app;
    bool setGroup = false;
    bool setApp = false;

    static struct option long_options[] = {
        { "group", optional_argument, 0, 'g' },
        { "app", optional_argument, 0, 'a' },
        { "workspace", required_argument, 0, 'w' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    while ((opt = getopt_long(argc, argv, "g::a::w:", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'g':
            group = optarg;
            setGroup = true;
            break;
        case 'a':
            app = optarg;
            setApp = true;
            break;
        case 'w':
            if (optarg[0] == '-') {
                showHelp("workspace");
                return __FAIL_EXIT__;
            } else
                theWorkspace = optarg;
            break;
        default:
            showHelp("workspace");
            return __FAIL_EXIT__;
        }
    }

    if (existWorkspace(theWorkspace)) {
        QVariantMap theGetOpt = daemonIF->workspaceOpt(theWorkspace).value();
        QVariantMap theSetOpt;

        if (setGroup) {
            theSetOpt["group"] = group;
            theGetOpt["group"] = group;
        }
        if (setApp) {
            theSetOpt["app"] = app;
            theGetOpt["app"] = app;
        }

        if (setGroup || setApp) {
            daemonIF->setWorkspaceOpt(theWorkspace, theSetOpt);
        }

        qInfo().noquote() << "Workspace" << theWorkspace;
        qInfo().noquote() << "\t group: \"" + theGetOpt["group"].toString() +
                                 '"';
        qInfo().noquote() << "\t app: \"" + theGetOpt["app"].toString() + '"';

        return __DELAY_EXIT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
call(int argc, char* argv[]) {
    if (argv[2][0] == '-' || argv[3][0] == '-') {
        showHelp("call");
        return __FAIL_EXIT__;
    }

    QString theWorkspace(daemonIF->use());

    static struct option long_options[] = {
        { "no-prefix", no_argument, 0, 'N' },
        { "workspace", required_argument, 0, 'w' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    bool withPrefix = true;
    int opt;
    while ((opt = getopt_long(argc, argv, "Nw:", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'N':
            withPrefix = false;
            break;
        case 'w':
            if (optarg[0] == '-') {
                showHelp("call");
                return __FAIL_EXIT__;
            } else
                theWorkspace = optarg;
            break;
        default:
            showHelp("call");
            return __FAIL_EXIT__;
        }
    }

    if (existWorkspace(theWorkspace)) {
        QString theObj = argv[optind + 1];
        QString theMethod = argv[optind + 2];

        QVariantList args;
        for (int i = optind + 4; i < argc; i++) {
            QByteArray baArgData(argv[i]);
            QVariant argData = QJsonDocument::fromJson(baArgData).toVariant();
            if (!argData.isValid()) {
                bool ok = false;
                argData = baArgData.toInt(&ok);
                if (!ok) argData = baArgData.toDouble(&ok);
                if (!ok) argData = baArgData;
            }

            args.append(argData);
            // args.append(QJsonDocument::fromJson(argv[i]).toVariant());
        }

        daemonIF->call(theWorkspace, theObj, theMethod, args, withPrefix);

        QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::returnData,
                         qApp, [=](const QString& json) {
                             qInfo().noquote() << json;
                             qApp->exit();
                         });
        QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::error, qApp,
                         [=](const QString& json) {
                             qCritical().noquote() << json;
                             qApp->exit(__FAIL_CALL__);
                         });

        return __WAIT_EVENT__;
    } else
        return __NO_EXIST_WORKSPACE__;
    // QTimer::singleShot(50, qApp, [=]() { qApp->exit(); });
}

int
watch(int argc, char* argv[]) {
    if (argc < 3) {
        showHelp("watch");
        return __FAIL_EXIT__;
    }

    QString theWorkspace(daemonIF->use());

    int which = -1;
    bool whichSet = false;
    QString cmd;

    static struct option long_options[] = {
        { "workspace", required_argument, 0, 'w' },
        { "obj", required_argument, 0, 'o' },
        { "sig", required_argument, 0, 'S' },
        { "pong", no_argument, &which, 1 },
        { "signal", no_argument, &which, 2 },
        { "state", no_argument, &which, 3 },
        { 0, 0, 0, 0 }
    };

    QString theObj;
    QString theSig;
    bool ok = true;
    int opt;
    int opt_index;
    while ((opt = getopt_long(argc, argv, "w:o:S:c:", long_options,
                              &opt_index)) != -1) {
        switch (opt) {
        case 'w':
            if (optarg[0] == '-')
                ok = false;
            else
                theWorkspace = optarg;
            break;
        case 'o':
            if (optarg[0] == '-')
                ok = false;
            else
                theObj = optarg;
            break;
        case 'S':
            if (optarg[0] == '-')
                ok = false;
            else
                theSig = optarg;
            break;
        case 'c':
            if (optarg[0] == '-')
                ok = false;
            else
                cmd = optarg;
            break;
        case 0:
            ok = !whichSet;
            whichSet = true;
            break;
        default:
            showHelp("watch");
            return __FAIL_EXIT__;
        }
    }
    if (!ok) {
        showHelp("watch");
        return __FAIL_EXIT__;
    }

    if (theWorkspace.isEmpty() || !existWorkspace(theWorkspace)) {
        qCritical().noquote()
            << "Unspecified workspace or specified workspace does not exist.";
        return __FAIL_EXIT__;
    }

    if (!theWorkspace.isEmpty()) {
        auto captured = [=](const QString& output) {
            if (!output.isEmpty()) qInfo().noquote() << output;

            if (!cmd.isEmpty()) {
                QProcess* cmdProc = new QProcess(qApp);
                QProcessEnvironment env =
                    QProcessEnvironment::systemEnvironment();
                QString path = env.value("PATH");
                path += ":" + QDir::toNativeSeparators(QDir::homePath()) +
                        ".local/share/BGStudio/whisp/watch:/usr/share/BGStudio/"
                        "whisp/watch:" +
                        QCoreApplication::applicationDirPath();
#ifdef WHISPDATALOCATION
                path += ":" + QString(WHISPDATALOCATION) + "/watch";
#endif
                env.insert("PATH", path);
                cmdProc->setProcessEnvironment(env);

                cmdProc->start("/bin/sh", QStringList()
                                              << "-c" << cmd.toLatin1());
                if (!output.isEmpty()) cmdProc->write(output.toLatin1());
                cmdProc->closeWriteChannel();

                QObject::connect(cmdProc, &QProcess::finished, qApp,
                                 [=](int exitCode) { cmdProc->deleteLater(); });
                QObject::connect(
                    cmdProc, &QProcess::readyReadStandardOutput, qApp, [=]() {
                        qInfo().noquote() << cmdProc->readAllStandardOutput();
                    });
                QObject::connect(
                    cmdProc, &QProcess::readyReadStandardError, qApp, [=]() {
                        qInfo().noquote() << cmdProc->readAllStandardError();
                    });
            }
        };

        switch (which) {
        case 1:
            QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::pong,
                             qApp, [=](const QString& workspace) {
                                 if (theWorkspace == workspace) {
                                     qInfo().noquote()
                                         << "Workspace" << theWorkspace << " - "
                                         << "Pone";
                                     captured(QString());
                                 }
                             });
            break;
        case 2:
            QObject::connect(
                daemonIF, &BGStudio::BGMRPCConsoleDaemon::remoteSignal, qApp,
                [=](const QString& workspace, const QString& obj,
                    const QString& sig, const QString& json) {
                    if (theWorkspace == workspace &&
                        (theObj.isEmpty() || theObj == obj) &&
                        (theSig.isEmpty() || theSig == sig)) {
                        if (!theObj.isEmpty() && !theSig.isEmpty())
                            captured(json);
                        else {
                            QString output('{');
                            if (theObj.isEmpty())
                                output += R"("object": ")" + obj + R"(",)";
                            if (theSig.isEmpty())
                                output += R"("signal": ")" + sig + R"(",)";
                            output += R"("args": )" + json + "}";
                            captured(output);
                        }
                    }
                });
            break;
        case 3:
            QObject::connect(daemonIF,
                             &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                             [=](const QString& aWorkspace, int state) {
                                 if (theWorkspace == aWorkspace)
                                     captured(QString::number(state));
                             });
            break;
        }

        return __WAIT_EVENT__;
    } else {
        qCritical() << "Workspace must be specified.";

        return __FAIL_EXIT__;
    }
}

void
myMessageOutput(QtMsgType type, const QMessageLogContext& context,
                const QString& msg) {
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "Debug: %s\n", msg.toUtf8().constData());
        break;
    case QtInfoMsg:
        fprintf(stdout, "%s\n", msg.toUtf8().constData());
        break;
    case QtWarningMsg:
        fprintf(stdout, "%s\n", msg.toUtf8().constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s\n", msg.toUtf8().constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s\n", msg.toUtf8().constData());
        break;
    }
}

int
main(int argc, char* argv[]) {
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication a(argc, argv);

    daemonIF = new BGStudio::BGMRPCConsoleDaemon("BGStudio.BGMRPCConsoleDaemon",
                                                 "/daemon",
                                                 QDBusConnection::sessionBus());

    if (!daemonIF->isValid()) {
        qCritical().noquote() << "Can't connect to Daemon.";

        delete daemonIF;

        return 0;
    }

    int result = 0;
    if (argc >= 2 && QString::compare(argv[1], "begin") == 0)
        result = begin(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "end") == 0)
        result = end(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "reconnect") == 0)
        result = reconnect(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "ping") == 0)
        result = ping(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "isConnected") == 0)
        result = isConnected(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "workspaces") == 0)
        result = workspaces();
    else if (argc >= 2 && QString::compare(argv[1], "use") == 0)
        result = use(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "RPCObjName") == 0)
        result = RPCObjName(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "workspace") == 0)
        result = workspace(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "call") == 0)
        result = call(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "watch") == 0)
        result = watch(argc, argv);
    else if ((argc >= 2 && QString::compare(argv[1], "help") == 0) ||
             argc == 1) {
        if (argc < 3)
            result = showHelp();
        else
            result = showHelp(argv[2]);
    }

    /*switch (result) {
    case __SUCCESSFUL_EXIT__:
    case __FAIL_EXIT__:
        QTimer::singleShot(0, qApp, [=]() { qApp->exit(result); });
        break;
    case __DELAY_EXIT__:
        QTimer::singleShot(50, qApp, [=]() { qApp->exit(); });
        break;
    }*/
    if (result != __WAIT_EVENT__) {
        if (result == __DELAY_EXIT__)
            QTimer::singleShot(50, qApp, [=]() { qApp->exit(); });
        else
            QTimer::singleShot(0, qApp, [=]() { qApp->exit(result); });
    }

    return a.exec();
}
