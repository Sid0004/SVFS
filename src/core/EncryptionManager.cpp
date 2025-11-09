// Clean OpenSSL-based implementation of EncryptionManager
#include "EncryptionManager.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QIODevice>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <algorithm>

namespace {
    constexpr const char* MAGIC = "SVFENC"; // 6 bytes
    constexpr unsigned char VERSION = 1;

    // Map ALG code to OpenSSL cipher
    const EVP_CIPHER* cipherForAlgCode(unsigned char algCode) {
        switch (algCode) {
            case 1: return EVP_aes_256_cbc();
            case 2: return EVP_aes_256_gcm();
            case 3: return EVP_chacha20_poly1305();
            default: return nullptr;
        }
    }

    unsigned char algCodeForEnum(EncryptionManager::EncryptionAlgorithm alg) {
        switch (alg) {
            case EncryptionManager::AES_256_CBC: return 1;
            case EncryptionManager::AES_256_GCM: return 2;
            case EncryptionManager::ChaCha20_Poly1305: return 3;
            default: return 1;
        }
    }
}

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
    if (!m_derivedKey.isEmpty()) {
        OPENSSL_cleanse(m_derivedKey.data(), m_derivedKey.size());
    }
    m_derivedKey.clear();
    m_keyLoaded = false;
}

bool EncryptionManager::isKeyLoaded() const {
    return m_keyLoaded;
}

QByteArray EncryptionManager::deriveKey(const QString &password, const QByteArray &salt) {
    // PBKDF2-HMAC-SHA256 with 100k iterations -> 32-byte key
    if (password.isEmpty()) return {};
    QByteArray key(32, 0);
    const QByteArray pass = password.toUtf8();
    const unsigned char* passPtr = reinterpret_cast<const unsigned char*>(pass.constData());
    const unsigned char* saltPtr = reinterpret_cast<const unsigned char*>(salt.constData());
    int ok = PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(passPtr), pass.size(),
                               saltPtr, salt.size(),
                               100000, EVP_sha256(), key.size(),
                               reinterpret_cast<unsigned char*>(key.data()));
    if (!ok) return {};
    return key;
}

QByteArray EncryptionManager::encrypt(const QByteArray &data, EncryptionAlgorithm algorithm) {
    if (!m_keyLoaded) {
        qWarning() << "EncryptionManager: No key loaded";
        return {};
    }
    switch (algorithm) {
        case AES_256_CBC: return encryptAES256CBC(data);
        case AES_256_GCM: return encryptAES256GCM(data);
        case ChaCha20_Poly1305: return encryptChaCha20Poly1305(data);
        default: return encryptAES256CBC(data);
    }
}

QByteArray EncryptionManager::decrypt(const QByteArray &encryptedData, EncryptionAlgorithm /*algorithm*/) {
    if (!m_keyLoaded) {
        qWarning() << "EncryptionManager: No key loaded";
        return {};
    }
    // Auto-detect from header
    if (encryptedData.size() < 10) return {};
    if (QByteArray(encryptedData.constData(), 6) != MAGIC) return {};
    unsigned char algCode = static_cast<unsigned char>(encryptedData[7]);
    switch (algCode) {
        case 1: return decryptAES256CBC(encryptedData);
        case 2: return decryptAES256GCM(encryptedData);
        case 3: return decryptChaCha20Poly1305(encryptedData);
        default: return {};
    }
}

QByteArray EncryptionManager::encryptAES256CBC(const QByteArray &data) {
    QByteArray iv(16, 0);
    RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size());
    QByteArray tag; // none for CBC
    QByteArray payload = evpEncrypt(data, EVP_aes_256_cbc(), iv, tag);
    if (payload.isEmpty()) return {};
    QByteArray out;
    out.append(MAGIC);
    out.append(char(VERSION));
    out.append(char(1)); // alg code
    out.append(char(0)); // flags
    out.append(char(iv.size()));
    out.append(iv);
    out.append(char(0)); // tag length
    out.append(payload);
    return out;
}

