//
// Created by siddh on 31-08-2025.
//
#ifndef VFSMANAGER_H
#define VFSMANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QDateTime>
#include "DatabaseManager.h"
#include "EncryptionManager.h"
#include "CompressionManager.h"

class VFSManager : public QObject {
    Q_OBJECT

public:
    static VFSManager& instance();
    
    // User management
    bool createUser(const QString &username, const QString &password);
    bool authenticateUser(const QString &username, const QString &password);
    void setCurrentUser(int userId);
    int getCurrentUserId() const;
    
    // File operations
    bool createFile(const QString &filename, const QString &path, const QByteArray &content, 
                   bool encrypt = false, bool compress = false);
    bool updateFile(int fileId, const QByteArray &content);
    bool deleteFile(int fileId);
    bool getFileContent(int fileId, QByteArray &content);
    QList<FileRecord> getFilesInDirectory(const QString &path);
    QList<FileRecord> searchFiles(const QString &query);
    
    // Directory operations
    bool createDirectory(const QString &name, const QString &path);
    bool deleteDirectory(int dirId);
    QList<DirectoryRecord> getDirectoriesInPath(const QString &path);
    
    // File import/export
    bool importFile(const QString &localPath, const QString &vfsPath, bool encrypt = false, bool compress = false);
    bool exportFile(int fileId, const QString &localPath);
    
    // Statistics
    qint64 getTotalStorageUsed();
    int getFileCount();
    int getDirectoryCount();
    
    // Security
    bool changePassword(const QString &newPassword);
    void logout();

signals:
    void fileCreated(int fileId, const QString &filename);
    void fileDeleted(int fileId);
    void fileUpdated(int fileId);
    void directoryCreated(int dirId, const QString &name);
    void directoryDeleted(int dirId);

private:
    VFSManager() = default;
    ~VFSManager() = default;
    VFSManager(const VFSManager&) = delete;
    VFSManager& operator=(const VFSManager&) = delete;
    
    int m_currentUserId = -1;
    QString m_currentUserPassword;
    
    QString getMimeType(const QString &filename);
    QByteArray processContent(const QByteArray &content, bool encrypt, bool compress);
    QByteArray unprocessContent(const QByteArray &processedContent, bool isEncrypted, bool isCompressed);
};

#endif // VFSMANAGER_H
