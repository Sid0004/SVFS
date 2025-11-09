//
// Created by siddh on 31-08-2025.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTreeWidget;
class QTextEdit;
class QLabel;
class QProgressBar;
class QAction;
class FileSystemScanner;
class QTreeWidgetItem;
template <typename K, typename V> class QHash;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void openFile();

private:
    // UI setup methods
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void loadFileTree();
    void loadFileTree(const QString &path);
    void updateActions();
    
    // File operations
    void createNewFile();
    void createNewFolder();
    void importFile();
    void exportFile();
    void deleteSelected();
    void renameSelected();
    void refreshFileTree();
    void searchFiles(const QString &searchText);
    void showFileProperties();
    void showSettings();
    void showContextMenu(const QPoint &pos);
    void copyFile();
    void pasteFile();
    void cutFile();
    void showAbout();
    
    // Advanced operations
    void editFile();
    void encryptFile();
    void decryptFile();
    void compressFile();
    void decompressFile();
    void showFileEditor(const QString &title, const QString &initialContent, int fileId = -1);
    // VFS management
    void createNewVFS();
    void openExistingVFS();
    void updateWindowTitle();
    void updateActionStates();

    // System Scan (read-only)
    void scanDrive();
    void cancelScan();
    void navigateUp();
    void importSelectedToVFS();     // Import scanned files to VFS with encryption/compression
    void batchEncryptCompress();    // Batch encrypt/compress selected items
    void togglePreviewPanel();      // Show/hide preview tabs
    void applyTheme(const QString &themeName); // runtime theme switch
    
    // Utility methods
    QString formatFileSize(qint64 bytes);

    QTreeWidget *fileTree;
    QTextEdit *filePreview;
    class QTabWidget *m_previewTabs = nullptr; // hold preview tabs for toggling
    QString m_currentTheme = "System";
    QString m_currentVfsPath = "svfs.db"; // currently opened VFS database file
    QString m_currentPath = "/"; // VFS current directory path
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    bool m_vfsIsOpen = false;
    bool m_scanMode = false; // true when showing system scan results
    FileSystemScanner *m_scanner = nullptr;
    QAction *m_scanAction = nullptr;
    QAction *m_cancelScanAction = nullptr;
    
    // Clipboard for copy/paste
    int m_clipboardFileId = -1;
    bool m_cutMode = false;

    // System scan helpers
    QString m_scanRootPath;
    QHash<QString, QTreeWidgetItem*> m_scanPathIndex; // absolute path -> item
};

#endif // MAINWINDOW_H
