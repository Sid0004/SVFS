//
// Created by siddh on 31-08-2025.
//
#include "MainWindow.h"
#include "VFSManager.h"
#include "DatabaseManager.h"
#include "LoginDialog.h"
#include "FileSystemScanner.h"
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
#include <QFileDialog>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Secure Virtual File System - SVFS");
    resize(1200, 800);
    setMinimumSize(800, 600);
    
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    // Apply persisted theme
    {
        QSettings settings("SVFS", "SecureVFS");
        QString theme = settings.value("ui/theme", "System").toString();
        if (!theme.isEmpty()) {
            applyTheme(theme);
        }
    }
    loadFileTree("/");
    updateWindowTitle();
    updateActionStates();
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
    m_previewTabs = new QTabWidget(mainSplitter);
    m_previewTabs->setTabPosition(QTabWidget::South);
    
    // Text preview tab
    filePreview = new QTextEdit(m_previewTabs);
    filePreview->setReadOnly(true);
    filePreview->setFont(QFont("Consolas", 10));
    m_previewTabs->addTab(filePreview, "Text Preview");
    
    // Image preview tab
    QLabel *imagePreview = new QLabel(m_previewTabs);
    imagePreview->setAlignment(Qt::AlignCenter);
    imagePreview->setStyleSheet("border: 1px solid #ccc; background: white;");
    imagePreview->setText("No image selected");
    m_previewTabs->addTab(imagePreview, "Image Preview");
    
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
    
    m_previewTabs->addTab(propertiesWidget, "Properties");
    
    mainSplitter->addWidget(m_previewTabs);
    
    // Set splitter proportions
    mainSplitter->setSizes({800, 400});
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
    
    // New/Open VFS actions
    QAction *newVfsAction = new QAction("New &VFS...", this);
    newVfsAction->setStatusTip("Create a new VFS (SQLite file)");
    QAction *openVfsAction = new QAction("&Open VFS...", this);
    openVfsAction->setStatusTip("Open an existing VFS (SQLite file)");
    
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
    
    QAction *exportFileAction = new QAction("&Export File", this);
    exportFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    exportFileAction->setShortcut(QKeySequence("Ctrl+E"));
    exportFileAction->setStatusTip("Export selected file to local system");
    
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
    
    fileMenu->addAction(newVfsAction);
    fileMenu->addAction(openVfsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(newFileAction);
    fileMenu->addAction(newFolderAction);
    fileMenu->addAction(importFileAction);
    fileMenu->addAction(exportFileAction);
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
    QAction *togglePreview = new QAction("Toggle Preview Panel", this);
    togglePreview->setCheckable(false);
    viewMenu->addAction(togglePreview);
    connect(togglePreview, &QAction::triggered, this, [this]() { togglePreviewPanel(); });
    
    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    
    QAction *propertiesAction = new QAction("&Properties", this);
    propertiesAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    propertiesAction->setShortcut(QKeySequence("Alt+Return"));
    propertiesAction->setStatusTip("Show file properties");
    
    QAction *settingsAction = new QAction("&Settings", this);
    
    // System scan actions
    m_scanAction = new QAction("Scan &Drive...", this);
    m_scanAction->setStatusTip("Scan a folder/drive (read-only) and list files");
    m_cancelScanAction = new QAction("&Cancel Scan", this);
    m_cancelScanAction->setStatusTip("Cancel ongoing system scan");
    m_cancelScanAction->setEnabled(false);
    settingsAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsAction->setStatusTip("Open settings");
    
    toolsMenu->addAction(propertiesAction);
    toolsMenu->addAction(settingsAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_scanAction);
    toolsMenu->addAction(m_cancelScanAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    QAction *aboutAction = new QAction("&About", this);
    aboutAction->setStatusTip("About SVFS");
    
    helpMenu->addAction(aboutAction);
    
    // Connect actions
    connect(newVfsAction, &QAction::triggered, this, [this]() { createNewVFS(); });
    connect(openVfsAction, &QAction::triggered, this, [this]() { openExistingVFS(); });
    connect(newFileAction, &QAction::triggered, this, [this]() { createNewFile(); });
    connect(newFolderAction, &QAction::triggered, this, [this]() { createNewFolder(); });
    connect(importFileAction, &QAction::triggered, this, [this]() { importFile(); });
    connect(exportFileAction, &QAction::triggered, this, &MainWindow::exportFile);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(deleteAction, &QAction::triggered, this, [this]() { deleteSelected(); });
    connect(renameAction, &QAction::triggered, this, [this]() { renameSelected(); });
    connect(copyAction, &QAction::triggered, this, [this]() { copyFile(); });
    connect(pasteAction, &QAction::triggered, this, [this]() { pasteFile(); });
    connect(cutAction, &QAction::triggered, this, [this]() { cutFile(); });
    connect(refreshAction, &QAction::triggered, this, [this]() { refreshFileTree(); });
    connect(propertiesAction, &QAction::triggered, this, [this]() { showFileProperties(); });
    connect(settingsAction, &QAction::triggered, this, [this]() { showSettings(); });
    connect(m_scanAction, &QAction::triggered, this, [this]() { scanDrive(); });
    connect(m_cancelScanAction, &QAction::triggered, this, [this]() { cancelScan(); });
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
    QAction *upAction = new QAction("Up", this);
    upAction->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    QAction *togglePreviewAction = new QAction("Toggle Preview", this);
    togglePreviewAction->setIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
    togglePreviewAction->setToolTip("Show/Hide preview panel (saves space)");
    
    mainToolBar->addAction(newFileAction);
    mainToolBar->addAction(newFolderAction);
    mainToolBar->addAction(importFileAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(openAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(deleteAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(refreshAction);
    mainToolBar->addAction(upAction);
    mainToolBar->addAction(togglePreviewAction);
    // Algorithm selectors
    QComboBox *encAlgoCombo = new QComboBox(mainToolBar);
    encAlgoCombo->addItems({"AES-256-GCM", "AES-256-CBC", "ChaCha20-Poly1305"});
    encAlgoCombo->setCurrentIndex(0);
    mainToolBar->addWidget(encAlgoCombo);
    QComboBox *compAlgoCombo = new QComboBox(mainToolBar);
    compAlgoCombo->addItems({"ZLIB", "LZ4", "ZSTD"});
    compAlgoCombo->setCurrentIndex(0);
    mainToolBar->addWidget(compAlgoCombo);
    QComboBox *themeCombo = new QComboBox(mainToolBar);
    themeCombo->addItems({"System", "Light", "Dark", "High Contrast"});
    themeCombo->setCurrentText(m_currentTheme);
    mainToolBar->addWidget(themeCombo);
    
    // Connect toolbar actions
    connect(newFileAction, &QAction::triggered, this, [this]() { createNewFile(); });
    connect(newFolderAction, &QAction::triggered, this, [this]() { createNewFolder(); });
    connect(importFileAction, &QAction::triggered, this, [this]() { importFile(); });
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(deleteAction, &QAction::triggered, this, [this]() { deleteSelected(); });
    connect(refreshAction, &QAction::triggered, this, [this]() { refreshFileTree(); });
    connect(upAction, &QAction::triggered, this, [this]() { navigateUp(); });
    connect(togglePreviewAction, &QAction::triggered, this, [this]() { togglePreviewPanel(); });
    connect(encAlgoCombo, &QComboBox::currentTextChanged, this, [this](const QString &text){
        using EA = EncryptionManager::EncryptionAlgorithm;
        EA alg = EA::AES_256_GCM;
        if (text.contains("CBC")) alg = EA::AES_256_CBC; else if (text.contains("ChaCha")) alg = EA::ChaCha20_Poly1305;
        VFSManager::instance().setDefaultEncryptionAlgorithm(alg);
        m_statusLabel->setText(QString("Encryption algorithm: %1").arg(text));
    });
    connect(compAlgoCombo, &QComboBox::currentTextChanged, this, [this](const QString &text){
        using CA = CompressionManager::CompressionAlgorithm;
        CA alg = CA::ZLIB;
        if (text == "LZ4") alg = CA::LZ4; else if (text == "ZSTD") alg = CA::ZSTD;
        VFSManager::instance().setDefaultCompressionAlgorithm(alg);
        m_statusLabel->setText(QString("Compression algorithm: %1").arg(text));
    });
    connect(themeCombo, &QComboBox::currentTextChanged, this, [this](const QString &text){ applyTheme(text); });
}

void MainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

void MainWindow::loadFileTree() {
    loadFileTree(m_currentPath);
}

void MainWindow::loadFileTree(const QString &path) {
    fileTree->clear();
    
    if (!m_vfsIsOpen || VFSManager::instance().getCurrentUserId() == -1) {
        QTreeWidgetItem *item = new QTreeWidgetItem(fileTree, 
            QStringList() << "No VFS opened" << "" << "" << "");
        item->setForeground(0, Qt::gray);
        m_statusLabel->setText("No VFS opened - Use File → New VFS or Open VFS");
        return;
    }
    m_currentPath = path;
    
    // Load directories from VFS
    QList<DirectoryRecord> directories = VFSManager::instance().getDirectoriesInPath(path);
    for (const auto &dir : directories) {
        QTreeWidgetItem *dirItem = new QTreeWidgetItem(fileTree, 
            QStringList() << dir.name << "Folder" << dir.createdAt.toString("yyyy-MM-dd") << "Directory");
        dirItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        dirItem->setData(0, Qt::UserRole, dir.id);
        dirItem->setData(0, Qt::UserRole + 1, "directory");
    }
    
    // Load files from VFS
    QList<FileRecord> files = VFSManager::instance().getFilesInDirectory(path);
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
    
    // Update status with stats
    int fileCount = VFSManager::instance().getFileCount();
    int dirCount = VFSManager::instance().getDirectoryCount();
    qint64 totalSize = VFSManager::instance().getTotalStorageUsed();
    m_statusLabel->setText(QString("VFS: %1 | Files: %2 | Folders: %3 | Size: %4")
        .arg(QFileInfo(m_currentVfsPath).fileName())
        .arg(fileCount)
        .arg(dirCount)
        .arg(formatFileSize(totalSize)));
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

    // If we are in system scan mode, open the real file/folder in explorer (read-only)
    if (itemType == "scan_file" || itemType == "scan_dir") {
        QString path = item->toolTip(0);
        if (!path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            m_statusLabel->setText(QString("Opened: %1").arg(path));
        }
        return;
    }
    // Directory navigation in VFS
    if (itemType == "directory") {
        QString name = item->text(0);
        QString newPath = m_currentPath;
        if (!newPath.endsWith('/')) newPath += '/';
        newPath += name;
        loadFileTree(newPath);
        return;
    }
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file to open.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    m_statusLabel->setText(QString("Opening %1...").arg(fileName));
    
    // Get file content from VFS
    QByteArray content;
    if (VFSManager::instance().getFileContent(fileId, content)) {
        if (fileName.endsWith(".txt") || fileName.endsWith(".md") || fileName.endsWith(".cpp") || 
            fileName.endsWith(".h") || fileName.endsWith(".json") || fileName.endsWith(".xml") ||
            fileName.endsWith(".log") || fileName.endsWith(".csv")) {
            filePreview->setText(QString::fromUtf8(content));
        } else {
            filePreview->setText(QString("File: %1\nSize: %2 bytes\nType: Binary\n\nBinary file - content not displayed as text.\n\nUse 'Export' to save this file to disk.")
                .arg(fileName).arg(content.size()));
        }
        m_statusLabel->setText(QString("Opened: %1 (%2)").arg(fileName).arg(formatFileSize(content.size())));
    } else {
        filePreview->setText(QString("Error: Could not read file %1\n\nThe file may be corrupted or encryption key is invalid.").arg(fileName));
        m_statusLabel->setText("Error opening file");
    }
}

void MainWindow::createNewFile() {
    if (!m_vfsIsOpen || VFSManager::instance().getCurrentUserId() == -1) {
        QMessageBox::warning(this, "Error", "Please open a VFS and login first.");
        return;
    }
    
    showFileEditor("Create New File", "", -1);
}

void MainWindow::showFileEditor(const QString &title, const QString &initialContent, int fileId) {
    QDialog editorDialog(this);
    editorDialog.setWindowTitle(title);
    editorDialog.resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(&editorDialog);
    
    // File name input
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("File Name:");
    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("example.txt");
    
    if (fileId != -1) {
        // Editing existing file - get name from tree
        QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
        if (!selected.isEmpty()) {
            nameEdit->setText(selected.first()->text(0));
            nameEdit->setEnabled(false); // Don't allow renaming during edit
        }
    }
    
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    
    // Options row
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    QCheckBox *encryptCheck = new QCheckBox("Encrypt");
    QCheckBox *compressCheck = new QCheckBox("Compress");
    QLabel *infoLabel = new QLabel("(Options apply when saving)");
    infoLabel->setStyleSheet("color: gray; font-style: italic;");
    
    optionsLayout->addWidget(encryptCheck);
    optionsLayout->addWidget(compressCheck);
    optionsLayout->addWidget(infoLabel);
    optionsLayout->addStretch();
    
    // Text editor
    QTextEdit *editor = new QTextEdit();
    editor->setPlainText(initialContent);
    editor->setFont(QFont("Consolas", 10));
    editor->setLineWrapMode(QTextEdit::NoWrap);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Save to VFS");
    saveBtn->setStyleSheet("QPushButton { background-color: #27ae60; color: white; padding: 8px 20px; font-weight: bold; }");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("QPushButton { padding: 8px 20px; }");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    
    layout->addLayout(nameLayout);
    layout->addLayout(optionsLayout);
    layout->addWidget(editor);
    layout->addLayout(buttonLayout);
    
    connect(saveBtn, &QPushButton::clicked, [&]() {
        QString fileName = nameEdit->text().trimmed();
        if (fileName.isEmpty()) {
            QMessageBox::warning(&editorDialog, "Error", "Please enter a file name!");
            return;
        }
        
        QByteArray content = editor->toPlainText().toUtf8();
        bool encrypt = encryptCheck->isChecked();
        bool compress = compressCheck->isChecked();
        
        bool success = false;
        if (fileId == -1) {
            // Create new file
            success = VFSManager::instance().createFile(fileName, "/", content, encrypt, compress);
        } else {
            // Update existing file
            success = VFSManager::instance().updateFile(fileId, content);
        }
        
        if (success) {
            m_statusLabel->setText(QString("Saved: %1 (%2)").arg(fileName).arg(formatFileSize(content.size())));
            refreshFileTree();
            editorDialog.accept();
        } else {
            QMessageBox::warning(&editorDialog, "Error", "Failed to save file to VFS!");
        }
    });
    
    connect(cancelBtn, &QPushButton::clicked, &editorDialog, &QDialog::reject);
    
    editorDialog.exec();
}

void MainWindow::createNewFolder() {
    if (!m_vfsIsOpen || VFSManager::instance().getCurrentUserId() == -1) {
        QMessageBox::warning(this, "Error", "Please open a VFS and login first.");
        return;
    }
    
    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder", "Enter folder name:", 
                                             QLineEdit::Normal, "New Folder", &ok);
    if (ok && !folderName.isEmpty()) {
        if (VFSManager::instance().createDirectory(folderName, "/")) {
            m_statusLabel->setText(QString("Created folder: %1").arg(folderName));
            refreshFileTree();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create folder. Folder may already exist.");
        }
    }
}

void MainWindow::importFile() {
    if (!m_vfsIsOpen || VFSManager::instance().getCurrentUserId() == -1) {
        QMessageBox::warning(this, "Error", "Please open a VFS and login first.");
        return;
    }
    
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
                m_statusLabel->setText(QString("Imported file: %1").arg(vfsFileName));
                refreshFileTree();
            } else {
                QMessageBox::warning(this, "Error", "Failed to import file. Check file permissions and disk space.");
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
        m_statusLabel->setText(QString("Deleted %1 item(s)").arg(selected.size()));
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
        m_statusLabel->setText(QString("Renamed %1 to %2").arg(oldName, newName));
    }
}

void MainWindow::refreshFileTree() {
    m_statusLabel->setText("Refreshing file tree...");
    loadFileTree();
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
            QString algName = "-";
            QString compressedStr = file.isCompressed ? "Yes" : "No";
            if (file.isEncrypted && !file.encryptedContent.isEmpty()) {
                QByteArray plainTmp; unsigned char flags = 0; EncryptionManager::EncryptionAlgorithm detAlg;
                if (EncryptionManager::instance().decryptAndGetFlags(file.encryptedContent, plainTmp, flags, detAlg)) {
                    algName = EncryptionManager::instance().getAlgorithmName(detAlg);
                    if (flags & 0x01) compressedStr = "Yes (inside encrypted)";
                } else {
                    algName = "Unknown/Invalid";
                }
            }
            QString properties = QString(
                "Name: %1\n"
                "Path: %2\n"
                "Size: %3\n"
                "Type: %4\n"
                "Created: %5\n"
                "Modified: %6\n"
                "Encrypted: %7\n"
                "Algorithm: %8\n"
                "Compressed: %9\n"
                "User ID: %10"
            ).arg(file.filename, file.path, formatFileSize(file.size), file.mimeType,
                  file.createdAt.toString(), file.modifiedAt.toString(),
                  file.isEncrypted ? "Yes" : "No", algName, compressedStr,
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
    themeCombo->setCurrentText(m_currentTheme);
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
    encryptionAlgoCombo->addItems({"AES-256-GCM", "AES-256-CBC", "ChaCha20-Poly1305"});
    // Set current to match VFS default
    using EA = EncryptionManager::EncryptionAlgorithm;
    EA currentAlg = VFSManager::instance().defaultEncryptionAlgorithm();
    int encIndex = 0;
    if (currentAlg == EA::AES_256_CBC) encIndex = 1; else if (currentAlg == EA::ChaCha20_Poly1305) encIndex = 2; else encIndex = 0;
    encryptionAlgoCombo->setCurrentIndex(encIndex);
    
    encryptionLayout->addRow(autoEncryptCheck);
    encryptionLayout->addRow("Algorithm:", encryptionAlgoCombo);
    
    QGroupBox *compressionGroup = new QGroupBox("Compression");
    QFormLayout *compressionLayout = new QFormLayout(compressionGroup);
    
    QCheckBox *autoCompressCheck = new QCheckBox("Auto-compress large files");
    QComboBox *compressionAlgoCombo = new QComboBox();
    compressionAlgoCombo->addItems({"ZLIB", "LZ4", "ZSTD"});
    using CA = CompressionManager::CompressionAlgorithm;
    CA compDefault = VFSManager::instance().defaultCompressionAlgorithm();
    int compIndex = 0;
    if (compDefault == CA::LZ4) compIndex = 1; else if (compDefault == CA::ZSTD) compIndex = 2; else compIndex = 0;
    compressionAlgoCombo->setCurrentIndex(compIndex);
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
        applyTheme(themeCombo->currentText());
        // Map combo to algorithms: 0=GCM, 1=CBC, 2=ChaCha20
        EncryptionManager::EncryptionAlgorithm encAlg = EncryptionManager::AES_256_GCM;
        switch (encryptionAlgoCombo->currentIndex()) {
            case 1: encAlg = EncryptionManager::AES_256_CBC; break;
            case 2: encAlg = EncryptionManager::ChaCha20_Poly1305; break;
            default: encAlg = EncryptionManager::AES_256_GCM; break;
        }
        VFSManager::instance().setDefaultEncryptionAlgorithm(encAlg);
        CompressionManager::CompressionAlgorithm compAlg = CompressionManager::ZLIB;
        switch (compressionAlgoCombo->currentIndex()) { case 1: compAlg = CompressionManager::LZ4; break; case 2: compAlg = CompressionManager::ZSTD; break; default: compAlg = CompressionManager::ZLIB; }
        VFSManager::instance().setDefaultCompressionAlgorithm(compAlg);
        VFSManager::instance().setCompressionLevel(compressionLevelSpin->value());
        m_statusLabel->setText("Settings applied");
    }
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QMenu contextMenu(this);
    if (m_scanMode) {
        // Limited, safe menu during system scan
        QAction *openInExplorer = new QAction("Open", this);
        QAction *openFolder = new QAction("Open Containing Folder", this);
        QAction *copyPath = new QAction("Copy Path", this);
        QAction *openParent = new QAction("Open Parent Folder", this);
        QAction *importToVFS = new QAction("📥 Import to VFS", this);
        contextMenu.addAction(openInExplorer);
        contextMenu.addAction(openFolder);
        contextMenu.addAction(openParent);
        contextMenu.addSeparator();
        contextMenu.addAction(importToVFS);
        contextMenu.addAction(copyPath);

        connect(openInExplorer, &QAction::triggered, this, [this]() {
            auto items = fileTree->selectedItems();
            if (items.isEmpty()) return;
            QString path = items.first()->toolTip(0);
            if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
        connect(openFolder, &QAction::triggered, this, [this]() {
            auto items = fileTree->selectedItems();
            if (items.isEmpty()) return;
            QString path = items.first()->toolTip(0);
            if (path.isEmpty()) return;
            QFileInfo fi(path);
            QString folder = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        });
        connect(copyPath, &QAction::triggered, this, [this]() {
            auto items = fileTree->selectedItems();
            if (items.isEmpty()) return;
            QString path = items.first()->toolTip(0);
            QApplication::clipboard()->setText(path);
            m_statusLabel->setText("Path copied to clipboard");
        });
        connect(openParent, &QAction::triggered, this, [this]() {
            auto items = fileTree->selectedItems(); if (items.isEmpty()) return;
            QString p = items.first()->toolTip(0); QFileInfo fi(p);
            QString parent = fi.absoluteDir().absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(parent));
        });
        connect(importToVFS, &QAction::triggered, this, &MainWindow::importSelectedToVFS);
        contextMenu.exec(mapToGlobal(pos));
        return;
    }
    
    bool hasSelection = !fileTree->selectedItems().isEmpty();
    bool isFile = false;
    bool isEncrypted = false;
    bool isCompressed = false;
    
    if (hasSelection) {
        QTreeWidgetItem *item = fileTree->selectedItems().first();
        QString itemType = item->data(0, Qt::UserRole + 1).toString();
        isFile = (itemType == "file");
        
        if (isFile) {
            // Check file properties
            int fileId = item->data(0, Qt::UserRole).toInt();
            FileRecord file;
            if (DatabaseManager::instance().getFile(fileId, file)) {
                isEncrypted = file.isEncrypted;
                isCompressed = file.isCompressed;
            }
        }
    }
    
    QAction *openAction = new QAction(QIcon(":/icons/open"), "Open", this);
    QAction *editAction = new QAction(QIcon(":/icons/edit"), "Edit", this);
    QAction *newFileAction = new QAction(QIcon(":/icons/file"), "New File", this);
    QAction *newFolderAction = new QAction(QIcon(":/icons/folder"), "New Folder", this);
    QAction *importAction = new QAction(QIcon(":/icons/import"), "Import File", this);
    QAction *exportAction = new QAction(QIcon(":/icons/export"), "Export File", this);
    QAction *copyAction = new QAction("Copy", this);
    QAction *cutAction = new QAction("Cut", this);
    QAction *pasteAction = new QAction("Paste", this);
    QAction *renameAction = new QAction("Rename", this);
    QAction *deleteAction = new QAction("Delete", this);
    QAction *propertiesAction = new QAction("Properties", this);
    
    // Security actions
    QAction *encryptAction = new QAction("🔒 Encrypt", this);
    QAction *decryptAction = new QAction("🔓 Decrypt", this);
    QAction *compressAction = new QAction("📦 Compress", this);
    QAction *decompressAction = new QAction("📂 Decompress", this);
    
    openAction->setEnabled(hasSelection && isFile);
    editAction->setEnabled(hasSelection && isFile);
    exportAction->setEnabled(hasSelection && isFile);
    encryptAction->setEnabled(hasSelection && isFile && !isEncrypted);
    decryptAction->setEnabled(hasSelection && isFile && isEncrypted);
    compressAction->setEnabled(hasSelection && isFile && !isCompressed);
    decompressAction->setEnabled(hasSelection && isFile && isCompressed);
    
    contextMenu.addAction(openAction);
    contextMenu.addAction(editAction);
    contextMenu.addSeparator();
    contextMenu.addAction(newFileAction);
    contextMenu.addAction(newFolderAction);
    contextMenu.addAction(importAction);
    contextMenu.addAction(exportAction);
    contextMenu.addSeparator();
    contextMenu.addAction(encryptAction);
    contextMenu.addAction(decryptAction);
    contextMenu.addAction(compressAction);
    contextMenu.addAction(decompressAction);
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
    connect(editAction, &QAction::triggered, this, &MainWindow::editFile);
    connect(newFileAction, &QAction::triggered, this, [this]() { createNewFile(); });
    connect(newFolderAction, &QAction::triggered, this, [this]() { createNewFolder(); });
    connect(importAction, &QAction::triggered, this, [this]() { importFile(); });
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportFile);
    connect(encryptAction, &QAction::triggered, this, &MainWindow::encryptFile);
    connect(decryptAction, &QAction::triggered, this, &MainWindow::decryptFile);
    connect(compressAction, &QAction::triggered, this, &MainWindow::compressFile);
    connect(decompressAction, &QAction::triggered, this, &MainWindow::decompressFile);
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
        m_statusLabel->setText(QString("Copied %1 to clipboard").arg(fileName));
    }
}

void MainWindow::pasteFile() {
    QString clipboardText = QApplication::clipboard()->text();
    if (!clipboardText.isEmpty()) {
        // TODO: Implement paste functionality with VFS
        m_statusLabel->setText(QString("Paste operation not yet implemented"));
    }
}

void MainWindow::cutFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (!selected.isEmpty()) {
        QString fileName = selected.first()->text(0);
        QApplication::clipboard()->setText(fileName);
        m_statusLabel->setText(QString("Cut %1 (paste not yet implemented)").arg(fileName));
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
        "• Secure file operations\n"
        "• Built-in file editor\n"
        "• Encrypt/Decrypt/Compress files on-demand\n\n"
        "Built with Qt6 and modern C++17.");
}

