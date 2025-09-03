//
// Created by siddh on 31-08-2025.
//
#include "MainWindow.h"
#include "VFSManager.h"
#include "DatabaseManager.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QSplitter>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>
#include <QTabWidget>
#include <QListWidget>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QPixmap>
#include <QTextDocument>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QStyle>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Secure Virtual File System - SVFS");
    resize(1200, 800);
    setMinimumSize(800, 600);
    
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    loadFileTree();
}

void MainWindow::setupUI() {
    // Main splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Left side splitter
    QSplitter *leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    
    // Search bar
    QWidget *searchWidget = new QWidget(leftSplitter);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->setContentsMargins(5, 5, 5, 5);
    
    QLineEdit *searchBar = new QLineEdit(searchWidget);
    searchBar->setPlaceholderText("Search files...");
    searchBar->setClearButtonEnabled(true);
    
    QComboBox *viewModeCombo = new QComboBox(searchWidget);
    viewModeCombo->addItems({"Tree View", "List View"});
    viewModeCombo->setMaximumWidth(100);
    
    searchLayout->addWidget(searchBar);
    searchLayout->addWidget(viewModeCombo);
    
    leftSplitter->addWidget(searchWidget);
    
    // File tree
    fileTree = new QTreeWidget(leftSplitter);
    fileTree->setHeaderLabels({"Name", "Size", "Modified", "Type"});
    fileTree->setAlternatingRowColors(true);
    fileTree->setSortingEnabled(true);
    fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    // Set column widths
    fileTree->setColumnWidth(0, 200);
    fileTree->setColumnWidth(1, 80);
    fileTree->setColumnWidth(2, 120);
    fileTree->setColumnWidth(3, 100);
    
    leftSplitter->addWidget(fileTree);
    
    // File preview with tabs
    QTabWidget *filePreviewTabs = new QTabWidget(mainSplitter);
    filePreviewTabs->setTabPosition(QTabWidget::South);
    
    // Text preview tab
    filePreview = new QTextEdit(filePreviewTabs);
    filePreview->setReadOnly(true);
    filePreview->setFont(QFont("Consolas", 10));
    filePreviewTabs->addTab(filePreview, "Text Preview");
    
    // Image preview tab
    QLabel *imagePreview = new QLabel(filePreviewTabs);
    imagePreview->setAlignment(Qt::AlignCenter);
    imagePreview->setStyleSheet("border: 1px solid #ccc; background: white;");
    imagePreview->setText("No image selected");
    filePreviewTabs->addTab(imagePreview, "Image Preview");
    
    // Properties tab
    QWidget *propertiesWidget = new QWidget();
    QVBoxLayout *propertiesLayout = new QVBoxLayout(propertiesWidget);
    
    QLabel *propertiesLabel = new QLabel("File Properties");
    propertiesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    propertiesLayout->addWidget(propertiesLabel);
    
    QLabel *propertiesText = new QLabel("Select a file to view properties");
    propertiesText->setWordWrap(true);
    propertiesLayout->addWidget(propertiesText);
    propertiesLayout->addStretch();
    
    filePreviewTabs->addTab(propertiesWidget, "Properties");
    
    mainSplitter->addWidget(filePreviewTabs);
    
    // Set splitter proportions
    mainSplitter->setSizes({300, 900});
    leftSplitter->setSizes({50, 750});
    
    // Connect signals
    connect(fileTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        updateActions();
    });
    connect(fileTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column)
        if (item) {
            openFile();
        }
    });
    connect(fileTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        showContextMenu(pos);
    });
    connect(searchBar, &QLineEdit::textChanged, this, [this, searchBar]() {
        searchFiles(searchBar->text());
    });
    connect(viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int index) {
                // TODO: Implement view mode switching
                Q_UNUSED(index)
            });
}

