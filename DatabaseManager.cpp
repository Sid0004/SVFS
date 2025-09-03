//
// Created by siddh on 31-08-2025.
//
#include "DatabaseManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initializeDatabase(const QString &dbPath) {
    if (m_isInitialized) {
        return true;
    }
    
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        qDebug() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    m_isInitialized = true;
    return createTables();
}

bool DatabaseManager::createTables() {
    QSqlQuery query(m_database);
    
    // Users table
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash BLOB NOT NULL,
            salt BLOB NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_login DATETIME,
            is_active BOOLEAN DEFAULT 1
        )
    )";
    
    if (!query.exec(createUsersTable)) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    // Directories table
    QString createDirectoriesTable = R"(
        CREATE TABLE IF NOT EXISTS directories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            path TEXT NOT NULL,
            parent_id INTEGER,
            user_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            modified_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (parent_id) REFERENCES directories(id),
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    
    if (!query.exec(createDirectoriesTable)) {
        qDebug() << "Failed to create directories table:" << query.lastError().text();
        return false;
    }
    
    // Files table
    QString createFilesTable = R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            path TEXT NOT NULL,
            content BLOB,
            encrypted_content BLOB,
            mime_type TEXT,
            size INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            modified_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            user_id INTEGER NOT NULL,
            is_encrypted BOOLEAN DEFAULT 0,
            is_compressed BOOLEAN DEFAULT 0,
            checksum BLOB,
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    
    if (!query.exec(createFilesTable)) {
        qDebug() << "Failed to create files table:" << query.lastError().text();
        return false;
    }
    
    // Create indexes for better performance
    query.exec("CREATE INDEX IF NOT EXISTS idx_files_path ON files(path)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_files_user_id ON files(user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_directories_path ON directories(path)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_directories_user_id ON directories(user_id)");
    
    return true;
}

QByteArray DatabaseManager::generateSalt() {
    QByteArray salt(32, 0);
    for (int i = 0; i < 32; ++i) {
        salt[i] = QRandomGenerator::global()->generate() % 256;
    }
    return salt;
}

QByteArray DatabaseManager::hashPassword(const QString &password, const QByteArray &salt) {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(password.toUtf8());
    hash.addData(salt);
    return hash.result();
}

bool DatabaseManager::verifyPassword(const QString &password, const QByteArray &hash, const QByteArray &salt) {
    QByteArray computedHash = hashPassword(password, salt);
    return computedHash == hash;
}

bool DatabaseManager::createUser(const QString &username, const QString &password) {
    QSqlQuery query(m_database);
    
    // Check if user already exists
    query.prepare("SELECT id FROM users WHERE username = ?");
    query.addBindValue(username);
    if (query.exec() && query.next()) {
        return false; // User already exists
    }
    
    QByteArray salt = generateSalt();
    QByteArray passwordHash = hashPassword(password, salt);
    
    query.prepare(R"(
        INSERT INTO users (username, password_hash, salt, created_at)
        VALUES (?, ?, ?, CURRENT_TIMESTAMP)
    )");
    query.addBindValue(username);
    query.addBindValue(passwordHash);
    query.addBindValue(salt);
    
    return query.exec();
}

bool DatabaseManager::authenticateUser(const QString &username, const QString &password, User &user) {
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM users WHERE username = ? AND is_active = 1");
    query.addBindValue(username);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    user.id = query.value("id").toInt();
    user.username = query.value("username").toString();
    user.passwordHash = query.value("password_hash").toByteArray();
    user.salt = query.value("salt").toByteArray();
    user.createdAt = query.value("created_at").toDateTime();
    user.lastLogin = query.value("last_login").toDateTime();
    user.isActive = query.value("is_active").toBool();
    
    if (!verifyPassword(password, user.passwordHash, user.salt)) {
        return false;
    }
    
    // Update last login
    updateUserLastLogin(user.id);
    return true;
}