QByteArray EncryptionManager::decryptAES256CBC(const QByteArray &encryptedData) {
    if (encryptedData.size() < 10) return {};
    if (QByteArray(encryptedData.constData(), 6) != MAGIC) return {};
    int ivLen = static_cast<unsigned char>(encryptedData[9]);
    int offset = 10;
    if (encryptedData.size() < offset + ivLen + 1) return {};
    QByteArray iv = encryptedData.mid(offset, ivLen);
    offset += ivLen;
    int tagLen = static_cast<unsigned char>(encryptedData[offset++]); Q_UNUSED(tagLen);
    QByteArray payload = encryptedData.mid(offset);
    bool ok = false;
    QByteArray plain = evpDecrypt(payload, EVP_aes_256_cbc(), iv, {}, ok);
    return ok ? plain : QByteArray();
}

QByteArray EncryptionManager::encryptAES256GCM(const QByteArray &data) {
    QByteArray iv(12, 0);
    RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size());
    QByteArray tag;
    QByteArray payload = evpEncrypt(data, EVP_aes_256_gcm(), iv, tag);
    if (payload.isEmpty() || tag.isEmpty()) return {};
    QByteArray out;
    out.append(MAGIC);
    out.append(char(VERSION));
    out.append(char(2)); // alg code
    out.append(char(0)); // flags
    out.append(char(iv.size()));
    out.append(iv);
    out.append(char(tag.size()));
    out.append(tag);
    out.append(payload);
    return out;
}

QByteArray EncryptionManager::decryptAES256GCM(const QByteArray &encryptedData) {
    if (encryptedData.size() < 10) return {};
    if (QByteArray(encryptedData.constData(), 6) != MAGIC) return {};
    int ivLen = static_cast<unsigned char>(encryptedData[9]);
    int offset = 10;
    if (encryptedData.size() < offset + ivLen + 1) return {};
    QByteArray iv = encryptedData.mid(offset, ivLen);
    offset += ivLen;
    int tagLen = static_cast<unsigned char>(encryptedData[offset++]);
    if (encryptedData.size() < offset + tagLen) return {};
    QByteArray tag = encryptedData.mid(offset, tagLen);
    offset += tagLen;
    QByteArray payload = encryptedData.mid(offset);
    bool ok = false;
    QByteArray plain = evpDecrypt(payload, EVP_aes_256_gcm(), iv, tag, ok);
    return ok ? plain : QByteArray();
}

QByteArray EncryptionManager::encryptChaCha20Poly1305(const QByteArray &data) {
    QByteArray iv(12, 0);
    RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size());
    QByteArray tag;
    QByteArray payload = evpEncrypt(data, EVP_chacha20_poly1305(), iv, tag);
    if (payload.isEmpty() || tag.isEmpty()) return {};
    QByteArray out;
    out.append(MAGIC);
    out.append(char(VERSION));
    out.append(char(3)); // alg code
    out.append(char(0)); // flags
    out.append(char(iv.size()));
    out.append(iv);
    out.append(char(tag.size()));
    out.append(tag);
    out.append(payload);
    return out;
}

QByteArray EncryptionManager::decryptChaCha20Poly1305(const QByteArray &encryptedData) {
    if (encryptedData.size() < 10) return {};
    if (QByteArray(encryptedData.constData(), 6) != MAGIC) return {};
    int ivLen = static_cast<unsigned char>(encryptedData[9]);
    int offset = 10;
    if (encryptedData.size() < offset + ivLen + 1) return {};
    QByteArray iv = encryptedData.mid(offset, ivLen);
    offset += ivLen;
    int tagLen = static_cast<unsigned char>(encryptedData[offset++]);
    if (encryptedData.size() < offset + tagLen) return {};
    QByteArray tag = encryptedData.mid(offset, tagLen);
    offset += tagLen;
    QByteArray payload = encryptedData.mid(offset);
    bool ok = false;
    QByteArray plain = evpDecrypt(payload, EVP_chacha20_poly1305(), iv, tag, ok);
    return ok ? plain : QByteArray();
}

QByteArray EncryptionManager::generateRandomBytes(int size) {
    QByteArray bytes(size, 0);
    if (size <= 0) return bytes;
    RAND_bytes(reinterpret_cast<unsigned char*>(bytes.data()), bytes.size());
    return bytes;
}

