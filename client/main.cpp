#include <fcntl.h>
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
#define __WORKSPACE_END__ 132
#define __NO_SPECIFY_API__ 133

#define __USAGE_TMP1__                                                      \
    "If this option is omitted, the name of the current workspace will be " \
    "used (Use 'whisp use' to view the name of the current workspace). "
#define __USAGE_TMP2__ "Can't connect to Server. "
#define __USAGE_TMP3__ "The specified workspace does not exist. "
#define __USAGE_TMP4__ \
    "generate object names based on the options of the specified workspace. "
#define __USAGE_TMP5__                                                      \
    "Explicitly specifies the group prefix for the remote object. If this " \
    "parameter is omitted, the group attribute of the workspace will be used."
#define __USAGE_TMP6__                                                    \
    "Explicitly specifies the app prefix for the remote object. If this " \
    "parameter is omitted, the app attribute of the workspace will be used."

QString ownWorkspace;

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
          "results.",
          R"(
            132     )" +
              wordWrap(20, 80, "Worksapce has ended.") } },
      { "begin",
        { "Usage: whisp begin <workspace> <url> [-A <api>|--api=<api>] [-g "
          "<group>|--group=<group>] [-a <app>|--app=<app>] [-c|--clean]",
          "Start a new workspace.",
          QString("\nOptions\n%1\n\nExit Code\n%2")
              .arg(R"(
<workspace>                   )" +
                   wordWrap(30, 80, "The name of the new workspace.") + R"(
      <url>                   )" +
                   wordWrap(
                       30, 80,
                       "The URL of server associated with the new workspace.") +
                   R"(
 -g <group>|--group=<group>     )" +
                   wordWrap(30, 80,
                            "Set the group attribute of the workspace.") +
                   R"(
   -a <app>|--app=<app>         )" +
                   wordWrap(30, 80, "Set the app attribute of the workspace.") +
                   R"(
   -A <api>|--api=<api>         )" +
                   wordWrap(30, 80,
                            "Specify the API associated with the workspace.") +
                   R"(
         -c|--clean           )" +
                   wordWrap(30, 80,
                            "Create a workspace with a clean environment "
                            "without executing the initialization script."))
              .arg(R"(
        127                   )" +
                   wordWrap(30, 80, __USAGE_TMP2__) + R"(
        128                   )" +
                   wordWrap(30, 80, "Workspace already exist.")) } },
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
        { "Usage: whisp RPCObjName <obj> [-N|--no-prefix] [-w "
          "<workspace>|--workspace=<workspace>] [-g <group>|--group=<group>] "
          "[-a <app>|--app=<app>]",
          "Generate a RPC object name.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(R"(
         <obj>                          )" +
                   wordWrap(40, 80, "Object Name.") + R"(
            -N|--no-prefix              )" +
                   wordWrap(40, 80, "Do not use a prefix. ") + R"(
-w <workspace>|--workspace=<workspace>  )" +
                   wordWrap(40, 80, __USAGE_TMP4__ __USAGE_TMP1__) + R"(
    -g <group>|--group=<group>          )" +
                   wordWrap(40, 80, __USAGE_TMP5__) + R"(
      -a <app>|--app=<app>              )" +
                   wordWrap(40, 80, __USAGE_TMP6__))
              .arg(R"(
           128                          )" +
                   wordWrap(40, 80, __USAGE_TMP3__)) } },
      { "workspace",
        { "Usage: whisp workspace [-g <group>|--group=<group>] [-a "
          "<app>|--app=<app>] "
          "[-A <api>|--api=<api>] [-w|--workspace=<workspace>]",
          "View or set options for the specified workspace.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(R"(
-g <group>|--group=<group>              )" +
                   wordWrap(40, 80,
                            "Set the group attribute of the workspace. If no "
                            "parameter value is specified, restore this "
                            "attribute to an empty value.") +
                   R"(
  -a <app>|--app=<app>                  )" +
                   wordWrap(40, 80,
                            "Set the app attribute of the workspace. If no "
                            "parameter value is specified, restore this "
                            "attribute to an empty value.") +
                   R"(
  -A <api>|--api=<api>                  )" +
                   wordWrap(40, 80,
                            "Set the API associated with the workspace. If no "
                            "parameter value is specified, restore this "
                            "attribute to an empty value.") +
                   R"(