void MainWindow::exportFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to export.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file to export.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    QString savePath = QFileDialog::getSaveFileName(this, "Export File", 
                                                     QDir::homePath() + "/" + fileName,
                                                     "All Files (*.*)");
    if (savePath.isEmpty()) return;
    
    if (VFSManager::instance().exportFile(fileId, savePath)) {
        m_statusLabel->setText(QString("Exported: %1 to %2").arg(fileName).arg(savePath));
        QMessageBox::information(this, "Success", 
            QString("File exported successfully to:\n%1").arg(savePath));
    } else {
        QMessageBox::critical(this, "Error", 
            "Failed to export file. Check permissions and disk space.");
    }
}

void MainWindow::editFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to edit.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file to edit.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    // Get file content
    QByteArray content;
    if (!VFSManager::instance().getFileContent(fileId, content)) {
        QMessageBox::critical(this, "Error", "Failed to read file content!");
        return;
    }
    
    showFileEditor("Edit File: " + fileName, QString::fromUtf8(content), fileId);
}

void MainWindow::encryptFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to encrypt.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    // Get file record
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        QMessageBox::critical(this, "Error", "Failed to get file information!");
        return;
    }
    
    if (file.isEncrypted) {
        QMessageBox::information(this, "Already Encrypted", 
            "This file is already encrypted!");
        return;
    }
    
    int ret = QMessageBox::question(this, "Encrypt File",
        QString("Encrypt file '%1'?\n\nThis will encrypt the file using AES-256 encryption.").arg(fileName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (VFSManager::instance().reprocessFile(fileId, true, file.isCompressed)) {
            m_statusLabel->setText(QString("Encrypted: %1").arg(fileName));
            refreshFileTree();
            QMessageBox::information(this, "Success", "File encrypted successfully!");
        } else {
            QMessageBox::critical(this, "Error", "Failed to encrypt file.");
        }
    }
}

