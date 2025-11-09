//
// Created by siddh on 31-08-2025.
//
#include "VFSManager.h"
#include <QFileInfo>
#include <QDebug>
#include <QMimeDatabase>
#include <QMimeType>

VFSManager::VFSManager() : QObject() {
}

VFSManager::~VFSManager() = default;

VFSManager& VFSManager::instance() {
    static VFSManager instance;
    return instance;
}

bool VFSManager::createUser(const QString &username, const QString &password) {
    return DatabaseManager::instance().createUser(username, password);
}

bool VFSManager::authenticateUser(const QString &username, const QString &password) {
    User user;
    if (DatabaseManager::instance().authenticateUser(username, password, user)) {
        m_currentUserId = user.id;
        m_currentUserPassword = password;
        
        // Load encryption key
        EncryptionManager::instance().loadKey(password, user.salt);
        
        return true;
    }
    return false;
}

void VFSManager::setCurrentUser(int userId) {
    m_currentUserId = userId;
}

int VFSManager::getCurrentUserId() const {
    return m_currentUserId;
}

bool VFSManager::createFile(const QString &filename, const QString &path, const QByteArray &content, 
                           bool encrypt, bool compress) {
    if (m_currentUserId == -1) return false;
    
    FileRecord file;
    file.filename = filename;
    file.path = path;
    file.mimeType = getMimeType(filename);
    file.size = content.size();
    file.userId = m_currentUserId;
    file.isEncrypted = encrypt;
    file.isCompressed = compress;
    file.createdAt = QDateTime::currentDateTime();
    file.modifiedAt = QDateTime::currentDateTime();
    
    // Process content (encrypt/compress)
    QByteArray processedContent = processContent(content, encrypt, compress);
    if (processedContent.isEmpty()) {
        return false;
    }
    
    if (encrypt) {
        file.encryptedContent = processedContent;
        file.content.clear();
    } else {
        file.content = processedContent;
        file.encryptedContent.clear();
    }
    
    // Calculate checksum
    file.checksum = EncryptionManager::instance().calculateChecksum(content);
    
    if (DatabaseManager::instance().createFile(file)) {
        emit fileCreated(file.id, filename);
        return true;
    }
    
    return false;
}

bool VFSManager::updateFile(int fileId, const QByteArray &content) {
    if (m_currentUserId == -1) return false;
    
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        return false;
    }
    
    if (file.userId != m_currentUserId) {
        return false; // Security check
    }
    
    // Process content
    QByteArray processedContent = processContent(content, file.isEncrypted, file.isCompressed);
    if (processedContent.isEmpty()) {
        return false;
    }
    
    if (file.isEncrypted) {
        file.encryptedContent = processedContent;
        file.content.clear();
    } else {
        file.content = processedContent;
        file.encryptedContent.clear();
    }
    
    file.size = content.size();
    file.modifiedAt = QDateTime::currentDateTime();
    file.checksum = EncryptionManager::instance().calculateChecksum(content);
    
    if (DatabaseManager::instance().updateFile(file)) {
        emit fileUpdated(fileId);
        return true;
    }
    
    return false;
}

bool VFSManager::deleteFile(int fileId) {
    if (m_currentUserId == -1) return false;
    
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        return false;
    }
    
    if (file.userId != m_currentUserId) {
        return false; // Security check
    }
    
    if (DatabaseManager::instance().deleteFile(fileId)) {
        emit fileDeleted(fileId);
        return true;
    }
    
    return false;
}

bool VFSManager::getFileContent(int fileId, QByteArray &content) {
    if (m_currentUserId == -1) return false;
    
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) {
        return false;
    }
    
    if (file.userId != m_currentUserId) {
        return false; // Security check
    }
    
    // Get the appropriate content
    QByteArray processedContent = file.isEncrypted ? file.encryptedContent : file.content;
    
    // Unprocess content (decrypt/decompress)
    content = unprocessContent(processedContent, file.isEncrypted, file.isCompressed);
    
    // Verify checksum
    QByteArray calculatedChecksum = EncryptionManager::instance().calculateChecksum(content);
    if (calculatedChecksum != file.checksum) {
        qDebug() << "Checksum verification failed for file" << fileId;
        return false;
    }
    
    return true;
}