-w <workspace>|--workspace=<workspace>  )" +
                   wordWrap(40, 80, "The specified workspace. " __USAGE_TMP1__))
              .arg(R"(
         128                            )" +
                   wordWrap(40, 80, __USAGE_TMP3__)) } },
      { "call",
        { "Usage: whisp call <obj> <method> [-N|--no-prefix] [-w "
          "<workspace>|workspace=<workspace>] [-g <group>|--group=<group>] "
          "[-a <app>|--app=<app>] -- [arg0 arg1 ...]",
          "Invoke a remote procedure.",
          QString("\nOptions\n%1\n\nExit code\n%2")
              .arg(
                  R"(
         <obj>                          )" +
                  wordWrap(40, 80, "Remote RPC object.") +
                  R"(
      <method>                          )" +
                  wordWrap(40, 80, "Function of the remote RPC interface.") +
                  R"(
            -N|--no-prefix              )" +
                  wordWrap(40, 80, "Do not use prefix for remote objects") +
                  R"(
  -w workspace|--workspace=<workspace>  )" +
                  wordWrap(40, 80, __USAGE_TMP4__ __USAGE_TMP1__) + R"(
    -g <group>|--group=<group>          )" +
                  wordWrap(40, 80, __USAGE_TMP5__) + R"(
      -a <app>|--app=<app>              )" +
                  wordWrap(40, 80, __USAGE_TMP6__) + R"(
            --                          )" +
                  wordWrap(
                      40, 80,
                      "The following are the arguments for remote invocation."))
              .arg(R"(
           128                          )" +
                   wordWrap(40, 80, __USAGE_TMP3__) + R"(
           129                          )" +
                   wordWrap(40, 80, "Incorrect remote call. ")) } },
      { "watch",
        { "Usage: whisp watch < --pong | --signal [--obj=<object>] "
          "[--sig=<signal>] [--app=<app>] [-N|--no-prefix] | --state > [-w "
          "workspace | --workspace=<workspace>] [-c <shell cmd>]",
          "Watch signals sent by remote procedures associated with the "
          "selected workspace, or monitor pong response signals from the "
          "associated server, or monitor changes in the connection status of "
          "the associated server.",
          QString("\nOptions\n%1")
              .arg(
                  R"(
   --ping | --signal | --state          )" +
                  wordWrap(40, 80,
                           "Specify the type of watch event: --pong to "
                           "monitor the server's pong signal response, "
                           "--state to monitor the connection status with the "
                           "server, and --signal to monitor signals sent by "
                           "the remote procedure. Note that only one type of "
                           "watch event can be selected at a time.") +
                  R"(
       --signal --obj=<object>          )" +
                  wordWrap(
                      40, 80,
                      "Watch the remote procedure object that sends signals.") +
                  R"(
       --signal --sig=<signal>          )" +
                  wordWrap(
                      40, 80,
                      "Watch the signal sent by the remote procedure object.") +
                  R"(
       --signal --app=<app>             )" +
                  wordWrap(40, 80, __USAGE_TMP6__) +
                  R"(
       --signal -N|--no-perfix          )" +
                  wordWrap(40, 80, "Do not use a prefix. ") +
                  R"(
-w workspace|--workspace=<workspace>    )" +
                  wordWrap(40, 80,
                           "Specify the workspace to watch ." __USAGE_TMP1__) +
                  R"(
