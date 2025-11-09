//
// Created by siddh on 31-08-2025.
//
#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

class QLineEdit;

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    LoginDialog(QWidget *parent = nullptr);

private slots:
    void tryLogin();

private:
    void setupUI();
    void loadSavedCredentials();
    void showCreateAccount();
    
    QLineEdit *userField;
    QLineEdit *passField;
};

#endif // LOGINDIALOG_H