QList<FileRecord> VFSManager::getFilesInDirectory(const QString &path) {
    if (m_currentUserId == -1) return QList<FileRecord>();
    
    return DatabaseManager::instance().getFilesInDirectory(path, m_currentUserId);
}

QList<FileRecord> VFSManager::searchFiles(const QString &query) {
    if (m_currentUserId == -1) return QList<FileRecord>();
    
    return DatabaseManager::instance().searchFiles(query, m_currentUserId);
}

bool VFSManager::createDirectory(const QString &name, const QString &path) {
    if (m_currentUserId == -1) return false;
    
    DirectoryRecord dir;
    dir.name = name;
    dir.path = path;
    dir.userId = m_currentUserId;
    dir.createdAt = QDateTime::currentDateTime();
    dir.modifiedAt = QDateTime::currentDateTime();
    
    if (DatabaseManager::instance().createDirectory(dir)) {
        emit directoryCreated(dir.id, name);
        return true;
    }
    
    return false;
}

bool VFSManager::deleteDirectory(int dirId) {
    if (m_currentUserId == -1) return false;
    
    DirectoryRecord dir;
    if (!DatabaseManager::instance().getDirectory(dirId, dir)) {
        return false;
    }
    
    if (dir.userId != m_currentUserId) {
        return false; // Security check
    }
    
    if (DatabaseManager::instance().deleteDirectory(dirId)) {
        emit directoryDeleted(dirId);
        return true;
    }
    
    return false;
}

QList<DirectoryRecord> VFSManager::getDirectoriesInPath(const QString &path) {
    if (m_currentUserId == -1) return QList<DirectoryRecord>();
    
    return DatabaseManager::instance().getDirectoriesInPath(path, m_currentUserId);
}

bool VFSManager::importFile(const QString &localPath, const QString &vfsPath, bool encrypt, bool compress) {
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray content = file.readAll();
    file.close();
    
    QFileInfo fileInfo(localPath);
    QString filename = fileInfo.fileName();
    
    return createFile(filename, vfsPath, content, encrypt, compress);
}

bool VFSManager::exportFile(int fileId, const QString &localPath) {
    QByteArray content;
    if (!getFileContent(fileId, content)) {
        return false;
    }
    
    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    qint64 bytesWritten = file.write(content);
    file.close();
    
    return bytesWritten == content.size();
}

qint64 VFSManager::getTotalStorageUsed() {
    if (m_currentUserId == -1) return 0;
    
    return DatabaseManager::instance().getTotalStorageUsed(m_currentUserId);
}

int VFSManager::getFileCount() {
    if (m_currentUserId == -1) return 0;
    
    return DatabaseManager::instance().getFileCount(m_currentUserId);
}

int VFSManager::getDirectoryCount() {
    if (m_currentUserId == -1) return 0;
    
    return DatabaseManager::instance().getDirectoryCount(m_currentUserId);
}

bool VFSManager::changePassword(const QString &newPassword) {
    if (m_currentUserId == -1) return false;
    
    if (DatabaseManager::instance().changePassword(m_currentUserId, newPassword)) {
        m_currentUserPassword = newPassword;
        return true;
    }
    
    return false;
}

void VFSManager::logout() {
    EncryptionManager::instance().clearKey();
    m_currentUserId = -1;
    m_currentUserPassword.clear();
}

QString VFSManager::getMimeType(const QString &filename) {
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(filename);
    return mimeType.name();
}

