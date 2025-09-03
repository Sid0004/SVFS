//
// Created by siddh on 31-08-2025.
//
#include "MainWindow.h"
#include "LoginDialog.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        MainWindow w;
        w.show();
        return app.exec();
    }

    return 0;
}
