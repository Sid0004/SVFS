//
// Created by siddh on 31-08-2025.
//
#ifndef COMPRESSIONMANAGER_H
#define COMPRESSIONMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QString>

class CompressionManager : public QObject {
    Q_OBJECT

public:
    enum CompressionAlgorithm {
        ZLIB,
        GZIP,
        LZ4,
        ZSTD
    };
    
    static CompressionManager& instance();
    
    // Compression/Decompression
    QByteArray compress(const QByteArray &data, CompressionAlgorithm algorithm = ZLIB, int level = 6);
    QByteArray decompress(const QByteArray &compressedData, CompressionAlgorithm algorithm = ZLIB);
    
    // File compression
    bool compressFile(const QString &inputPath, const QString &outputPath, CompressionAlgorithm algorithm = ZLIB, int level = 6);
    bool decompressFile(const QString &inputPath, const QString &outputPath, CompressionAlgorithm algorithm = ZLIB);
    
    // Utility functions
    double getCompressionRatio(const QByteArray &original, const QByteArray &compressed);
    bool isCompressed(const QByteArray &data, CompressionAlgorithm algorithm = ZLIB);
    
    // Algorithm info
    QString getAlgorithmName(CompressionAlgorithm algorithm) const;
    int getMaxCompressionLevel(CompressionAlgorithm algorithm) const;

private:
    CompressionManager() = default;
    ~CompressionManager() = default;
    CompressionManager(const CompressionManager&) = delete;
    CompressionManager& operator=(const CompressionManager&) = delete;
    
    QByteArray compressZlib(const QByteArray &data, int level);
    QByteArray decompressZlib(const QByteArray &compressedData);
    QByteArray compressGzip(const QByteArray &data, int level);
    QByteArray decompressGzip(const QByteArray &compressedData);
};

#endif // COMPRESSIONMANAGER_H