-c <shell cmd>                          )" +
                  wordWrap(40, 80,
                           "Use a shell command or script to handle the "
                           "monitoring.")) } },
      { "@cmd",
        { "Usage: @<cmd> [arg0 arg1 arg2 ...]",
          "Use the <cmd> parameter to specify the script to be called and "
          "provide the corresponding arguments to achieve various "
          "functionalities of remote applications. (Internally, the script "
          "invokes the API of the remote application to perform operations on "
          "the remote side.)",
          QString("\nOptions\n%1\n\nAttention:\n%2")
              .arg(
                  R"(
               <cmd>     )" +
                  wordWrap(25, 80,
                           "Name of the script (without the extension).") +
                  R"(
[arg0 arg1 arg2 ...]     )" +
                  wordWrap(25, 80, "Arguments required for the invocation."))
              .arg(R"(
                  1.     )" +
                   wordWrap(25, 80,
                            "Prior to executing this command, it is necessary "
                            "to specify the specific \"API\" associated with "
                            "the workspace, so that the command line can "
                            "locate the corresponding script.") +
                   R"(
                  2.     )" +
                   wordWrap(
                       25, 80,
                       R"(Ensure that the API scripts are available and present.
                         The script search directories are ~/.local/share/BGStudio/whisp/api/<api>, /usr/share/BGStudio/whisp/api/<api>, and INSTALL_DIR/share/api/<api>.)")) } },
      { "api::cmd",
        { "Usage: @<api>:<cmd> [arg0 arg1 arg2 ...]",
          "Use the <api> parameter to specify the API to be called and the "
          "<cmd> parameter to specify the script within the API. Provide the "
          "corresponding arguments to achieve various functionalities of the "
          "remote application. (Internally, the script invokes the API of the "
          "remote application to perform operations on the remote side.)",
          QString("\nOptions\n%1\n\nAttention:\n%2")
              .arg(
                  R"(
               <api>     )" +
                  wordWrap(25, 80, "Specify the name of the API.") +
                  R"(
               <cmd>     )" +
                  wordWrap(25, 80,
                           "Name of the script (without the extension).") +
                  R"(
[arg0 arg1 arg2 ...]     )" +
                  wordWrap(25, 80, "Arguments required for the invocation."))
              .arg(
                  R"(
                  1.     )" +
                  wordWrap(
                      25, 80,
                      R"(Ensure that the API scripts are available and present.
                         The script search directories are ~/.local/share/BGStudio/whisp/api/<api>, /usr/share/BGStudio/whisp/api/<api>, and INSTALL_DIR/share/api/<api>.)")) } },
      { "env",
        { "Usage: whisp env <name> [-v <value>|--value=<value>] [-w "
          "<workspace>|--workspace=<workspace>]",
          "Read or set environment variables for the specified workspace.",
          QString("\nOptions\n%1")
              .arg(
                  R"(
        <name>                          )" +
                  wordWrap(40, 80, "Environment variable name.") +
                  R"(
    -v <value>|--value=<value>          )" +
                  wordWrap(
                      40, 80,
                      "Set the value of the specified environment variable. "
                      "If omitted, only retrieves the variable value.") +
                  R"(
-w <workspace>|--workspace=<workspace>  )" +
                  wordWrap(40, 80,
                           "Specify the workspace. If this option is omitted, "
                           "it sets or retrieves the global variable.")) } },
      { "listEnv",
        { "Usage: whisp listEnv [workspace]", "List all environment variables.",
          QString("\nOptions\n%1")
              .arg(
                  R"(
      workspace     )" +
                  wordWrap(20, 80,
                           "Specify the workspace. If this option is omitted, "
                           "it lists the global variables.")) } } });

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

    qInfo().noquote() << "\nExit code";
    qInfo().noquote() << whispHelp[2];

    return __SUCCESSFUL_EXIT__;
}

