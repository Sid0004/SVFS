//
// Created by siddh on 31-08-2025.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTreeWidget;
class QTextEdit;

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
    void updateActions();
    
    // File operations
    void createNewFile();
    void createNewFolder();
    void importFile();
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
    
    // Utility methods
    QString formatFileSize(qint64 bytes);

    QTreeWidget *fileTree;
    QTextEdit *filePreview;
};

#endif // MAINWINDOW_H