void MainWindow::decryptFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to decrypt.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    // Get file record
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        QMessageBox::critical(this, "Error", "Failed to get file information!");
        return;
    }
    
    if (!file.isEncrypted) {
        QMessageBox::information(this, "Not Encrypted", 
            "This file is not encrypted!");
        return;
    }
    
    int ret = QMessageBox::question(this, "Decrypt File",
        QString("Decrypt file '%1'?\n\nThis will decrypt the file and store it as plain text in the VFS.").arg(fileName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (VFSManager::instance().reprocessFile(fileId, false, file.isCompressed)) {
            m_statusLabel->setText(QString("Decrypted: %1").arg(fileName));
            refreshFileTree();
            QMessageBox::information(this, "Success", "File decrypted successfully!");
        } else {
            QMessageBox::critical(this, "Error", "Failed to decrypt file. Wrong password?");
        }
    }
}

void MainWindow::compressFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to compress.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    // Get file record
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        QMessageBox::critical(this, "Error", "Failed to get file information!");
        return;
    }
    
    if (file.isCompressed) {
        QMessageBox::information(this, "Already Compressed", 
            "This file is already compressed!");
        return;
    }
    
    // Get current content
    QByteArray content;
    if (!VFSManager::instance().getFileContent(fileId, content)) {
        QMessageBox::critical(this, "Error", "Failed to read file!");
        return;
    }
    
    qint64 originalSize = content.size();
    
    // Compress
    QByteArray compressed = CompressionManager::instance().compress(content);
    if (compressed.isEmpty()) {
        QMessageBox::critical(this, "Error", "Compression failed!");
        return;
    }
    
    qint64 compressedSize = compressed.size();
    double ratio = (1.0 - ((double)compressedSize / (double)originalSize)) * 100.0;
    
    int ret = QMessageBox::question(this, "Compress File",
        QString("Compress file '%1'?\n\nOriginal: %2\nCompressed: %3\nSavings: %4%")
            .arg(fileName)
            .arg(formatFileSize(originalSize))
            .arg(formatFileSize(compressedSize))
            .arg(ratio, 0, 'f', 1),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (VFSManager::instance().reprocessFile(fileId, file.isEncrypted, true)) {
            m_statusLabel->setText(QString("Compressed: %1 (saved %2%)").arg(fileName).arg(ratio, 0, 'f', 1));
            refreshFileTree();
            QMessageBox::information(this, "Success", 
                QString("File compressed successfully!\nSaved %1% space.").arg(ratio, 0, 'f', 1));
        } else {
            QMessageBox::critical(this, "Error", "Failed to compress file!");
        }
    }
}

