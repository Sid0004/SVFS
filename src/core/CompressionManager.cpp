//
// Created by siddh on 31-08-2025.
//
#include "CompressionManager.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QDataStream>
#include <QBuffer>

#ifdef HAVE_LZ4
#include <lz4.h>
#include <lz4frame.h>
#endif
#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

CompressionManager& CompressionManager::instance() {
    static CompressionManager instance;
    return instance;
}

QByteArray CompressionManager::compress(const QByteArray &data, CompressionAlgorithm algorithm, int level) {
    switch (algorithm) {
        case ZLIB:
            return compressZlib(data, level);
        case GZIP:
            return compressGzip(data, level);
        case LZ4:
        {
#ifdef HAVE_LZ4
            if (!data.isEmpty()) {
                // Use LZ4 frame API to embed size for robust decode
                LZ4F_preferences_t prefs{};
                prefs.compressionLevel = level;
                size_t bound = LZ4F_compressFrameBound(static_cast<size_t>(data.size()), &prefs);
                QByteArray out;
                out.resize(static_cast<int>(bound));
                size_t written = LZ4F_compressFrame(out.data(), bound, data.constData(), static_cast<size_t>(data.size()), &prefs);
                if (LZ4F_isError(written)) return QByteArray();
                out.resize(static_cast<int>(written));
                return out;
            }
#endif
            return compressZlib(data, level);
        }
        case ZSTD:
        {
#ifdef HAVE_ZSTD
            if (!data.isEmpty()) {
                int lvl = qBound(1, level, 22);
                size_t bound = ZSTD_compressBound(static_cast<size_t>(data.size()));
                QByteArray out;
                out.resize(static_cast<int>(bound));
                size_t written = ZSTD_compress(out.data(), bound, data.constData(), static_cast<size_t>(data.size()), lvl);
                if (ZSTD_isError(written)) return QByteArray();
                out.resize(static_cast<int>(written));
                return out;
            }
#endif
            return compressZlib(data, level);
        }
        default:
            return compressZlib(data, level);
    }
}

QByteArray CompressionManager::decompress(const QByteArray &compressedData, CompressionAlgorithm algorithm) {
    switch (algorithm) {
        case ZLIB:
            return decompressZlib(compressedData);
        case GZIP:
            return decompressGzip(compressedData);
        case LZ4:
        {
#ifdef HAVE_LZ4
            if (compressedData.isEmpty()) return QByteArray();
            LZ4F_decompressionContext_t dctx;
            if (LZ4F_isError(LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION))) return QByteArray();
            QByteArray out; out.reserve(compressedData.size() * 3); // heuristic
            const char *src = compressedData.constData();
            size_t srcSize = static_cast<size_t>(compressedData.size());
            size_t srcPos = 0;
            char buf[64 * 1024];
            while (srcPos < srcSize) {
                size_t inSize = srcSize - srcPos;
                size_t outSize = sizeof(buf);
                size_t ret = LZ4F_decompress(dctx, buf, &outSize, src + srcPos, &inSize, nullptr);
                if (LZ4F_isError(ret)) { LZ4F_freeDecompressionContext(dctx); return QByteArray(); }
                srcPos += inSize;
                if (outSize > 0) out.append(buf, static_cast<int>(outSize));
                if (ret == 0) break; // done
            }
            LZ4F_freeDecompressionContext(dctx);
            return out;
#else
            return decompressZlib(compressedData);
#endif
        }
        case ZSTD:
        {
#ifdef HAVE_ZSTD
            if (compressedData.isEmpty()) return QByteArray();
            size_t contentSize = ZSTD_getFrameContentSize(compressedData.constData(), static_cast<size_t>(compressedData.size()));
            if (contentSize == ZSTD_CONTENTSIZE_ERROR) return QByteArray();
            if (contentSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                // Fallback to streaming
                ZSTD_DStream *dstream = ZSTD_createDStream();
                if (!dstream) return QByteArray();
                size_t initRes = ZSTD_initDStream(dstream);
                if (ZSTD_isError(initRes)) { ZSTD_freeDStream(dstream); return QByteArray(); }
                QByteArray out; out.reserve(compressedData.size() * 3);
                ZSTD_inBuffer in = { compressedData.constData(), static_cast<size_t>(compressedData.size()), 0 };
                QByteArray chunk; chunk.resize(64 * 1024);
                while (in.pos < in.size) {
                    ZSTD_outBuffer outBuf = { chunk.data(), static_cast<size_t>(chunk.size()), 0 };
                    size_t res = ZSTD_decompressStream(dstream, &outBuf, &in);
                    if (ZSTD_isError(res)) { ZSTD_freeDStream(dstream); return QByteArray(); }
                    if (outBuf.pos > 0) out.append(chunk.constData(), static_cast<int>(outBuf.pos));
                }
                ZSTD_freeDStream(dstream);
                return out;
            } else {
                QByteArray out; out.resize(static_cast<int>(contentSize));
                size_t res = ZSTD_decompress(out.data(), contentSize, compressedData.constData(), static_cast<size_t>(compressedData.size()));
                if (ZSTD_isError(res)) return QByteArray();
                return out;
            }
#else
            return decompressZlib(compressedData);
#endif
        }
        default:
            return decompressZlib(compressedData);
    }
}