void MainWindow::setupMenuBar() {
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    QAction *newFileAction = new QAction("&New File", this);
    newFileAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    newFileAction->setShortcut(QKeySequence::New);
    newFileAction->setStatusTip("Create a new file");
    
    QAction *newFolderAction = new QAction("New &Folder", this);
    newFolderAction->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    newFolderAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    newFolderAction->setStatusTip("Create a new folder");
    
    QAction *importFileAction = new QAction("&Import File", this);
    importFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    importFileAction->setShortcut(QKeySequence("Ctrl+I"));
    importFileAction->setStatusTip("Import file from local system");
    
    QAction *openAction = new QAction("&Open", this);
    openAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip("Open selected file");
    
    QAction *deleteAction = new QAction("&Delete", this);
    deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setStatusTip("Delete selected items");
    
    QAction *renameAction = new QAction("&Rename", this);
    renameAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    renameAction->setShortcut(QKeySequence("F2"));
    renameAction->setStatusTip("Rename selected item");
    
    QAction *refreshAction = new QAction("&Refresh", this);
    refreshAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refreshAction->setShortcut(QKeySequence::Refresh);
    refreshAction->setStatusTip("Refresh file tree");
    
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Exit application");
    
    fileMenu->addAction(newFileAction);
    fileMenu->addAction(newFolderAction);
    fileMenu->addAction(importFileAction);
    fileMenu->addSeparator();
    fileMenu->addAction(openAction);
    fileMenu->addSeparator();
    fileMenu->addAction(deleteAction);
    fileMenu->addAction(renameAction);
    fileMenu->addSeparator();
    fileMenu->addAction(refreshAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    
    // Edit menu
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    
    QAction *copyAction = new QAction("&Copy", this);
    copyAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setStatusTip("Copy selected items");
    
    QAction *pasteAction = new QAction("&Paste", this);
    pasteAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setStatusTip("Paste items from clipboard");
    
    QAction *cutAction = new QAction("Cu&t", this);
    cutAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    cutAction->setShortcut(QKeySequence::Cut);
    cutAction->setStatusTip("Cut selected items");
    
    editMenu->addAction(copyAction);
    editMenu->addAction(cutAction);
    editMenu->addAction(pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(deleteAction);
    editMenu->addAction(renameAction);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(refreshAction);
    
    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    
    QAction *propertiesAction = new QAction("&Properties", this);
    propertiesAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    propertiesAction->setShortcut(QKeySequence("Alt+Return"));
    propertiesAction->setStatusTip("Show file properties");
    
    QAction *settingsAction = new QAction("&Settings", this);
    settingsAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsAction->setStatusTip("Open settings");
    
    toolsMenu->addAction(propertiesAction);
    toolsMenu->addAction(settingsAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    QAction *aboutAction = new QAction("&About", this);
    aboutAction->setStatusTip("About SVFS");
    
    helpMenu->addAction(aboutAction);
    
    // Connect actions
    connect(newFileAction, &QAction::triggered, this, [this]() { createNewFile(); });
    connect(newFolderAction, &QAction::triggered, this, [this]() { createNewFolder(); });
    connect(importFileAction, &QAction::triggered, this, [this]() { importFile(); });
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(deleteAction, &QAction::triggered, this, [this]() { deleteSelected(); });
    connect(renameAction, &QAction::triggered, this, [this]() { renameSelected(); });
    connect(copyAction, &QAction::triggered, this, [this]() { copyFile(); });
    connect(pasteAction, &QAction::triggered, this, [this]() { pasteFile(); });
    connect(cutAction, &QAction::triggered, this, [this]() { cutFile(); });
    connect(refreshAction, &QAction::triggered, this, [this]() { refreshFileTree(); });
    connect(propertiesAction, &QAction::triggered, this, [this]() { showFileProperties(); });
    connect(settingsAction, &QAction::triggered, this, [this]() { showSettings(); });
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    connect(aboutAction, &QAction::triggered, this, [this]() { showAbout(); });
}

void MainWindow::setupToolBar() {
    QToolBar *mainToolBar = addToolBar("Main Toolbar");
    mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    // Add actions to toolbar
    QAction *newFileAction = new QAction("New File", this);
    newFileAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    
    QAction *newFolderAction = new QAction("New Folder", this);
    newFolderAction->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    
    QAction *importFileAction = new QAction("Import", this);
    importFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    
    QAction *openAction = new QAction("Open", this);
    openAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    
    QAction *deleteAction = new QAction("Delete", this);
    deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    
    QAction *refreshAction = new QAction("Refresh", this);
    refreshAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    
    mainToolBar->addAction(newFileAction);
    mainToolBar->addAction(newFolderAction);
    mainToolBar->addAction(importFileAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(openAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(deleteAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(refreshAction);
    
    // Connect toolbar actions
    connect(newFileAction, &QAction::triggered, this, [this]() { createNewFile(); });
    connect(newFolderAction, &QAction::triggered, this, [this]() { createNewFolder(); });
    connect(importFileAction, &QAction::triggered, this, [this]() { importFile(); });
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(deleteAction, &QAction::triggered, this, [this]() { deleteSelected(); });
    connect(refreshAction, &QAction::triggered, this, [this]() { refreshFileTree(); });
}

void MainWindow::setupStatusBar() {
    QLabel *statusLabel = new QLabel("Ready");
    statusBar()->addWidget(statusLabel);
    
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setVisible(false);
    statusBar()->addPermanentWidget(progressBar);
}

void MainWindow::loadFileTree() {
    fileTree->clear();
    
    // Load directories from VFS
    QList<DirectoryRecord> directories = VFSManager::instance().getDirectoriesInPath("/");
    for (const auto &dir : directories) {
        QTreeWidgetItem *dirItem = new QTreeWidgetItem(fileTree, 
            QStringList() << dir.name << "Folder" << dir.createdAt.toString("yyyy-MM-dd") << "Directory");
        dirItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        dirItem->setData(0, Qt::UserRole, dir.id);
        dirItem->setData(0, Qt::UserRole + 1, "directory");
    }
    
    // Load files from VFS
    QList<FileRecord> files = VFSManager::instance().getFilesInDirectory("/");
    for (const auto &file : files) {
        QString sizeStr = formatFileSize(file.size);
        QString typeStr = file.isEncrypted ? "Encrypted" : (file.isCompressed ? "Compressed" : "Normal");
        
        QTreeWidgetItem *fileItem = new QTreeWidgetItem(fileTree, 
            QStringList() << file.filename << sizeStr << file.createdAt.toString("yyyy-MM-dd") << typeStr);
        fileItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        fileItem->setData(0, Qt::UserRole, file.id);
        fileItem->setData(0, Qt::UserRole + 1, "file");
    }
    
    fileTree->expandAll();
}

QString MainWindow::formatFileSize(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
}

void MainWindow::updateActions() {
    // This would update action states based on selection
    // For now, just a placeholder
}

void MainWindow::openFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to open.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file to open.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    statusBar()->showMessage(QString("Opening %1...").arg(fileName), 2000);
    
    // Get file content from VFS
    QByteArray content;
    if (VFSManager::instance().getFileContent(fileId, content)) {
        if (fileName.endsWith(".txt") || fileName.endsWith(".md") || fileName.endsWith(".cpp") || fileName.endsWith(".h")) {
            filePreview->setText(QString::fromUtf8(content));
        } else {
            filePreview->setText(QString("File: %1\nSize: %2 bytes\n\nBinary file - content not displayed").arg(fileName).arg(content.size()));
        }
    } else {
        filePreview->setText(QString("Error: Could not read file %1").arg(fileName));
    }
}

void MainWindow::createNewFile() {
    bool ok;
    QString fileName = QInputDialog::getText(this, "New File", "Enter file name:", 
                                           QLineEdit::Normal, "newfile.txt", &ok);
    if (ok && !fileName.isEmpty()) {
        // Create empty file in VFS
        QByteArray content = "";
        if (VFSManager::instance().createFile(fileName, "/", content)) {
            statusBar()->showMessage(QString("Created file: %1").arg(fileName), 2000);
            refreshFileTree();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create file.");
        }
    }
}

void MainWindow::createNewFolder() {
    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder", "Enter folder name:", 
                                             QLineEdit::Normal, "New Folder", &ok);
    if (ok && !folderName.isEmpty()) {
        if (VFSManager::instance().createDirectory(folderName, "/")) {
            statusBar()->showMessage(QString("Created folder: %1").arg(folderName), 2000);
            refreshFileTree();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create folder.");
        }
    }
}

void MainWindow::importFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import File", "", "All Files (*.*)");
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString vfsFileName = fileInfo.fileName();
        
        // Ask for encryption and compression options
        QDialog optionsDialog(this);
        optionsDialog.setWindowTitle("Import Options");
        optionsDialog.setFixedSize(300, 150);
        
        QVBoxLayout *layout = new QVBoxLayout(&optionsDialog);
        
        QCheckBox *encryptCheck = new QCheckBox("Encrypt file");
        QCheckBox *compressCheck = new QCheckBox("Compress file");
        
        QPushButton *okButton = new QPushButton("Import");
        QPushButton *cancelButton = new QPushButton("Cancel");
        
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        
        layout->addWidget(encryptCheck);
        layout->addWidget(compressCheck);
        layout->addLayout(buttonLayout);
        
        connect(okButton, &QPushButton::clicked, &optionsDialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, &optionsDialog, &QDialog::reject);
        
        if (optionsDialog.exec() == QDialog::Accepted) {
            if (VFSManager::instance().importFile(fileName, "/", encryptCheck->isChecked(), compressCheck->isChecked())) {
                statusBar()->showMessage(QString("Imported file: %1").arg(vfsFileName), 2000);
                refreshFileTree();
            } else {
                QMessageBox::warning(this, "Error", "Failed to import file.");
            }
        }
    }
}

void MainWindow::deleteSelected() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select items to delete.");
        return;
    }
    
    QString itemNames;
    for (auto item : selected) {
        if (!itemNames.isEmpty()) itemNames += ", ";
        itemNames += item->text(0);
    }
    
    int ret = QMessageBox::question(this, "Delete Items", 
                                   QString("Are you sure you want to delete:\n%1").arg(itemNames),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        for (auto item : selected) {
            QString itemType = item->data(0, Qt::UserRole + 1).toString();
            int itemId = item->data(0, Qt::UserRole).toInt();
            
            if (itemType == "file") {
                VFSManager::instance().deleteFile(itemId);
            } else if (itemType == "directory") {
                VFSManager::instance().deleteDirectory(itemId);
            }
        }
        statusBar()->showMessage(QString("Deleted %1 item(s)").arg(selected.size()), 2000);
        refreshFileTree();
    }
}