void MainWindow::decompressFile() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a file to decompress.");
        return;
    }
    
    QTreeWidgetItem *item = selected.first();
    QString itemType = item->data(0, Qt::UserRole + 1).toString();
    
    if (itemType != "file") {
        QMessageBox::information(this, "Invalid Selection", "Please select a file.");
        return;
    }
    
    int fileId = item->data(0, Qt::UserRole).toInt();
    QString fileName = item->text(0);
    
    // Get file record
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        QMessageBox::critical(this, "Error", "Failed to get file information!");
        return;
    }
    
    if (!file.isCompressed) {
        QMessageBox::information(this, "Not Compressed", 
            "This file is not compressed!");
        return;
    }
    
    int ret = QMessageBox::question(this, "Decompress File",
        QString("Decompress file '%1'?").arg(fileName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (VFSManager::instance().reprocessFile(fileId, file.isEncrypted, false)) {
            m_statusLabel->setText(QString("Decompressed: %1").arg(fileName));
            refreshFileTree();
            QMessageBox::information(this, "Success", "File decompressed successfully!");
        } else {
            QMessageBox::critical(this, "Error", "Failed to decompress file!");
        }
    }
}

void MainWindow::createNewVFS() {
    QString path = QFileDialog::getSaveFileName(this, "Create New VFS", QDir::homePath() + "/my_vault.svfsdb",
                                                "SVFS Database (*.svfsdb *.db);;All Files (*.*)");
    if (path.isEmpty()) return;
    
    // Close current VFS
    VFSManager::instance().logout();
    DatabaseManager::instance().closeDatabase();
    
    // Create/open new database
    if (!DatabaseManager::instance().reopenDatabase(path)) {
        QMessageBox::critical(this, "Error", 
            QString("Failed to create VFS database:\n%1\n\nPlease check file permissions and disk space.")
            .arg(path));
        return;
    }
    
    m_currentVfsPath = path;
    m_vfsIsOpen = true;
    
    // Prompt to create first user account
    QMessageBox::information(this, "New VFS Created", 
        "Your new VFS has been created successfully!\n\n"
        "Please create an administrator account to secure this VFS.");
    
    // Show login dialog to create account
    LoginDialog login(this);
    if (login.exec() != QDialog::Accepted) {
        m_statusLabel->setText("VFS created but login cancelled");
        m_vfsIsOpen = false;
        loadFileTree();
        updateActionStates();
        return;
    }
    
    refreshFileTree();
    updateWindowTitle();
    updateActionStates();
    QMessageBox::information(this, "Success", 
        QString("VFS '%1' is now active and ready to use!").arg(QFileInfo(path).fileName()));
}

