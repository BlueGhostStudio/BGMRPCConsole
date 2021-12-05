#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <bgmrpcclient.h>

#include <QMainWindow>

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow, private Ui::MainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void message(const QString& m);

public slots:
    void switchConnect();
    void callMethod();

private:
    NS_BGMRPCClient::BGMRPCClient m_client;
};
#endif  // MAINWINDOW_H
