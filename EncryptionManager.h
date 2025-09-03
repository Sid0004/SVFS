


//
// Created by siddh on 31-08-2025.
//
#ifndef ENCRYPTIONMANAGER_H
#define ENCRYPTIONMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QMap>
#include <memory>

class EncryptionManager : public QObject {
    Q_OBJECT

public:
    enum EncryptionAlgorithm {
        AES_256_CBC,
        AES_256_GCM,
        ChaCha20_Poly1305
    };
    
    static EncryptionManager& instance();
    
    // Key management
    bool generateKey(const QString &password, const QByteArray &salt);
    bool loadKey(const QString &password, const QByteArray &salt);
    void clearKey();
    bool isKeyLoaded() const;
    
    // Encryption/Decryption
    QByteArray encrypt(const QByteArray &data, EncryptionAlgorithm algorithm = AES_256_CBC);
    QByteArray decrypt(const QByteArray &encryptedData, EncryptionAlgorithm algorithm = AES_256_CBC);
    
    // File encryption
    bool encryptFile(const QString &inputPath, const QString &outputPath, EncryptionAlgorithm algorithm = AES_256_CBC);
    bool decryptFile(const QString &inputPath, const QString &outputPath, EncryptionAlgorithm algorithm = AES_256_CBC);
    
    // Utility functions
    QByteArray generateRandomBytes(int size);
    QByteArray calculateChecksum(const QByteArray &data);
    bool verifyChecksum(const QByteArray &data, const QByteArray &checksum);
    
    // Algorithm info
    QString getAlgorithmName(EncryptionAlgorithm algorithm) const;
    int getKeySize(EncryptionAlgorithm algorithm) const;
    int getIVSize(EncryptionAlgorithm algorithm) const;

private:
    EncryptionManager() = default;
    ~EncryptionManager() = default;
    EncryptionManager(const EncryptionManager&) = delete;
    EncryptionManager& operator=(const EncryptionManager&) = delete;
    
    QByteArray m_derivedKey;
    bool m_keyLoaded = false;
    
    QByteArray deriveKey(const QString &password, const QByteArray &salt);
    QByteArray encryptAES256CBC(const QByteArray &data);
    QByteArray decryptAES256CBC(const QByteArray &encryptedData);
    QByteArray encryptAES256GCM(const QByteArray &data);
    QByteArray decryptAES256GCM(const QByteArray &encryptedData);
};

#endif // ENCRYPTIONMANAGER_H