QByteArray VFSManager::processContent(const QByteArray &content, bool encrypt, bool compress) {
    QByteArray processedContent = content;
    
    // First compress if needed
    if (compress) {
        processedContent = CompressionManager::instance().compress(processedContent, m_defaultCompAlg, m_compLevel);
        if (processedContent.isEmpty()) {
            return QByteArray();
        }
    }
    
    // Then encrypt if needed (with compression flag in header)
    if (encrypt) {
        // bit0 = compressed; bits2-3 = compression algorithm (0=zlib,1=lz4,2=zstd)
        unsigned char flags = compress ? 0x01 : 0x00;
        if (compress) {
            unsigned char compCode = 0;
            switch (m_defaultCompAlg) {
                case CompressionManager::ZLIB: compCode = 0; break;
                case CompressionManager::LZ4: compCode = 1; break;
                case CompressionManager::ZSTD: compCode = 2; break;
                case CompressionManager::GZIP: compCode = 0; break;
            }
            flags |= (compCode & 0x03) << 2;
        }
        processedContent = EncryptionManager::instance().encryptWithFlags(
            processedContent,
            m_defaultEncAlg,
            flags);
        if (processedContent.isEmpty()) {
            return QByteArray();
        }
    }
    
    return processedContent;
}

QByteArray VFSManager::unprocessContent(const QByteArray &processedContent, bool isEncrypted, bool isCompressed) {
    QByteArray content = processedContent;
    
    // First decrypt if needed (auto-detects compression flag)
    if (isEncrypted) {
        unsigned char flags = 0;
        EncryptionManager::EncryptionAlgorithm alg;
        if (!EncryptionManager::instance().decryptAndGetFlags(processedContent, content, flags, alg)) {
            qWarning() << "VFSManager: Decryption failed";
            return QByteArray();
        }
        // Check if content was compressed (bit 0 of flags)
        bool wasCompressed = (flags & 0x01) != 0;
        if (wasCompressed || isCompressed) {
            // Determine compression algorithm from flags if present
            CompressionManager::CompressionAlgorithm compAlg = CompressionManager::ZLIB;
            if (wasCompressed) {
                unsigned char compCode = (flags >> 2) & 0x03;
                switch (compCode) {
                    case 0: compAlg = CompressionManager::ZLIB; break;
                    case 1: compAlg = CompressionManager::LZ4; break;
                    case 2: compAlg = CompressionManager::ZSTD; break;
                    default: compAlg = CompressionManager::ZLIB; break;
                }
            }
            content = CompressionManager::instance().decompress(content, compAlg);
            if (content.isEmpty()) {
                qWarning() << "VFSManager: Decompression failed";
                return QByteArray();
            }
        }
    } else if (isCompressed) {
        // Not encrypted but compressed
        content = CompressionManager::instance().decompress(content, CompressionManager::ZLIB);
        if (content.isEmpty()) {
            return QByteArray();
        }
    }
    
    return content;
}

bool VFSManager::reprocessFile(int fileId, bool encrypt, bool compress) {
    if (m_currentUserId == -1) return false;
    FileRecord file;
    if (!DatabaseManager::instance().getFile(fileId, file)) return false;
    if (file.userId != m_currentUserId) return false;

    // Obtain current plaintext regardless of existing transformations
    QByteArray plain;
    if (!getFileContent(fileId, plain)) {
        qWarning() << "VFSManager: reprocessFile failed to obtain plaintext";
        return false;
    }

    QByteArray processed = processContent(plain, encrypt, compress);
    if (processed.isEmpty()) return false;

    file.isEncrypted = encrypt;
    file.isCompressed = compress;
    file.size = plain.size();
    file.modifiedAt = QDateTime::currentDateTime();
    file.checksum = EncryptionManager::instance().calculateChecksum(plain);
    if (encrypt) {
        file.encryptedContent = processed;
        file.content.clear();
    } else {
        file.content = processed;
        file.encryptedContent.clear();
    }

    if (DatabaseManager::instance().updateFile(file)) {
        emit fileUpdated(fileId);
        return true;
    }
    return false;
}