int
showHelp(const QString& cmd) {
    if (Usages.contains(cmd)) {
        QStringList help = Usages[cmd];

        qInfo().noquote() << wordWrap(0, 80, help[0]);
        qInfo().noquote() << wordWrap(0, 80, help[1]);

        if (help.size() == 3) qInfo().noquote() << help[2];
    }

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
    QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                     [=](const QString& aWorkspace, int state) {
                         if (aWorkspace == workspace) {
                             switch (state) {
                             case 0:
                                 qCritical().noquote() << "\t"
                                                       << "disconnected.";
                                 break;
                             case 1:
                                 qInfo().noquote()
                                     << "\t"
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
                                 qInfo().noquote()
                                     << "\t"
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

void
initialProcess(QProcess* process, const QString& path) {
    // process->setInputChannelMode(QProcess::ForwardedInputChannel);
    if (!path.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString searchPath = env.value("PATH");
        searchPath += ":" + QDir::toNativeSeparators(QDir::homePath()) +
                      "/.local/share/BGStudio/whisp/" + path +
                      ":/usr/share/BGStudio/whisp/" + path + ":" +
                      QCoreApplication::applicationDirPath();
#ifdef WHISPDATALOCATION
        QString workingDirectory = QString(WHISPDATALOCATION) + "/" + path;
        searchPath +=
            ":" + workingDirectory;  // QString(WHISPDATALOCATION) + "/" + path;
        process->setWorkingDirectory(workingDirectory);
#endif

        env.insert("PATH", searchPath);
        process->setProcessEnvironment(env);
    }

    /*QObject::connect(process, &QProcess::finished, qApp,
                     [=](int exitCode) { process->deleteLater(); });*/

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp,
                     [=]() { process->terminate(); });

    QObject::connect(process, &QProcess::readyReadStandardOutput, qApp, [=]() {
        puts(process->readAllStandardOutput().constData());
        // qInfo().noquote() << process->readAllStandardOutput();
    });
    QObject::connect(process, &QProcess::readyReadStandardError, qApp, [=]() {
        puts(process->readAllStandardError().constData());
        // qInfo().noquote() << process->readAllStandardError();
    });

    /*else {
        QString cachePath(QDir::toNativeSeparators(QDir::homePath()) +
                          ".cache/whisp");
        process->setStandardOutputFile(
            cachePath, QIODeviceBase::Append | QIODeviceBase::Unbuffered);
        process->setStandardErrorFile(
            cachePath, QIODeviceBase::Append | QIODeviceBase::Unbuffered);
    }*/
}

void
outputRaw(const QString& data) {
    int devNull = open("/dev/null", O_WRONLY);
    if (devNull == -1) perror("open");

    if (dup2(3, devNull) == -1) perror("dup2");

    dprintf(3, "%s", data.toLatin1().constData());

    close(devNull);
    close(3);
}

int
begin(int argc, char* argv[]) {
    if (argc < 4 || argv[2][0] == '-' || argv[3][0] == '-') {
        showHelp("begin");
        return __FAIL_EXIT__;
    }

    ownWorkspace = argv[2];

    if (daemonIF->existWorkspace(ownWorkspace)) {
        qCritical().noquote()
            << "The workspace named" << ownWorkspace << "already exists.";
        return __NO_EXIST_WORKSPACE__;
    }
    QString url(argv[3]);

    qInfo().noquote() << "Workspace" << ownWorkspace << "- begin";

    static struct option long_options[] = {
        { "group", required_argument, 0, 'g' },
        { "app", required_argument, 0, 'a' },
        { "api", required_argument, 0, 'A' },
        { "clean", no_argument, 0, 'c' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    int opt;
    bool clean = false;
    QString group;
    QString app;
    QString api;
    bool ok = true;
    while ((opt = getopt_long(argc, argv, "g:a:A:c", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'g':
            if (optarg[0] == '-')
                ok = false;
            else
                group = optarg;
            break;
        case 'a':
            if (optarg[0] == '-')
                ok = false;
            else
                app = optarg;
            break;
        case 'A':
            if (optarg[0] == '-')
                ok = false;
            else
                api = optarg;
            break;
        case 'c':
            clean = true;
            break;
        default:
            ok = false;
        }
    }

    if (!ok) {
        showHelp("begin");
        return __FAIL_EXIT__;
    }

    onStateChanged(ownWorkspace, daemonIF);
    QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                     qApp, [=](const QString& aWorkspace, int state) {
                         if (state == 3)
                             qApp->exit();
                         else if (state == 0)
                             qApp->exit(__FAIL_CONNECT__);
                     });

    daemonIF->begin(ownWorkspace, url, group, app, api);

    if (!clean) {
        QProcess* defaultInitialProcess = new QProcess(qApp);
        initialProcess(defaultInitialProcess, "api/default");
        defaultInitialProcess->setProgram("sh");
        defaultInitialProcess->setArguments(QStringList()
                                            << "-c"
                                            << "initial.sh " + ownWorkspace);
        defaultInitialProcess->startDetached();
        // defaultInitialProcess->waitForFinished();
        if (!api.isEmpty()) {
            daemonIF->setWorkspaceOpt(ownWorkspace,
                                      QVariantMap({ { "api", api } }));
            QProcess* apiInitialProcess = new QProcess(qApp);
            initialProcess(apiInitialProcess, "api/" + api);
            apiInitialProcess->setProgram("sh");
            apiInitialProcess->setArguments(QStringList()
                                            << "-c"
                                            << "initial.sh " + ownWorkspace);
            apiInitialProcess->startDetached();
            // defaultInitialProcess->waitForFinished();
        }
    }

    return __WAIT_EVENT__;
}

int
end(int argc, char* argv[]) {
    ownWorkspace = (argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(ownWorkspace)) {
        qInfo().noquote() << "Workspace" << ownWorkspace << "- end";

        onStateChanged(ownWorkspace, daemonIF);
        /*QObject::connect(daemonIF,
           &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                         [=](const QString& aWorkspace, int state) {
                             if (state == 0) qApp->exit();
                         });*/

        daemonIF->end(ownWorkspace);

        return __WAIT_EVENT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
reconnect(int argc, char* argv[]) {
    ownWorkspace = (argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(ownWorkspace)) {
        qInfo().noquote() << "Workspace " << ownWorkspace << "- reconnect";

        qApp->setProperty("reconnect", true);
        onStateChanged(ownWorkspace, daemonIF);
        QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                         [=](const QString& aWorkspace, int state) {
                             if (state == 3)
                                 qApp->exit();
                             else if (state == 0) {
                                 if (qApp->property("reconnect").toBool())
                                     qApp->setProperty("reconnect", QVariant());
                                 else
                                     qApp->exit(__FAIL_CONNECT__);
                             }
                         });

        daemonIF->reconnect(ownWorkspace);

        return __WAIT_EVENT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
ping(int argc, char* argv[]) {
    QString theWorkspace(argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace" << theWorkspace << "- ping";
        daemonIF->ping(theWorkspace);

        return __DELAY_EXIT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

int
isConnected(int argc, char* argv[]) {
    QString theWorkspace(argc > 2 ? argv[2] : daemonIF->use().value());

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace" << theWorkspace << "- isConnected";
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

    if (!theWorkspace.isEmpty()) {
        qInfo().noquote() << "use - " << theWorkspace;
        outputRaw(theWorkspace);
    } else
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
        { "no-prefix", no_argument, 0, 'N' },
        { "workspace", required_argument, 0, 'w' },
        { "group", required_argument, 0, 'g' },
        { "app", required_argument, 0, 'a' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    bool withPrefix = true;
    QString group;
    QString app;
    int opt;
    bool ok = true;
    while ((opt = getopt_long(argc, argv, "Nw:g:a:", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'N':
            withPrefix = false;
            break;
        case 'w':
            if (optarg[0] == '-')
                ok = false;
            else
                ownWorkspace = optarg;
            break;
        case 'g':
            if (optarg[0] == '-')
                ok = false;
            else
                group = optarg;
            break;
        case 'a':
            if (optarg[0] == '-')
                ok = false;
            else
                app = optarg;
            break;
        default:
            ok = false;
            break;
        }
    }

    if (!ok) {
        showHelp("RPCObjName");
        return __FAIL_EXIT__;
    }

    if (existWorkspace(theWorkspace)) {
        qInfo().noquote() << "Workspace" << theWorkspace;
        QString objName =
            daemonIF->RPCObjName(theWorkspace, theObj, withPrefix, group, app);
        qInfo().noquote() << "\t" << objName;
        outputRaw(objName);

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
    QString api;
    bool setGroup = false;
    bool setApp = false;
    bool setApi = false;

    static struct option long_options[] = {
        { "group", required_argument, 0, 'g' },
        { "app", required_argument, 0, 'a' },
        { "api", required_argument, 0, 'A' },
        { "workspace", required_argument, 0, 'w' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    while ((opt = getopt_long(argc, argv, "g::a::A::w:", long_options,
                              nullptr)) != -1) {
        switch (opt) {
        case 'g':
            group = optarg;
            setGroup = true;
            break;
        case 'a':
            app = optarg;
            setApp = true;
            break;
        case 'A':
            api = optarg;
            setApi = true;
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
        if (setApi) {
            theSetOpt["api"] = api;
            theGetOpt["api"] = api;
        }

        if (setGroup || setApp || setApi) {
            daemonIF->setWorkspaceOpt(theWorkspace, theSetOpt);
        }

        group = theGetOpt["group"].toString();
        app = theGetOpt["app"].toString();
        api = theGetOpt["api"].toString();
        qInfo().noquote() << "Workspace" << theWorkspace;
        qInfo().noquote() << "\t group: \"" + group + '"';
        qInfo().noquote() << "\t app: \"" + app + '"';
        qInfo().noquote() << "\t api: \"" + api + '"';

        outputRaw(QString(R"({
    "group": "%1",
    "app": "%2",
    "api": "%3"
})")
                      .arg(group)
                      .arg(app)
                      .arg(api));
        return __DELAY_EXIT__;
    } else
        return __NO_EXIST_WORKSPACE__;
}

QVariant
ba2Var(const QByteArray& baData) {
    QVariant var = QJsonDocument::fromJson(baData).toVariant();
    if (!var.isValid()) {
        bool ok = false;

        if (!ok) var = baData.toInt(&ok);
        if (!ok) var = baData.toDouble(&ok);
        if (!ok) var = baData;
    }

    return var;
}

int
call(int argc, char* argv[]) {
    if (argv[2][0] == '-' || argv[3][0] == '-') {
        showHelp("call");
        return __FAIL_EXIT__;
    }

    ownWorkspace = daemonIF->use();

    static struct option long_options[] = {
        { "no-prefix", no_argument, 0, 'N' },
        { "workspace", required_argument, 0, 'w' },
        { "group", required_argument, 0, 'g' },
        { "app", required_argument, 0, 'a' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    bool withPrefix = true;
    QString group;
    QString app;
    int opt;
    bool ok = true;
    while ((opt = getopt_long(argc, argv, "Nw:g:a:", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'N':
            withPrefix = false;
            break;
        case 'w':
            if (optarg[0] == '-')
                ok = false;
            else
                ownWorkspace = optarg;
            break;
        case 'g':
            if (optarg[0] == '-')
                ok = false;
            else
                group = optarg;
            break;
        case 'a':
            if (optarg[0] == '-')
                ok = false;
            else
                app = optarg;
            break;
        default:
            showHelp("call");
            ok = false;
        }
    }

    if (!ok) {
        showHelp("call");
        return __FAIL_EXIT__;
    }

    if (existWorkspace(ownWorkspace)) {
        QString theObj = argv[optind + 1];
        QString theMethod = argv[optind + 2];

        QVariantList args;
        for (int i = optind + 3; i < argc; i++) {
            /*QByteArray baArgData(argv[i]);
            QVariant argData = QJsonDocument::fromJson(baArgData).toVariant();
            if (!argData.isValid()) {
                bool ok = false;
                argData = baArgData.toInt(&ok);
                if (!ok) argData = baArgData.toDouble(&ok);
                if (!ok) argData = baArgData;
            }*/

            args.append(ba2Var(argv[i]));
            // args.append(QJsonDocument::fromJson(argv[i]).toVariant());
        }

        daemonIF->call(ownWorkspace, theObj, theMethod,
                       QJsonDocument::fromVariant(args).toJson(), withPrefix,
                       group, app);

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

    ownWorkspace = daemonIF->use();

    int which = -1;
    bool whichSet = false;
    QString cmd;

    static struct option long_options[] = {
        { "workspace", required_argument, 0, 'w' },
        { "obj", required_argument, 0, 'o' },
        { "sig", required_argument, 0, 'S' },
        { "app", required_argument, 0, 'a' },
        { "no-prefix", no_argument, 0, 'N' },
        { "pong", no_argument, &which, 1 },
        { "signal", no_argument, &which, 2 },
        { "state", no_argument, &which, 3 },
        { "WSPChanged", no_argument, &which, 4 },
        { 0, 0, 0, 0 }
    };

    bool withPrefix = true;
    QString theObj;
    QString app;
    QString theSig;
    bool ok = true;
    int opt;
    int opt_index;
    while ((opt = getopt_long(argc, argv, "w:o:S:c:", long_options,
                              &opt_index)) != -1) {
        switch (opt) {
        case 'N':
            withPrefix = false;
            break;
        case 'a':
            if (optarg[0] == '-')
                ok = false;
            else
                app = optarg;
            break;
        case 'w':
            if (optarg[0] == '-')
                ok = false;
            else
                ownWorkspace = optarg;
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

    if (which != 4 && ownWorkspace.isEmpty()) {
        qCritical().noquote()
            << "Unspecified workspace or specified workspace does not exist.";
        return __FAIL_EXIT__;
    } else {
        auto captured = [=](const QString& output) {
            // if (!output.isEmpty()) qInfo().noquote() << output;

            if (!cmd.isEmpty()) {
                QProcess* cmdProc = new QProcess(qApp);
                /*QProcessEnvironment env =
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
                cmdProc->setProcessEnvironment(env);*/
                initialProcess(cmdProc, QString());

                cmdProc->start("/bin/sh",
                               QStringList()
                                   << "-c"
                                   << cmd.toLatin1() + " " + ownWorkspace);
                if (!output.isEmpty()) cmdProc->write(output.toLatin1());
                cmdProc->closeWriteChannel();

                /*QObject::connect(cmdProc, &QProcess::finished, qApp,
                                 [=](int exitCode) { cmdProc->deleteLater(); });
                QObject::connect(
                    cmdProc, &QProcess::readyReadStandardOutput, qApp, [=]() {
                        qInfo().noquote() << cmdProc->readAllStandardOutput();
                    });
                QObject::connect(
                    cmdProc, &QProcess::readyReadStandardError, qApp, [=]() {
                        qInfo().noquote() << cmdProc->readAllStandardError();
                    });*/
            }
        };

        switch (which) {
        case 1:
            QObject::connect(daemonIF, &BGStudio::BGMRPCConsoleDaemon::pong,
                             qApp, [=](const QString& workspace) {
                                 if (ownWorkspace == workspace) {
                                     qInfo().noquote()
                                         << "Workspace" << ownWorkspace << "-"
                                         << "Pone";
                                     captured(QString());
                                 }
                             });
            break;
        case 2:
            if (!theObj.isEmpty())
                theObj = daemonIF->RPCObjName(ownWorkspace, theObj, withPrefix,
                                              QString(), app);
            QObject::connect(
                daemonIF, &BGStudio::BGMRPCConsoleDaemon::remoteSignal, qApp,
                [=](const QString& workspace, const QString& obj,
                    const QString& sig, const QString& json) {
                    if (ownWorkspace == workspace &&
                        (theObj.isEmpty() || theObj == obj) &&
                        (theSig.isEmpty() || theSig == sig)) {
                        if (!theObj.isEmpty() && !theSig.isEmpty()) {
                            if (!cmd.isEmpty())
                                captured(json);
                            else
                                dprintf(1, "%s\n", json.toUtf8().constData());
                            // qInfo().noquote() << json << "\n\n";
                        } else {
                            QString output('{');
                            if (theObj.isEmpty())
                                output += R"("object": ")" + obj + R"(",)";
                            if (theSig.isEmpty())
                                output += R"("signal": ")" + sig + R"(",)";
                            output += R"("args": )" + json + "}";
                            if (!cmd.isEmpty())
                                captured(output);
                            else
                                dprintf(1, "%s\n", output.toUtf8().constData());
                            // qInfo().noquote() << output << "\n\n";
                        }
                    }
                });
            break;
        case 3:
            QObject::connect(daemonIF,
                             &BGStudio::BGMRPCConsoleDaemon::stateChanged,
                             [=](const QString& aWorkspace, int state) {
                                 if (ownWorkspace == aWorkspace)
                                     captured(QString::number(state));
                             });
            break;
        case 4:
            QObject::connect(
                daemonIF, &BGStudio::BGMRPCConsoleDaemon::workspaceChanged,
                qApp, [=](const QString& workspace) {
                    dprintf(1, "%s\n", workspace.toLatin1().constData());
                });
            break;
        }

        return __WAIT_EVENT__;
    }
}

int
env(int argc, char* argv[]) {
    if (argc < 3 || argv[2][0] == '-') {
        showHelp("env");

        return __FAIL_EXIT__;
    }

    QString name(argv[2]);
    QString theWorkspace;

    static struct option long_options[] = {
        { "value", required_argument, 0, 'v' },
        { "workspace", required_argument, 0, 'w' },
        { 0, 0, 0, 0 }  // 结束标志
    };

    int opt;
    QByteArray baValue;
    bool setValue = false;
    bool ok = true;
    while ((opt = getopt_long(argc, argv, "n:v:w:", long_options, nullptr)) !=
           -1) {
        switch (opt) {
        case 'v':
            if (optarg[0] == '-') ok = false;
            baValue = optarg;
            setValue = true;
            break;
        case 'w':
            if (optarg[0] == '-')
                ok = false;
            else
                theWorkspace = optarg;
            break;
        default:
            ok = false;
        }
    }
    if (!ok) {
        showHelp("env");
        return __FAIL_EXIT__;
    }

    if (!name.isEmpty()) {
        if (setValue)
            daemonIF->setEnv(theWorkspace, name, baValue);
        else
            baValue = daemonIF->env(theWorkspace, name).value();

        QJsonDocument json = QJsonDocument::fromJson(baValue);
        if (!json.isNull()) baValue = json.toJson();

        qInfo().noquote() << baValue;

        return __DELAY_EXIT__;
    } else
        return __FAIL_EXIT__;
}

int
listEnv(int argc, char* argv[]) {
    QVariantMap theEnvs = daemonIF->listEnv(argc == 2 ? "" : argv[2]);

    if (argc > 2 && !existWorkspace(argv[2])) return __NO_EXIST_WORKSPACE__;

    foreach (const QString& name, theEnvs.keys()) {
        qInfo().noquote() << name << "=" << theEnvs[name].toString();
    }

    return __SUCCESSFUL_EXIT__;
}

int
apiCall(const QByteArray& theWorkspace, const QByteArray& api, QString method,
        int argc, char* argv[]) {
    if (api.isEmpty()) {
        qCritical().noquote() << "Do not specified for API";

        return __NO_SPECIFY_API__;
    }

    if (!method.contains(QRegularExpression(".sh$"))) method.append(".sh");
    if (method.isEmpty()) return __FAIL_EXIT__;

    /* QByteArray theWorkspace = daemonIF->use().value().toLatin1();
    if (theWorkspace.isEmpty()) {
        qCritical().noquote() << "No workspace currently in use";
        return __NO_EXIST_WORKSPACE__;
    }*/

    QByteArray cmd = method.toLatin1() + ' ' + theWorkspace;
    for (int i = 2; i < argc; i++) {
        QByteArray arg(argv[i]);
        arg.replace('\"', "\\\"");

        if (arg.contains(" ")) arg.prepend('"').append('"');

        cmd.append(' ').append(arg);
    }

    QByteArray apiPath = "api/" + api;
    QByteArray searchPath(
        QDir::toNativeSeparators(QDir::homePath()).toLatin1() +
        "/.local/share/BGStudio/whisp/" + apiPath +
        ":/usr/share/BGStudio/whisp/" + apiPath + ":" +
        QCoreApplication::applicationDirPath().toLatin1());
#ifdef WHISPDATALOCATION
    QByteArray workingDirectory = QByteArray(WHISPDATALOCATION) + "/" + apiPath;
    searchPath += ":" + workingDirectory + ":" + getenv("PATH");
    //chdir(workingDirectory.constData());
#endif
    setenv("PATH", searchPath.constData(), 1);
    system(cmd.constData());

    return __SUCCESSFUL_EXIT__;
}

int
apiCall(int argc, char* argv[]) {
    QByteArray theWorkspace = daemonIF->use().value().toLatin1();
    if (theWorkspace.isEmpty()) {
        qCritical().noquote() << "No workspace currently in use";
        return __NO_EXIST_WORKSPACE__;
    }

    QString apiArg(argv[1]);

    return apiCall(theWorkspace, apiArg.section("::", 0, 0).toLatin1(),
                   apiArg.section("::", 1, 1).toLatin1(), argc, argv);
}

int
defaultApiCall(int argc, char* argv[]) {
    QByteArray theWorkspace = daemonIF->use().value().toLatin1();
    if (theWorkspace.isEmpty()) {
        qCritical().noquote() << "No workspace currently in use";
        return __NO_EXIST_WORKSPACE__;
    }

    QByteArray api =
        daemonIF->workspaceOpt(theWorkspace).value()["api"].toByteArray();

    return apiCall(theWorkspace, api, QByteArray(argv[1]).mid(1), argc, argv);
    /*QString theWorkspace = daemonIF->use();
    if (theWorkspace.isEmpty()) {
        qCritical().noquote() << "No workspace currently in use";
        return __NO_EXIST_WORKSPACE__;
    }

    QString api =
        daemonIF->workspaceOpt(theWorkspace).value()["api"].toString();
    if (api.isEmpty()) {
        qCritical().noquote() << "Workspace not specified for API";

        return __NO_SPECIFY_API__;
    }

    QString apiMethodName = QString(argv[1]).mid(1);
    if (!apiMethodName.contains(QRegularExpression(".sh$")))
        apiMethodName += ".sh";

    if (!apiMethodName.isEmpty()) {
        QString cmd = apiMethodName + ' ' + theWorkspace;
        for (int i = 2; i < argc; i++) {
            QString arg(argv[i]);
            arg.replace('\"', "\\\"");

            if (arg.contains(" ")) arg.prepend('"').append('"');

            cmd.append(' ').append(arg);
        }

        QString apiPath = "api/" + api;
        QString searchPath(QDir::toNativeSeparators(QDir::homePath()) +
                           "/.local/share/BGStudio/whisp/" + apiPath +
                           ":/usr/share/BGStudio/whisp/" + apiPath + ":" +
                           QCoreApplication::applicationDirPath());
#ifdef WHISPDATALOCATION
        QString workingDirectory = QString(WHISPDATALOCATION) + "/" + apiPath;
        searchPath += ":" + workingDirectory + ":" + getenv("PATH");
        chdir(workingDirectory.toLatin1().constData());
#endif
        setenv("PATH", searchPath.toLatin1().constData(), 1);
        system(cmd.toLatin1().constData());
    }

    return __SUCCESSFUL_EXIT__;*/
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

    /*if (!daemonIF->isValid()) {
        qCritical().noquote() << "Can't connect to Daemon.";

        delete daemonIF;

        return 0;
    }*/

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
    else if (argc >= 2 && QString::compare(argv[1], "env") == 0)
        result = env(argc, argv);
    else if (argc >= 2 && QString::compare(argv[1], "listEnv") == 0)
        result = listEnv(argc, argv);
    else if (argc >= 2 && QString(argv[1]).contains("::"))
        result = apiCall(argc, argv);
    else if (argc >= 2 && argv[1][0] == '@')
        result = defaultApiCall(argc, argv);
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
    if (result == __WAIT_EVENT__) {
        QObject::connect(daemonIF,
                         &BGStudio::BGMRPCConsoleDaemon::workspaceEnded, qApp,
                         [=](const QString& workspace) {
                             if (workspace == ownWorkspace)
                                 qApp->exit(__WORKSPACE_END__);
                         });
    } else {
        if (result == __DELAY_EXIT__)
            QTimer::singleShot(50, qApp, [=]() { qApp->exit(); });
        else
            QTimer::singleShot(0, qApp, [=]() { qApp->exit(result); });
    }

    return a.exec();
}