void MainWindow::openExistingVFS() {
    QString path = QFileDialog::getOpenFileName(this, "Open Existing VFS", QDir::homePath(),
                                                "SVFS Database (*.svfsdb *.db);;All Files (*.*)");
    if (path.isEmpty()) return;
    
    // Verify file exists and is readable
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        QMessageBox::critical(this, "Error", 
            QString("Cannot open VFS:\n%1\n\nFile does not exist or is not readable.").arg(path));
        return;
    }
    
    // Close current VFS
    VFSManager::instance().logout();
    DatabaseManager::instance().closeDatabase();
    
    // Open existing database
    if (!DatabaseManager::instance().reopenDatabase(path)) {
        QMessageBox::critical(this, "Error", 
            QString("Failed to open VFS database:\n%1\n\nThe file may be corrupted or not a valid SVFS database.")
            .arg(path));
        return;
    }
    
    m_currentVfsPath = path;
    m_vfsIsOpen = true;
    
    // Prompt login
    LoginDialog login(this);
    if (login.exec() != QDialog::Accepted) {
        m_statusLabel->setText("Login cancelled");
        m_vfsIsOpen = false;
        loadFileTree();
        updateActionStates();
        return;
    }
    
    refreshFileTree();
    updateWindowTitle();
    updateActionStates();
    m_statusLabel->setText(QString("Opened VFS: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::updateWindowTitle() {
    QString base = "Secure Virtual File System - SVFS";
    if (m_vfsIsOpen && !m_currentVfsPath.isEmpty()) {
        setWindowTitle(base + " [" + QFileInfo(m_currentVfsPath).fileName() + "]");
    } else {
        setWindowTitle(base + " [No VFS]");
    }
}

void MainWindow::updateActionStates() {
    // Enable/disable actions based on VFS vs system scan mode
    if (m_scanMode) {
        // When scanning system, disable VFS destructive actions implicitly handled by context menu checks
        m_scanAction->setEnabled(false);
        m_cancelScanAction->setEnabled(true);
        m_statusLabel->setText("System Scan Mode - read-only");
    } else {
        m_scanAction->setEnabled(true);
        m_cancelScanAction->setEnabled(false);
    }
}

void MainWindow::scanDrive() {
    if (m_scanner && m_cancelScanAction->isEnabled()) {
        // A scan is already running, ignore
        QMessageBox::information(this, "Scan Running", "A scan is already in progress.");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "Select Folder or Drive to Scan", QDir::homePath());
    if (dir.isEmpty()) return;
    m_scanRootPath = QDir::toNativeSeparators(dir);
    m_scanPathIndex.clear();

    if (!m_scanner) {
        m_scanner = new FileSystemScanner(this);
        // Connect signals
        connect(m_scanner, &FileSystemScanner::batchReady, this, [this](const QVector<FSItem> &items, qint64 totalFiles, qint64 totalBytes) {
            // Recursive directory creator using std::function to allow self-call
            std::function<QTreeWidgetItem*(const QString&)> ensureDirItem;
            ensureDirItem = [&](const QString &absPath) -> QTreeWidgetItem* {
                QString norm = QDir::toNativeSeparators(absPath);
                if (norm == m_scanRootPath || norm.isEmpty()) return nullptr; // root shows children directly
                if (m_scanPathIndex.contains(norm)) return m_scanPathIndex.value(norm);
                // create parent first
                QTreeWidgetItem *parentItem = ensureDirItem(QFileInfo(norm).absolutePath());
                QString label = QFileInfo(norm).fileName();
                if (label.isEmpty()) label = norm; // drive root fallback
                QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << label << "<DIR>" << "" << "Directory");
                item->setToolTip(0, norm);
                item->setData(0, Qt::UserRole + 1, "scan_dir");
                item->setData(0, Qt::UserRole, 0);
                item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
                if (parentItem) parentItem->addChild(item); else fileTree->addTopLevelItem(item);
                m_scanPathIndex.insert(norm, item);
                return item;
            };

            // Populate tree incrementally
            for (const auto &it : items) {
                QString sizeStr = it.isDir ? "<DIR>" : formatFileSize(it.size);
                QString typeStr = it.isDir ? "Directory" : "File";
                QTreeWidgetItem *node = new QTreeWidgetItem(QStringList() << it.name << sizeStr << it.modified.toString("yyyy-MM-dd HH:mm") << typeStr);
                node->setToolTip(0, it.path);
                node->setData(0, Qt::UserRole + 1, it.isDir ? "scan_dir" : "scan_file");
                node->setData(0, Qt::UserRole, it.size);
                if (it.isDir) node->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
                else node->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
                // Hierarchical insertion via index
                QTreeWidgetItem *parentItem = ensureDirItem(QFileInfo(it.path).absolutePath());
                if (it.isDir) {
                    if (parentItem) parentItem->addChild(node); else fileTree->addTopLevelItem(node);
                    m_scanPathIndex.insert(QDir::toNativeSeparators(it.path), node);
                } else {
                    if (parentItem) parentItem->addChild(node); else fileTree->addTopLevelItem(node);
                }
            }
            m_progressBar->setVisible(true);
            m_progressBar->setMaximum(0); // indefinite
            m_statusLabel->setText(QString("Scanning... Files: %1 Size: %2")
                .arg(totalFiles)
                .arg(formatFileSize(totalBytes)));
        });
        connect(m_scanner, &FileSystemScanner::finished, this, [this](qint64 totalFiles, qint64 totalBytes, qint64 elapsedMs, bool cancelled) {
            m_progressBar->setVisible(false);
            m_scanMode = false; // keep results but exit scan mode
            updateActionStates();
            m_statusLabel->setText(QString("Scan %1 - Files: %2 Total Size: %3 Time: %4 ms")
                .arg(cancelled ? "Cancelled" : "Complete")
                .arg(totalFiles)
                .arg(formatFileSize(totalBytes))
                .arg(elapsedMs));
        });
        connect(m_scanner, &FileSystemScanner::error, this, [this](const QString &msg){
            QMessageBox::critical(this, "Scan Error", msg);
            m_progressBar->setVisible(false);
            m_scanMode = false;
            updateActionStates();
        });
    }

    // Prepare UI
    fileTree->clear();
    m_scanMode = true;
    updateActionStates();
    m_statusLabel->setText("Starting system scan (read-only)...");
    m_scanner->startScan(dir, 400); // moderate batch size
}

