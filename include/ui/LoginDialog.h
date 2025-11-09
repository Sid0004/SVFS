// Canonical location for LoginDialog
#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

class QLineEdit;

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

private slots:
    void tryLogin();

private:
    void setupUI();
    void loadSavedCredentials();
    void showCreateAccount();
    
    QLineEdit *userField = nullptr;
    QLineEdit *passField = nullptr;
};

#endif // LOGINDIALOG_H