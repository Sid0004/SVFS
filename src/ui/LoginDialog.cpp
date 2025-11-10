//
// Created by siddh on 31-08-2025.
//
#include "LoginDialog.h"
#include "VFSManager.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QProgressBar>
#include <QApplication>
#include <QSettings>
#include <QTimer>
#include <QStyle>
#include <QHBoxLayout>
#include <QSpacerItem>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Login - Secure Virtual File System");
    setFixedSize(500, 480);
    setModal(true);
    setStyleSheet("QDialog { background: #1e1e1e; }");
    
    // Initialize database and VFS
    DatabaseManager::instance().initializeDatabase();
    
    setupUI();
    loadSavedCredentials();
}

void LoginDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
mainLayout->setContentsMargins(30, 15, 30, 30);

    
    // Header section
    QWidget *headerWidget = new QWidget();
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
headerLayout->setContentsMargins(0, 20, 0, 10); // adds top and bottom padding

    
    QLabel *titleLabel = new QLabel("SVFS");
    titleLabel->setAlignment(Qt::AlignCenter);
   titleLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: white; margin: 15px; letter-spacing: 4px;");

    
    QLabel *subtitleLabel = new QLabel("Secure Virtual File System");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("font-size: 13px; color: #b0b0b0; margin-bottom: 10px;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addSpacing(15); // gives breathing space below the header

    
    // Login form
    QGroupBox *loginGroup = new QGroupBox("Login");
    loginGroup->setStyleSheet("QGroupBox { font-weight: bold; color: white; border: 2px solid #444; border-radius: 5px; margin-top: 10px; padding-top: 10px; background: #2a2a2a; }");
    
    QFormLayout *formLayout = new QFormLayout(loginGroup);
    formLayout->setSpacing(30);  // Increased spacing
    formLayout->setLabelAlignment(Qt::AlignRight);
    
    userField = new QLineEdit();
    userField->setPlaceholderText("Enter username");
    userField->setMinimumHeight(35);  // Make input boxes taller
    userField->setStyleSheet("padding: 8px; border: 2px solid #555; border-radius: 5px; font-size: 14px; background: #333; color: white;");
    
    passField = new QLineEdit();
    passField->setEchoMode(QLineEdit::Password);
    passField->setPlaceholderText("Enter password");
    passField->setMinimumHeight(35);  // Make input boxes taller
    passField->setStyleSheet("padding: 8px; border: 2px solid #555; border-radius: 5px; font-size: 14px; background: #333; color: white;");
    
    formLayout->addRow("Username:", userField);
    formLayout->addRow("Password:", passField);
    
    // Options
    QWidget *optionsWidget = new QWidget();
    QHBoxLayout *optionsLayout = new QHBoxLayout(optionsWidget);
    
    QCheckBox *rememberMeCheck = new QCheckBox("Remember me");
    rememberMeCheck->setStyleSheet("color: #ccc;");
    
    QCheckBox *showPasswordCheck = new QCheckBox("Show password");
    showPasswordCheck->setStyleSheet("color: #ccc;");
    
    optionsLayout->addWidget(rememberMeCheck);
    optionsLayout->addStretch();
    optionsLayout->addWidget(showPasswordCheck);
    
    formLayout->addRow(optionsWidget);
    
    // Buttons
    QWidget *buttonWidget = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    
    QPushButton *loginBtn = new QPushButton("Login");
    loginBtn->setMinimumHeight(45);  // Make button taller
    loginBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #3498db; "
        "color: white; "
        "border: none; "
        "padding: 12px 25px; "
        "border-radius: 8px; "
        "font-weight: bold; "
        "font-size: 16px; "  // Larger font
        "} "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #21618c; } "
        "QPushButton:disabled { background-color: #bdc3c7; }"
    );
    loginBtn->setDefault(true);
    
    QPushButton *forgotPasswordBtn = new QPushButton("Forgot Password?");
    forgotPasswordBtn->setStyleSheet(
        "QPushButton { "
        "color: #3498db; "
        "border: none; "
        "background: transparent; "
        "text-decoration: underline; "
        "} "
        "QPushButton:hover { color: #2980b9; }"
    );
    
    QPushButton *createAccountBtn = new QPushButton("Create Account");
    createAccountBtn->setMinimumHeight(40);
    createAccountBtn->setStyleSheet(
        "QPushButton { "
        "color: #27ae60; "
        "border: 2px solid #27ae60; "
        "background: transparent; "
        "padding: 10px 20px; "
        "border-radius: 5px; "
        "font-size: 14px; "
        "} "
        "QPushButton:hover { background-color: #27ae60; color: white; }"
    );
    
    buttonLayout->addWidget(loginBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(forgotPasswordBtn);
    
    // Status and progress
    QLabel *statusLabel = new QLabel();
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    statusLabel->setVisible(false);
    
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { "
        "border: 1px solid #bdc3c7; "
        "border-radius: 3px; "
        "text-align: center; "
        "} "
        "QProgressBar::chunk { "
        "background-color: #3498db; "
        "border-radius: 2px; "
        "}"
    );
    
    // Assemble main layout
    mainLayout->addWidget(headerWidget);
    mainLayout->addWidget(loginGroup);
    mainLayout->addStretch();  // Push everything else downward
    mainLayout->addWidget(buttonWidget);
    mainLayout->addWidget(createAccountBtn);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addStretch();
    
    // Connect signals
    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::tryLogin);
    connect(forgotPasswordBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Forgot Password", 
            "Password recovery feature will be implemented here.\n\n"
            "TODO: Connect with User module to implement password recovery functionality.");
    });
    connect(createAccountBtn, &QPushButton::clicked, this, [this]() {
        showCreateAccount();
    });
    connect(rememberMeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            // Clear saved credentials if "Remember me" is unchecked
            QSettings settings;
            settings.remove("username");
            settings.remove("password");
        }
    });
    connect(showPasswordCheck, &QCheckBox::toggled, this, [this](bool checked) {
        passField->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    connect(userField, &QLineEdit::returnPressed, this, &LoginDialog::tryLogin);
    connect(passField, &QLineEdit::returnPressed, this, &LoginDialog::tryLogin);
}

void LoginDialog::tryLogin() {
    QString user = userField->text().trimmed();
    QString pass = passField->text();

    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "Login Failed", "Please enter both username and password!");
        return;
    }
    
    // Use real VFS authentication
    if (VFSManager::instance().authenticateUser(user, pass)) {
        // Save credentials if remember me is checked
        QCheckBox *rememberMeCheck = findChild<QCheckBox*>();
        if (rememberMeCheck && rememberMeCheck->isChecked()) {
            QSettings settings;
            settings.setValue("username", user);
            settings.setValue("password", pass);
        }
        accept();
    } else {
        QMessageBox::warning(this, "Login Failed", "Invalid credentials!");
        passField->clear();
        passField->setFocus();
    }
}