void MainWindow::renameSelected() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select an item to rename.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString oldName = item->text(0);
    
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename", "Enter new name:", 
                                          QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        // TODO: Implement rename functionality in VFS
        item->setText(0, newName);
        statusBar()->showMessage(QString("Renamed %1 to %2").arg(oldName, newName), 2000);
    }
}

void MainWindow::refreshFileTree() {
    statusBar()->showMessage("Refreshing file tree...", 1000);
    loadFileTree();
    statusBar()->showMessage("File tree refreshed", 2000);
}

void MainWindow::searchFiles(const QString &searchText) {
    if (searchText.isEmpty()) {
        loadFileTree();
        return;
    }
    
    fileTree->clear();
    
    // Search files in VFS
    QList<FileRecord> files = VFSManager::instance().searchFiles(searchText);
    for (const auto &file : files) {
        QString sizeStr = formatFileSize(file.size);
        QString typeStr = file.isEncrypted ? "Encrypted" : (file.isCompressed ? "Compressed" : "Normal");
        
        QTreeWidgetItem *fileItem = new QTreeWidgetItem(fileTree, 
            QStringList() << file.filename << sizeStr << file.createdAt.toString("yyyy-MM-dd") << typeStr);
        fileItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        fileItem->setData(0, Qt::UserRole, file.id);
        fileItem->setData(0, Qt::UserRole + 1, "file");
    }
}