void MainWindow::cancelScan() {
    if (m_scanner) {
        m_scanner->cancel();
        m_statusLabel->setText("Cancelling scan...");
    }
}

void MainWindow::navigateUp() {
    if (m_scanMode) return; // Not applicable during scan results
    if (m_currentPath == "/" || m_currentPath.isEmpty()) return;
    QString path = m_currentPath;
    if (path.endsWith('/') && path.size() > 1) path.chop(1);
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash <= 0) m_currentPath = "/"; else m_currentPath = path.left(lastSlash);
    loadFileTree(m_currentPath);
}

void MainWindow::togglePreviewPanel() {
    if (!m_previewTabs) return;
    bool visible = m_previewTabs->isVisible();
    m_previewTabs->setVisible(!visible);
}

void MainWindow::importSelectedToVFS() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Import", "Select files/folders from scan results to import.");
        return;
    }
    
    // Dialog for options
    QDialog dlg(this);
    dlg.setWindowTitle("Import Options");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    
    QCheckBox *encryptCheck = new QCheckBox("Encrypt files (AES-256-GCM)");
    encryptCheck->setChecked(true);
    QCheckBox *compressCheck = new QCheckBox("Compress files (ZLIB)");
    compressCheck->setChecked(true);
    
    layout->addWidget(new QLabel(QString("Importing %1 item(s)").arg(selected.size())));
    layout->addWidget(encryptCheck);
    layout->addWidget(compressCheck);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);
    
    if (dlg.exec() != QDialog::Accepted) return;
    
    bool doEncrypt = encryptCheck->isChecked();
    bool doCompress = compressCheck->isChecked();
    
    int imported = 0;
    int failed = 0;
    
    for (auto *item : selected) {
        QString type = item->data(0, Qt::UserRole + 1).toString();
        if (!type.startsWith("scan_")) continue; // skip VFS items
        
        bool isDir = (type == "scan_dir");
        if (isDir) continue; // skip directories for now (could recursively import later)
        
        QString realPath = item->toolTip(0);
        QString filename = item->text(0);
        
        QFile f(realPath);
        if (!f.open(QIODevice::ReadOnly)) {
            failed++;
            continue;
        }
        QByteArray content = f.readAll();
        f.close();
        
        if (VFSManager::instance().createFile(filename, m_currentPath, content, doEncrypt, doCompress)) {
            imported++;
        } else {
            failed++;
        }
    }
    
    loadFileTree();
    QMessageBox::information(this, "Import Complete", 
        QString("Imported %1 file(s). Failed: %2").arg(imported).arg(failed));
}

