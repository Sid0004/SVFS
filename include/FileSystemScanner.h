//
// Safe, read-only file system scanner for WizTree-like listing
//
#ifndef FILESYSTEMSCANNER_H
#define FILESYSTEMSCANNER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QElapsedTimer>
#include <atomic>
#include <thread>

struct FSItem {
    QString path;
    QString name;
    qint64 size = 0;
    bool isDir = false;
    QDateTime modified;
};

class FileSystemScanner : public QObject {
    Q_OBJECT
public:
    explicit FileSystemScanner(QObject *parent = nullptr);
    virtual ~FileSystemScanner();

    // Start async scan. Emits signals with batches and final summary.
    void startScan(const QString &rootPath, int batchSize = 512);
    void cancel();

signals:
    void batchReady(const QVector<FSItem> &items, qint64 totalFiles, qint64 totalBytes);
    void finished(qint64 totalFiles, qint64 totalBytes, qint64 elapsedMs, bool cancelled);
    void error(const QString &message);

private:
    void scanRecursive(const QString &rootPath, int batchSize);

    std::atomic_bool m_cancelled{false};
    std::thread m_thread; // worker thread for scan
};

#endif // FILESYSTEMSCANNER_H