void MainWindow::showFileProperties() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to view properties.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    int itemId = item->data(0, Qt::UserRole).toInt();
    
    if (itemType == "file") {
        FileRecord file;
        if (DatabaseManager::instance().getFile(itemId, file)) {
            QString properties = QString(
                "Name: %1\n"
                "Path: %2\n"
                "Size: %3\n"
                "Type: %4\n"
                "Created: %5\n"
                "Modified: %6\n"
                "Encrypted: %7\n"
                "Compressed: %8\n"
                "User ID: %9"
            ).arg(file.filename, file.path, formatFileSize(file.size), file.mimeType,
                  file.createdAt.toString(), file.modifiedAt.toString(),
                  file.isEncrypted ? "Yes" : "No", file.isCompressed ? "Yes" : "No",
                  QString::number(file.userId));
            
            QMessageBox::information(this, "File Properties", properties);
        }
    } else {
        QMessageBox::information(this, "Properties", "Directory properties not implemented yet.");
    }
}

void MainWindow::showSettings() {
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle("Settings - SVFS");
    settingsDialog.setFixedSize(500, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&settingsDialog);
    
    QTabWidget *tabWidget = new QTabWidget();
    
    // General tab
    QWidget *generalTab = new QWidget();
    QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
    
    QGroupBox *appearanceGroup = new QGroupBox("Appearance");
    QFormLayout *appearanceLayout = new QFormLayout(appearanceGroup);
    
    QComboBox *themeCombo = new QComboBox();
    themeCombo->addItems({"System", "Light", "Dark", "High Contrast"});
    appearanceLayout->addRow("Theme:", themeCombo);
    
    QGroupBox *behaviorGroup = new QGroupBox("Behavior");
    QFormLayout *behaviorLayout = new QFormLayout(behaviorGroup);
    
    QCheckBox *autoLoginCheck = new QCheckBox("Auto-login with saved credentials");
    QCheckBox *minimizeToTrayCheck = new QCheckBox("Minimize to system tray");
    QSpinBox *sessionTimeoutSpin = new QSpinBox();
    sessionTimeoutSpin->setRange(5, 480);
    sessionTimeoutSpin->setSuffix(" minutes");
    sessionTimeoutSpin->setValue(30);
    
    behaviorLayout->addRow(autoLoginCheck);
    behaviorLayout->addRow(minimizeToTrayCheck);
    behaviorLayout->addRow("Session timeout:", sessionTimeoutSpin);
    
    generalLayout->addWidget(appearanceGroup);
    generalLayout->addWidget(behaviorGroup);
    generalLayout->addStretch();
    
    // Security tab
    QWidget *securityTab = new QWidget();
    QVBoxLayout *securityLayout = new QVBoxLayout(securityTab);
    
    QGroupBox *encryptionGroup = new QGroupBox("Encryption");
    QFormLayout *encryptionLayout = new QFormLayout(encryptionGroup);
    
    QCheckBox *autoEncryptCheck = new QCheckBox("Auto-encrypt new files");
    QComboBox *encryptionAlgoCombo = new QComboBox();
    encryptionAlgoCombo->addItems({"AES-256-CBC", "AES-256-GCM", "ChaCha20-Poly1305"});
    
    encryptionLayout->addRow(autoEncryptCheck);
    encryptionLayout->addRow("Algorithm:", encryptionAlgoCombo);
    
    QGroupBox *compressionGroup = new QGroupBox("Compression");
    QFormLayout *compressionLayout = new QFormLayout(compressionGroup);
    
    QCheckBox *autoCompressCheck = new QCheckBox("Auto-compress large files");
    QComboBox *compressionAlgoCombo = new QComboBox();
    compressionAlgoCombo->addItems({"ZLIB", "GZIP", "LZ4", "ZSTD"});
    QSpinBox *compressionLevelSpin = new QSpinBox();
    compressionLevelSpin->setRange(1, 9);
    compressionLevelSpin->setValue(6);
    
    compressionLayout->addRow(autoCompressCheck);
    compressionLayout->addRow("Algorithm:", compressionAlgoCombo);
    compressionLayout->addRow("Level:", compressionLevelSpin);
    
    securityLayout->addWidget(encryptionGroup);
    securityLayout->addWidget(compressionGroup);
    securityLayout->addStretch();
    
    tabWidget->addTab(generalTab, "General");
    tabWidget->addTab(securityTab, "Security");
    
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    layout->addWidget(tabWidget);
    layout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, &settingsDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &settingsDialog, &QDialog::reject);
    
    if (settingsDialog.exec() == QDialog::Accepted) {
        // TODO: Save settings
        statusBar()->showMessage("Settings saved", 2000);
    }
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QMenu contextMenu(this);
    
    QAction *openAction = new QAction("Open", this);
    QAction *newFileAction = new QAction("New File", this);
    QAction *newFolderAction = new QAction("New Folder", this);
    QAction *importAction = new QAction("Import File", this);
    QAction *copyAction = new QAction("Copy", this);
    QAction *cutAction = new QAction("Cut", this);
    QAction *pasteAction = new QAction("Paste", this);
    QAction *renameAction = new QAction("Rename", this);
    QAction *deleteAction = new QAction("Delete", this);
    QAction *propertiesAction = new QAction("Properties", this);
    
    contextMenu.addAction(openAction);
    contextMenu.addSeparator();
    contextMenu.addAction(newFileAction);
    contextMenu.addAction(newFolderAction);
    contextMenu.addAction(importAction);
    contextMenu.addSeparator();
    contextMenu.addAction(copyAction);
    contextMenu.addAction(cutAction);
    contextMenu.addAction(pasteAction);
    contextMenu.addSeparator();
    contextMenu.addAction(renameAction);
    contextMenu.addAction(deleteAction);
    contextMenu.addSeparator();
    contextMenu.addAction(propertiesAction);
    
    // Connect actions
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(newFileAction, &QAction::triggered, this, [this]() { createNewFile(); });
    connect(newFolderAction, &QAction::triggered, this, [this]() { createNewFolder(); });
    connect(importAction, &QAction::triggered, this, [this]() { importFile(); });
    connect(copyAction, &QAction::triggered, this, [this]() { copyFile(); });
    connect(cutAction, &QAction::triggered, this, [this]() { cutFile(); });
    connect(pasteAction, &QAction::triggered, this, [this]() { pasteFile(); });
    connect(renameAction, &QAction::triggered, this, [this]() { renameSelected(); });
    connect(deleteAction, &QAction::triggered, this, [this]() { deleteSelected(); });
    connect(propertiesAction, &QAction::triggered, this, [this]() { showFileProperties(); });
    
    contextMenu.exec(mapToGlobal(pos));
}