bool DatabaseManager::updateUserLastLogin(int userId) {
    QSqlQuery query(m_database);
    query.prepare("UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(userId);
    return query.exec();
}

bool DatabaseManager::changePassword(int userId, const QString &newPassword) {
    QSqlQuery query(m_database);
    
    // Get current salt
    query.prepare("SELECT salt FROM users WHERE id = ?");
    query.addBindValue(userId);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    QByteArray salt = query.value("salt").toByteArray();
    QByteArray newPasswordHash = hashPassword(newPassword, salt);
    
    // Update password
    query.prepare("UPDATE users SET password_hash = ? WHERE id = ?");
    query.addBindValue(newPasswordHash);
    query.addBindValue(userId);
    
    return query.exec();
}

bool DatabaseManager::createFile(const FileRecord &file) {
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO files (filename, path, content, encrypted_content, mime_type, 
                          size, user_id, is_encrypted, is_compressed, checksum)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(file.filename);
    query.addBindValue(file.path);
    query.addBindValue(file.content);
    query.addBindValue(file.encryptedContent);
    query.addBindValue(file.mimeType);
    query.addBindValue(file.size);
    query.addBindValue(file.userId);
    query.addBindValue(file.isEncrypted);
    query.addBindValue(file.isCompressed);
    query.addBindValue(file.checksum);
    
    return query.exec();
}

bool DatabaseManager::updateFile(const FileRecord &file) {
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE files SET 
            filename = ?, path = ?, content = ?, encrypted_content = ?, 
            mime_type = ?, size = ?, modified_at = CURRENT_TIMESTAMP,
            is_encrypted = ?, is_compressed = ?, checksum = ?
        WHERE id = ?
    )");
    
    query.addBindValue(file.filename);
    query.addBindValue(file.path);
    query.addBindValue(file.content);
    query.addBindValue(file.encryptedContent);
    query.addBindValue(file.mimeType);
    query.addBindValue(file.size);
    query.addBindValue(file.isEncrypted);
    query.addBindValue(file.isCompressed);
    query.addBindValue(file.checksum);
    query.addBindValue(file.id);
    
    return query.exec();
}

bool DatabaseManager::deleteFile(int fileId) {
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM files WHERE id = ?");
    query.addBindValue(fileId);
    return query.exec();
}

bool DatabaseManager::getFile(int fileId, FileRecord &file) {
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM files WHERE id = ?");
    query.addBindValue(fileId);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    file.id = query.value("id").toInt();
    file.filename = query.value("filename").toString();
    file.path = query.value("path").toString();
    file.content = query.value("content").toByteArray();
    file.encryptedContent = query.value("encrypted_content").toByteArray();
    file.mimeType = query.value("mime_type").toString();
    file.size = query.value("size").toLongLong();
    file.createdAt = query.value("created_at").toDateTime();
    file.modifiedAt = query.value("modified_at").toDateTime();
    file.userId = query.value("user_id").toInt();
    file.isEncrypted = query.value("is_encrypted").toBool();
    file.isCompressed = query.value("is_compressed").toBool();
    file.checksum = query.value("checksum").toByteArray();
    
    return true;
}

QList<FileRecord> DatabaseManager::getFilesInDirectory(const QString &path, int userId) {
    QList<FileRecord> files;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM files WHERE path = ? AND user_id = ?");
    query.addBindValue(path);
    query.addBindValue(userId);
    
    if (query.exec()) {
        while (query.next()) {
            FileRecord file;
            file.id = query.value("id").toInt();
            file.filename = query.value("filename").toString();
            file.path = query.value("path").toString();
            file.content = query.value("content").toByteArray();
            file.encryptedContent = query.value("encrypted_content").toByteArray();
            file.mimeType = query.value("mime_type").toString();
            file.size = query.value("size").toLongLong();
            file.createdAt = query.value("created_at").toDateTime();
            file.modifiedAt = query.value("modified_at").toDateTime();
            file.userId = query.value("user_id").toInt();
            file.isEncrypted = query.value("is_encrypted").toBool();
            file.isCompressed = query.value("is_compressed").toBool();
            file.checksum = query.value("checksum").toByteArray();
            files.append(file);
        }
    }
    
    return files;
}