void LoginDialog::loadSavedCredentials() {
    QSettings settings;
    QString savedUser = settings.value("username", "").toString();
    QString savedPass = settings.value("password", "").toString();
    
    if (!savedUser.isEmpty()) {
        userField->setText(savedUser);
        
        // Find and check the remember me checkbox
        QCheckBox *rememberMeCheck = findChild<QCheckBox*>();
        if (rememberMeCheck) {
            rememberMeCheck->setChecked(true);
        }
        
        if (!savedPass.isEmpty()) {
            passField->setText(savedPass);
        }
    }
}

void LoginDialog::showCreateAccount() {
    QDialog createAccountDialog(this);
    createAccountDialog.setWindowTitle("Create Account - SVFS");
    createAccountDialog.setFixedSize(350, 250);
    
    QVBoxLayout *layout = new QVBoxLayout(&createAccountDialog);
    
    QFormLayout *formLayout = new QFormLayout();
    
    QLineEdit *usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Enter username");
    
    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("Enter password");
    
    QLineEdit *confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText("Confirm password");
    
    formLayout->addRow("Username:", usernameEdit);
    formLayout->addRow("Password:", passwordEdit);
    formLayout->addRow("Confirm:", confirmPasswordEdit);
    
    QPushButton *createBtn = new QPushButton("Create Account");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(createBtn);
    buttonLayout->addWidget(cancelBtn);
    
    layout->addLayout(formLayout);
    layout->addLayout(buttonLayout);
    
    connect(createBtn, &QPushButton::clicked, [&]() {
        QString username = usernameEdit->text().trimmed();
        QString password = passwordEdit->text();
        QString confirmPassword = confirmPasswordEdit->text();
        
        if (username.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(&createAccountDialog, "Error", "Please fill in all fields!");
            return;
        }
        
        if (password != confirmPassword) {
            QMessageBox::warning(&createAccountDialog, "Error", "Passwords do not match!");
            return;
        }
        
        if (password.length() < 4) {
            QMessageBox::warning(&createAccountDialog, "Error", "Password must be at least 4 characters long!");
            return;
        }
        
        if (VFSManager::instance().createUser(username, password)) {
            QMessageBox::information(&createAccountDialog, "Success", "Account created successfully!");
            createAccountDialog.accept();
        } else {
            QMessageBox::warning(&createAccountDialog, "Error", "Failed to create account. Username may already exist.");
        }
    });
    
    connect(cancelBtn, &QPushButton::clicked, &createAccountDialog, &QDialog::reject);
    
    createAccountDialog.exec();
}