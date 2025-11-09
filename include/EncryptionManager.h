


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
#include <vector>
#include <openssl/evp.h>

// Real encryption uses OpenSSL EVP (AES-256-CBC/GCM, ChaCha20-Poly1305)
// File/data format (versioned):
// [MAGIC(6)="SVFENC"] [VER(1)] [ALG(1)] [FLAGS(1)] [IVLEN(1)] [IV(var)] [TAGLEN(1)] [TAG(var GCM/Poly)] [PAYLOAD(var)]
// Magic ensures we can detect format; VER allows future evolution.
// ALG codes: 1=AES-256-CBC,2=AES-256-GCM,3=ChaCha20-Poly1305
// FLAGS bit0=compressed(original), bit1=hasChecksum
// When compressed, decompression happens after decrypt.

class EncryptionManager {

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
    QByteArray encrypt(const QByteArray &data, EncryptionAlgorithm algorithm = AES_256_GCM);
    QByteArray decrypt(const QByteArray &encryptedData, EncryptionAlgorithm algorithm = AES_256_GCM);
    
    // Encryption with flags (e.g., compression indicator)
    QByteArray encryptWithFlags(const QByteArray &data, EncryptionAlgorithm algorithm, unsigned char flags);
    bool decryptAndGetFlags(const QByteArray &encryptedData, QByteArray &plaintext, unsigned char &flags, EncryptionAlgorithm &detectedAlg);
    
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
    QByteArray encryptChaCha20Poly1305(const QByteArray &data);
    QByteArray decryptChaCha20Poly1305(const QByteArray &encryptedData);
    QByteArray evpEncrypt(const QByteArray &data, const EVP_CIPHER *cipher, QByteArray &iv, QByteArray &tag);
    QByteArray evpDecrypt(const QByteArray &enc, const EVP_CIPHER *cipher, const QByteArray &iv, const QByteArray &tag, bool &ok);
};

#endif // ENCRYPTIONMANAGER_H