QList<FileRecord> DatabaseManager::searchFiles(const QString &query, int userId) {
    QList<FileRecord> files;
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare("SELECT * FROM files WHERE (filename LIKE ? OR path LIKE ?) AND user_id = ?");
    QString searchPattern = "%" + query + "%";
    sqlQuery.addBindValue(searchPattern);
    sqlQuery.addBindValue(searchPattern);
    sqlQuery.addBindValue(userId);
    
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            FileRecord file;
            file.id = sqlQuery.value("id").toInt();
            file.filename = sqlQuery.value("filename").toString();
            file.path = sqlQuery.value("path").toString();
            file.content = sqlQuery.value("content").toByteArray();
            file.encryptedContent = sqlQuery.value("encrypted_content").toByteArray();
            file.mimeType = sqlQuery.value("mime_type").toString();
            file.size = sqlQuery.value("size").toLongLong();
            file.createdAt = sqlQuery.value("created_at").toDateTime();
            file.modifiedAt = sqlQuery.value("modified_at").toDateTime();
            file.userId = sqlQuery.value("user_id").toInt();
            file.isEncrypted = sqlQuery.value("is_encrypted").toBool();
            file.isCompressed = sqlQuery.value("is_compressed").toBool();
            file.checksum = sqlQuery.value("checksum").toByteArray();
            files.append(file);
        }
    }
    
    return files;
}

bool DatabaseManager::createDirectory(const DirectoryRecord &dir) {
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO directories (name, path, parent_id, user_id, created_at, modified_at)
        VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");
    
    query.addBindValue(dir.name);
    query.addBindValue(dir.path);
    query.addBindValue(dir.parentId);
    query.addBindValue(dir.userId);
    
    return query.exec();
}

bool DatabaseManager::getDirectory(int dirId, DirectoryRecord &dir) {
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM directories WHERE id = ?");
    query.addBindValue(dirId);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    dir.id = query.value("id").toInt();
    dir.name = query.value("name").toString();
    dir.path = query.value("path").toString();
    dir.parentId = query.value("parent_id").toInt();
    dir.userId = query.value("user_id").toInt();
    dir.createdAt = query.value("created_at").toDateTime();
    dir.modifiedAt = query.value("modified_at").toDateTime();
    
    return true;
}

bool DatabaseManager::deleteDirectory(int dirId) {
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM directories WHERE id = ?");
    query.addBindValue(dirId);
    return query.exec();
}

QList<DirectoryRecord> DatabaseManager::getDirectoriesInPath(const QString &path, int userId) {
    QList<DirectoryRecord> directories;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM directories WHERE path = ? AND user_id = ?");
    query.addBindValue(path);
    query.addBindValue(userId);
    
    if (query.exec()) {
        while (query.next()) {
            DirectoryRecord dir;
            dir.id = query.value("id").toInt();
            dir.name = query.value("name").toString();
            dir.path = query.value("path").toString();
            dir.parentId = query.value("parent_id").toInt();
            dir.userId = query.value("user_id").toInt();
            dir.createdAt = query.value("created_at").toDateTime();
            dir.modifiedAt = query.value("modified_at").toDateTime();
            directories.append(dir);
        }
    }
    
    return directories;
}

qint64 DatabaseManager::getTotalStorageUsed(int userId) {
    QSqlQuery query(m_database);
    query.prepare("SELECT SUM(size) FROM files WHERE user_id = ?");
    query.addBindValue(userId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    
    return 0;
}

int DatabaseManager::getFileCount(int userId) {
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM files WHERE user_id = ?");
    query.addBindValue(userId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

int DatabaseManager::getDirectoryCount(int userId) {
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM directories WHERE user_id = ?");
    query.addBindValue(userId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

void DatabaseManager::closeDatabase() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    m_isInitialized = false;
}