void MainWindow::copyFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (!selected.isEmpty()) {
        QString fileName = selected.first()->text(0);
        QApplication::clipboard()->setText(fileName);
        statusBar()->showMessage(QString("Copied %1").arg(fileName), 2000);
    }
}

void MainWindow::pasteFile() {
    QString clipboardText = QApplication::clipboard()->text();
    if (!clipboardText.isEmpty()) {
        // TODO: Implement paste functionality with VFS
        statusBar()->showMessage(QString("Pasted %1").arg(clipboardText), 2000);
    }
}

void MainWindow::cutFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (!selected.isEmpty()) {
        QString fileName = selected.first()->text(0);
        QApplication::clipboard()->setText(fileName);
        statusBar()->showMessage(QString("Cut %1").arg(fileName), 2000);
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About SVFS", 
        "Secure Virtual File System (SVFS)\n\n"
        "Version 2.0\n"
        "A secure file management system with encryption and compression capabilities.\n\n"
        "Features:\n"
        "• Real SQLite database storage\n"
        "• AES-256 encryption\n"
        "• ZLIB/GZIP compression\n"
        "• User authentication with salted hashes\n"
        "• File import/export\n"
        "• Search functionality\n"
        "• Modern UI with tabs and toolbars\n"
        "• Context menus\n"
        "• Settings dialog\n"
        "• File properties\n"
        "• Secure file operations\n\n"
        "Built with Qt6 and modern C++17.");
}