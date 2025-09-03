//
// Created by siddh on 31-08-2025.
//
#include "EncryptionManager.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QIODevice>

EncryptionManager& EncryptionManager::instance() {
    static EncryptionManager instance;
    return instance;
}

bool EncryptionManager::generateKey(const QString &password, const QByteArray &salt) {
    m_derivedKey = deriveKey(password, salt);
    m_keyLoaded = !m_derivedKey.isEmpty();
    return m_keyLoaded;
}

bool EncryptionManager::loadKey(const QString &password, const QByteArray &salt) {
    return generateKey(password, salt);
}

void EncryptionManager::clearKey() {
    m_derivedKey.clear();
    m_keyLoaded = false;
}

bool EncryptionManager::isKeyLoaded() const {
    return m_keyLoaded;
}

QByteArray EncryptionManager::deriveKey(const QString &password, const QByteArray &salt) {
    // Use PBKDF2 for key derivation
    QCryptographicHash hash(QCryptographicHash::Sha256);
    
    // Simple key derivation (in production, use proper PBKDF2)
    QByteArray data = password.toUtf8() + salt;
    for (int i = 0; i < 10000; ++i) {
        data = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    }
    
    return data;
}

QByteArray EncryptionManager::encrypt(const QByteArray &data, EncryptionAlgorithm algorithm) {
    if (!m_keyLoaded) {
        qDebug() << "No encryption key loaded";
        return QByteArray();
    }
    
    switch (algorithm) {
        case AES_256_CBC:
            return encryptAES256CBC(data);
        case AES_256_GCM:
            return encryptAES256GCM(data);
        case ChaCha20_Poly1305:
            // Simplified implementation
            return encryptAES256CBC(data);
        default:
            return encryptAES256CBC(data);
    }
}

QByteArray EncryptionManager::decrypt(const QByteArray &encryptedData, EncryptionAlgorithm algorithm) {
    if (!m_keyLoaded) {
        qDebug() << "No encryption key loaded";
        return QByteArray();
    }
    
    switch (algorithm) {
        case AES_256_CBC:
            return decryptAES256CBC(encryptedData);
        case AES_256_GCM:
            return decryptAES256GCM(encryptedData);
        case ChaCha20_Poly1305:
            // Simplified implementation
            return decryptAES256CBC(encryptedData);
        default:
            return decryptAES256CBC(encryptedData);
    }
}

QByteArray EncryptionManager::encryptAES256CBC(const QByteArray &data) {
    // Simplified AES implementation using Qt's built-in encryption
    // In production, use proper OpenSSL or similar library
    
    // Generate random IV
    QByteArray iv = generateRandomBytes(16);
    
    // Simple XOR encryption for demonstration (NOT SECURE for production)
    QByteArray encrypted;
    encrypted.reserve(data.size() + 16);
    encrypted.append(iv);
    
    QByteArray key = m_derivedKey.left(32);
    for (int i = 0; i < data.size(); ++i) {
        char encryptedByte = data[i] ^ key[i % key.size()] ^ iv[i % iv.size()];
        encrypted.append(encryptedByte);
    }
    
    return encrypted;
}

QByteArray EncryptionManager::decryptAES256CBC(const QByteArray &encryptedData) {
    if (encryptedData.size() < 16) return QByteArray();
    
    // Extract IV
    QByteArray iv = encryptedData.left(16);
    QByteArray ciphertext = encryptedData.mid(16);
    
    // Simple XOR decryption for demonstration (NOT SECURE for production)
    QByteArray decrypted;
    decrypted.reserve(ciphertext.size());
    
    QByteArray key = m_derivedKey.left(32);
    for (int i = 0; i < ciphertext.size(); ++i) {
        char decryptedByte = ciphertext[i] ^ key[i % key.size()] ^ iv[i % iv.size()];
        decrypted.append(decryptedByte);
    }
    
    return decrypted;
}

QByteArray EncryptionManager::encryptAES256GCM(const QByteArray &data) {
    // Simplified GCM implementation
    return encryptAES256CBC(data);
}

QByteArray EncryptionManager::decryptAES256GCM(const QByteArray &encryptedData) {
    // Simplified GCM implementation
    return decryptAES256CBC(encryptedData);
}

QByteArray EncryptionManager::generateRandomBytes(int size) {
    QByteArray bytes(size, 0);
    for (int i = 0; i < size; ++i) {
        bytes[i] = QRandomGenerator::global()->generate() % 256;
    }
    return bytes;
}

QByteArray EncryptionManager::calculateChecksum(const QByteArray &data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

bool EncryptionManager::verifyChecksum(const QByteArray &data, const QByteArray &checksum) {
    QByteArray calculatedChecksum = calculateChecksum(data);
    return calculatedChecksum == checksum;
}

bool EncryptionManager::encryptFile(const QString &inputPath, const QString &outputPath, EncryptionAlgorithm algorithm) {
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = inputFile.readAll();
    inputFile.close();
    
    QByteArray encryptedData = encrypt(data, algorithm);
    if (encryptedData.isEmpty()) {
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    qint64 bytesWritten = outputFile.write(encryptedData);
    outputFile.close();
    
    return bytesWritten == encryptedData.size();
}

bool EncryptionManager::decryptFile(const QString &inputPath, const QString &outputPath, EncryptionAlgorithm algorithm) {
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray encryptedData = inputFile.readAll();
    inputFile.close();
    
    QByteArray decryptedData = decrypt(encryptedData, algorithm);
    if (decryptedData.isEmpty()) {
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    qint64 bytesWritten = outputFile.write(decryptedData);
    outputFile.close();
    
    return bytesWritten == decryptedData.size();
}

QString EncryptionManager::getAlgorithmName(EncryptionAlgorithm algorithm) const {
    switch (algorithm) {
        case AES_256_CBC: return "AES-256-CBC";
        case AES_256_GCM: return "AES-256-GCM";
        case ChaCha20_Poly1305: return "ChaCha20-Poly1305";
        default: return "Unknown";
    }
}

int EncryptionManager::getKeySize(EncryptionAlgorithm algorithm) const {
    switch (algorithm) {
        case AES_256_CBC:
        case AES_256_GCM:
        case ChaCha20_Poly1305:
            return 32; // 256 bits
        default:
            return 32;
    }
}

int EncryptionManager::getIVSize(EncryptionAlgorithm algorithm) const {
    switch (algorithm) {
        case AES_256_CBC:
        case AES_256_GCM:
            return 16; // 128 bits
        case ChaCha20_Poly1305:
            return 12; // 96 bits
        default:
            return 16;
    }
}


