//
// Created by siddh on 31-08-2025.
//
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMap>

struct User {
    int id;
    QString username;
    QByteArray passwordHash;
    QByteArray salt;
    QDateTime createdAt;
    QDateTime lastLogin;
    bool isActive;
};

struct FileRecord {
    int id;
    QString filename;
    QString path;
    QByteArray content;
    QByteArray encryptedContent;
    QString mimeType;
    qint64 size;
    QDateTime createdAt;
    QDateTime modifiedAt;
    int userId;
    bool isEncrypted;
    bool isCompressed;
    QByteArray checksum;
};

struct DirectoryRecord {
    int id;
    QString name;
    QString path;
    int parentId;
    int userId;
    QDateTime createdAt;
    QDateTime modifiedAt;
};

class DatabaseManager {

public:
    static DatabaseManager& instance();
    
    bool initializeDatabase(const QString &dbPath = "svfs.db");
    bool createTables();
    // Allow switching/opening another VFS database file at runtime
    bool reopenDatabase(const QString &dbPath);
    QString currentDatabasePath() const { return m_dbPath; }
    
    // User management
    bool createUser(const QString &username, const QString &password);
    bool authenticateUser(const QString &username, const QString &password, User &user);
    bool updateUserLastLogin(int userId);
    bool changePassword(int userId, const QString &newPassword);
    
    // File operations
    bool createFile(const FileRecord &file);
    bool updateFile(const FileRecord &file);
    bool deleteFile(int fileId);
    bool getFile(int fileId, FileRecord &file);
    QList<FileRecord> getFilesInDirectory(const QString &path, int userId);
    QList<FileRecord> searchFiles(const QString &query, int userId);
    
    // Directory operations
    bool createDirectory(const DirectoryRecord &dir);
    bool deleteDirectory(int dirId);
    bool getDirectory(int dirId, DirectoryRecord &dir);
    QList<DirectoryRecord> getDirectoriesInPath(const QString &path, int userId);
    
    // Statistics
    qint64 getTotalStorageUsed(int userId);
    int getFileCount(int userId);
    int getDirectoryCount(int userId);
    
    void closeDatabase();

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    QSqlDatabase m_database;
    bool m_isInitialized = false;
    QString m_connectionName = "svfs_connection";
    QString m_dbPath = "svfs.db";
    
    QByteArray generateSalt();
    QByteArray hashPassword(const QString &password, const QByteArray &salt);
    bool verifyPassword(const QString &password, const QByteArray &hash, const QByteArray &salt);
};

#endif // DATABASEMANAGER_H