void MainWindow::batchEncryptCompress() {
    QList<QTreeWidgetItem*> selected = fileTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Batch Operation", "Select VFS files to encrypt/compress.");
        return;
    }
    
    // Dialog for options
    QDialog dlg(this);
    dlg.setWindowTitle("Batch Encrypt/Compress");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    
    QCheckBox *encryptCheck = new QCheckBox("Encrypt files (AES-256-GCM)");
    encryptCheck->setChecked(true);
    QCheckBox *compressCheck = new QCheckBox("Compress files (ZLIB)");
    compressCheck->setChecked(true);
    
    layout->addWidget(new QLabel(QString("Processing %1 VFS item(s)").arg(selected.size())));
    layout->addWidget(encryptCheck);
    layout->addWidget(compressCheck);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);
    
    if (dlg.exec() != QDialog::Accepted) return;
    
    bool doEncrypt = encryptCheck->isChecked();
    bool doCompress = compressCheck->isChecked();
    
    int processed = 0;
    int failed = 0;
    
    for (auto *item : selected) {
        QString type = item->data(0, Qt::UserRole + 1).toString();
        if (type != "file") continue; // only files
        
        int fileId = item->data(0, Qt::UserRole).toInt();
        // Reprocess file with selected options (ensures flags/header consistent)
        if (VFSManager::instance().reprocessFile(fileId, doEncrypt, doCompress)) processed++; else failed++;
    }
    
    loadFileTree();
    QMessageBox::information(this, "Batch Complete", 
        QString("Processed %1 file(s). Failed: %2").arg(processed).arg(failed));
}

