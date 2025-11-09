// Canonical location for FileSystemScanner
#ifndef FILESYSTEMSCANNER_H
#define FILESYSTEMSCANNER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <atomic>
#include <thread>

struct FSItem { QString path; QString name; qint64 size = 0; bool isDir = false; QDateTime modified; };

class FileSystemScanner : public QObject {
    Q_OBJECT
public:
    explicit FileSystemScanner(QObject *parent = nullptr);
    ~FileSystemScanner() override;
    void startScan(const QString &rootPath, int batchSize = 512);
    void cancel();
signals:
    void batchReady(const QVector<FSItem> &items, qint64 totalFiles, qint64 totalBytes);
    void finished(qint64 totalFiles, qint64 totalBytes, qint64 elapsedMs, bool cancelled);
    void error(const QString &message);
private:
    void scanRecursive(const QString &rootPath, int batchSize);
    std::atomic_bool m_cancelled{false};
};

#endif // FILESYSTEMSCANNER_H