QByteArray EncryptionManager::calculateChecksum(const QByteArray &data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

bool EncryptionManager::verifyChecksum(const QByteArray &data, const QByteArray &checksum) {
    return calculateChecksum(data) == checksum;
}

bool EncryptionManager::encryptFile(const QString &inputPath, const QString &outputPath, EncryptionAlgorithm algorithm) {
    QFile in(inputPath);
    if (!in.open(QIODevice::ReadOnly)) return false;
    QByteArray plain = in.readAll();
    in.close();
    QByteArray enc = encrypt(plain, algorithm);
    if (enc.isEmpty()) return false;
    QFile out(outputPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    qint64 n = out.write(enc);
    out.close();
    return n == enc.size();
}

bool EncryptionManager::decryptFile(const QString &inputPath, const QString &outputPath, EncryptionAlgorithm /*algorithm*/) {
    QFile in(inputPath);
    if (!in.open(QIODevice::ReadOnly)) return false;
    QByteArray enc = in.readAll();
    in.close();
    QByteArray plain = decrypt(enc, AES_256_CBC); // alg autodetected in decrypt()
    if (plain.isEmpty()) return false;
    QFile out(outputPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    qint64 n = out.write(plain);
    out.close();
    return n == plain.size();
}

QString EncryptionManager::getAlgorithmName(EncryptionAlgorithm algorithm) const {
    switch (algorithm) {
        case AES_256_CBC: return "AES-256-CBC";
        case AES_256_GCM: return "AES-256-GCM";
        case ChaCha20_Poly1305: return "ChaCha20-Poly1305";
        default: return "Unknown";
    }
}

int EncryptionManager::getKeySize(EncryptionAlgorithm) const {
    // 256-bit for all supported algorithms
    return 32;
}

int EncryptionManager::getIVSize(EncryptionAlgorithm algorithm) const {
    switch (algorithm) {
        case AES_256_CBC: return 16; // 128-bit IV
        case AES_256_GCM: return 12; // recommended 96-bit nonce for GCM
        case ChaCha20_Poly1305: return 12; // 96-bit nonce
        default: return 16;
    }
}

QByteArray EncryptionManager::encryptWithFlags(const QByteArray &data, EncryptionAlgorithm algorithm, unsigned char flags) {
    if (!m_keyLoaded) {
        qWarning() << "EncryptionManager: No key loaded";
        return {};
    }
    
    // Build header with custom flags
    const EVP_CIPHER *cipher = nullptr;
    int ivSize = 12;
    unsigned char algCode = 0;
    
    switch (algorithm) {
        case AES_256_CBC:
            cipher = EVP_aes_256_cbc();
            ivSize = 16;
            algCode = 1;
            break;
        case AES_256_GCM:
            cipher = EVP_aes_256_gcm();
            ivSize = 12;
            algCode = 2;
            break;
        case ChaCha20_Poly1305:
            cipher = EVP_chacha20_poly1305();
            ivSize = 12;
            algCode = 3;
            break;
        default:
            cipher = EVP_aes_256_gcm();
            algCode = 2;
            ivSize = 12;
    }
    
    QByteArray iv(ivSize, 0);
    RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size());
    QByteArray tag;
    QByteArray payload = evpEncrypt(data, cipher, iv, tag);
    if (payload.isEmpty()) return {};
    
    QByteArray out;
    out.append("SVFENC");           // magic (6)
    out.append(char(1));            // version
    out.append(char(algCode));      // algorithm
    out.append(char(flags));        // FLAGS (bit 0 = compressed)
    out.append(char(iv.size()));
    out.append(iv);
    out.append(char(tag.size()));
    if (!tag.isEmpty()) out.append(tag);
    out.append(payload);
    return out;
}

bool EncryptionManager::decryptAndGetFlags(const QByteArray &encryptedData, QByteArray &plaintext, unsigned char &flags, EncryptionAlgorithm &detectedAlg) {
    plaintext.clear();
    flags = 0;
    detectedAlg = AES_256_GCM;
    
    if (!m_keyLoaded) {
        qWarning() << "EncryptionManager: No key loaded";
        return false;
    }
    if (encryptedData.size() < 10) return false;
    if (QByteArray(encryptedData.constData(), 6) != "SVFENC") return false;
    
    const char *p = encryptedData.constData();
    // unsigned char ver = p[6]; // unused for now
    unsigned char algCode = static_cast<unsigned char>(p[7]);
    flags = static_cast<unsigned char>(p[8]);
    int ivLen = static_cast<unsigned char>(p[9]);
    int offset = 10;
    
    if (encryptedData.size() < offset + ivLen + 1) return false;
    QByteArray iv = encryptedData.mid(offset, ivLen);
    offset += ivLen;
    
    int tagLen = static_cast<unsigned char>(encryptedData[offset++]);
    QByteArray tag;
    if (tagLen > 0) {
        if (encryptedData.size() < offset + tagLen) return false;
        tag = encryptedData.mid(offset, tagLen);
        offset += tagLen;
    }
    
    QByteArray payload = encryptedData.mid(offset);
    
    const EVP_CIPHER *cipher = nullptr;
    switch (algCode) {
        case 1:
            cipher = EVP_aes_256_cbc();
            detectedAlg = AES_256_CBC;
            break;
        case 2:
            cipher = EVP_aes_256_gcm();
            detectedAlg = AES_256_GCM;
            break;
        case 3:
            cipher = EVP_chacha20_poly1305();
            detectedAlg = ChaCha20_Poly1305;
            break;
        default:
            return false;
    }
    
    bool ok = false;
    plaintext = evpDecrypt(payload, cipher, iv, tag, ok);
    return ok && !plaintext.isEmpty();
}

QByteArray EncryptionManager::evpEncrypt(const QByteArray &data, const EVP_CIPHER *cipher, QByteArray &iv, QByteArray &tag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};
    QByteArray key = m_derivedKey.left(EVP_CIPHER_key_length(cipher));
    if (iv.isEmpty()) {
        iv.resize(EVP_CIPHER_iv_length(cipher));
        if (iv.size() > 0) RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size());
    }
    if (!EVP_EncryptInit_ex(ctx, cipher, nullptr,
                            reinterpret_cast<const unsigned char*>(key.constData()),
                            reinterpret_cast<const unsigned char*>(iv.constData()))) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    QByteArray out;
    out.resize(int(data.size()) + EVP_CIPHER_block_size(cipher));
    int outLen1 = 0;
    if (!EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(out.data()), &outLen1,
                           reinterpret_cast<const unsigned char*>(data.constData()), data.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int outLen2 = 0;
    if (!EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(out.data() + outLen1), &outLen2)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    out.resize(outLen1 + outLen2);

    // Fetch tag for AEAD ciphers
    if (EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE || cipher == EVP_chacha20_poly1305()) {
        tag.resize(16);
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data())) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
    }
    EVP_CIPHER_CTX_free(ctx);
    return out;
}

QByteArray EncryptionManager::evpDecrypt(const QByteArray &enc, const EVP_CIPHER *cipher, const QByteArray &iv, const QByteArray &tag, bool &okOut) {
    okOut = false;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};
    QByteArray key = m_derivedKey.left(EVP_CIPHER_key_length(cipher));
    if (!EVP_DecryptInit_ex(ctx, cipher, nullptr,
                            reinterpret_cast<const unsigned char*>(key.constData()),
                            reinterpret_cast<const unsigned char*>(iv.constData()))) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    // For AEAD, set expected tag before final
    if ((EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE || cipher == EVP_chacha20_poly1305()) && !tag.isEmpty()) {
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(), const_cast<char*>(tag.constData()))) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
    }

    QByteArray out;
    out.resize(enc.size());
    int outLen1 = 0;
    if (!EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(out.data()), &outLen1,
                           reinterpret_cast<const unsigned char*>(enc.constData()), enc.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int outLen2 = 0;
    if (!EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(out.data() + outLen1), &outLen2)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    out.resize(outLen1 + outLen2);
    EVP_CIPHER_CTX_free(ctx);
    okOut = true;
    return out;
}


