//
// Created by siddh on 31-08-2025.
//
#include "CompressionManager.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QDataStream>
#include <QBuffer>

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
        case ZSTD:
            // Fallback to zlib for now
            return compressZlib(data, level);
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
        case ZSTD:
            // Fallback to zlib for now
            return decompressZlib(compressedData);
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