QByteArray CompressionManager::compressZlib(const QByteArray &data, int level) {
    if (data.isEmpty()) return QByteArray();
    
    QByteArray compressed = qCompress(data, level);
    return compressed;
}

QByteArray CompressionManager::decompressZlib(const QByteArray &compressedData) {
    if (compressedData.isEmpty()) return QByteArray();
    
    QByteArray decompressed = qUncompress(compressedData);
    return decompressed;
}

QByteArray CompressionManager::compressGzip(const QByteArray &data, int level) {
    // Simplified gzip implementation using Qt's compression
    return compressZlib(data, level);
}

QByteArray CompressionManager::decompressGzip(const QByteArray &compressedData) {
    // Simplified gzip implementation using Qt's compression
    return decompressZlib(compressedData);
}

bool CompressionManager::compressFile(const QString &inputPath, const QString &outputPath, CompressionAlgorithm algorithm, int level) {
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = inputFile.readAll();
    inputFile.close();
    
    QByteArray compressedData = compress(data, algorithm, level);
    if (compressedData.isEmpty()) {
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    qint64 bytesWritten = outputFile.write(compressedData);
    outputFile.close();
    
    return bytesWritten == compressedData.size();
}

bool CompressionManager::decompressFile(const QString &inputPath, const QString &outputPath, CompressionAlgorithm algorithm) {
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray compressedData = inputFile.readAll();
    inputFile.close();
    
    QByteArray decompressedData = decompress(compressedData, algorithm);
    if (decompressedData.isEmpty()) {
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    qint64 bytesWritten = outputFile.write(decompressedData);
    outputFile.close();
    
    return bytesWritten == decompressedData.size();
}

double CompressionManager::getCompressionRatio(const QByteArray &original, const QByteArray &compressed) {
    if (original.isEmpty()) return 0.0;
    return static_cast<double>(compressed.size()) / static_cast<double>(original.size());
}

bool CompressionManager::isCompressed(const QByteArray &data, CompressionAlgorithm algorithm) {
    if (data.size() < 2) return false;
    
    switch (algorithm) {
        case ZLIB:
            return (data[0] == 0x78 && (data[1] == 0x01 || data[1] == 0x5e || data[1] == 0x9c || data[1] == 0xda));
        case GZIP:
            return (data[0] == 0x1f && data[1] == 0x8b);
        default:
            return false;
    }
}

QString CompressionManager::getAlgorithmName(CompressionAlgorithm algorithm) const {
    switch (algorithm) {
        case ZLIB: return "ZLIB";
        case GZIP: return "GZIP";
        case LZ4: return "LZ4";
        case ZSTD: return "ZSTD";
        default: return "Unknown";
    }
}

int CompressionManager::getMaxCompressionLevel(CompressionAlgorithm algorithm) const {
    switch (algorithm) {
        case ZLIB:
        case GZIP:
            return 9;
        case LZ4:
            return 12;
        case ZSTD:
            return 22;
        default:
            return 9;
    }
}