void MainWindow::applyTheme(const QString &themeName) {
    m_currentTheme = themeName;
    QString style;
    if (themeName == "Light") {
        style = R"(QWidget { background: #f5f7fa; color: #2c3e50; }
QTreeWidget { background: #ffffff; }
QHeaderView::section { background: #ecf0f1; padding:4px; border:1px solid #d0d3d4; }
QToolBar { background:#ecf0f1; border:0; }
QLineEdit, QTextEdit { background:#ffffff; color:#2c3e50; border:1px solid #bdc3c7; }
QPushButton { background:#3498db; color:white; border-radius:3px; padding:4px 10px; }
QPushButton:hover { background:#2980b9; }
QTabBar::tab { background:#ecf0f1; padding:6px; }
QTabBar::tab:selected { background:#ffffff; }
)";
    } else if (themeName == "Dark") {
        style = R"(QWidget { background: #232629; color: #ecf0f1; }
QTreeWidget { background: #1b1e21; }
QHeaderView::section { background: #2f3337; padding:4px; border:1px solid #3e4347; }
QToolBar { background:#2f3337; border:0; }
QLineEdit, QTextEdit { background:#1b1e21; color:#ecf0f1; border:1px solid #3e4347; }
QPushButton { background:#3b7ddd; color:white; border-radius:3px; padding:4px 10px; }
QPushButton:hover { background:#2f67b5; }
QMenu { background:#2f3337; color:#ecf0f1; }
QTabBar::tab { background:#2f3337; padding:6px; }
QTabBar::tab:selected { background:#3b4147; }
QScrollBar:vertical { background:#2f3337; width:12px; }
QScrollBar::handle:vertical { background:#555b62; min-height:20px; border-radius:5px; }
)";
    } else if (themeName == "High Contrast") {
        style = R"(QWidget { background: #000; color: #fff; }
QTreeWidget { background:#000; color:#fff; }
QHeaderView::section { background:#000; color:#fff; border:2px solid #fff; }
QToolBar { background:#000; }
QLineEdit, QTextEdit { background:#000; color:#fff; border:2px solid #fff; }
QPushButton { background:#fff; color:#000; font-weight:bold; }
QPushButton:hover { background:#ff0; }
QMenu { background:#000; color:#fff; }
QTabBar::tab { background:#000; color:#fff; padding:6px; border:2px solid #fff; }
QTabBar::tab:selected { background:#fff; color:#000; }
)";
    } else { // System default
        style.clear();
    }
    qApp->setStyleSheet(style);
    QSettings settings("SVFS", "SecureVFS");
    settings.setValue("ui/theme", themeName);
    if (m_statusLabel) m_statusLabel->setText(QString("Theme applied: %1").arg(themeName));
}