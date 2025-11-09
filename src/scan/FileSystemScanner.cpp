#include "FileSystemScanner.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QElapsedTimer>
#include <thread>

FileSystemScanner::FileSystemScanner(QObject *parent) : QObject(parent) {}

FileSystemScanner::~FileSystemScanner() {
    cancel();
    // Note: destructor does not own thread here (created in startScan). We'll wait only if joinable
}

void FileSystemScanner::startScan(const QString &rootPath, int batchSize) {
    m_cancelled.store(false);
    // Launch worker in a detached std::thread
    std::thread([this, rootPath, batchSize]() {
        scanRecursive(rootPath, batchSize);
    }).detach();
}

void FileSystemScanner::cancel() {
    m_cancelled.store(true);
}

void FileSystemScanner::scanRecursive(const QString &rootPath, int batchSize) {
    if (m_cancelled.load()) return;
    QDirIterator it(rootPath, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);

    QVector<FSItem> batch;
    batch.reserve(batchSize);
    qint64 totalFiles = 0;
    qint64 totalBytes = 0;
    QElapsedTimer timer; timer.start();

    while (it.hasNext()) {
    if (m_cancelled.load()) break;
        it.next();
        QFileInfo info = it.fileInfo();
        FSItem item;
        item.path = info.absoluteFilePath();
        item.name = info.fileName();
        item.isDir = info.isDir();
        item.size = item.isDir ? 0 : info.size();
        item.modified = info.lastModified();

        batch.push_back(item);
        if (!item.isDir) totalBytes += item.size;
        totalFiles++;

        if (batch.size() >= batchSize) {
            emit batchReady(batch, totalFiles, totalBytes);
            batch.clear();
        }
    }
    if (!batch.isEmpty()) {
        emit batchReady(batch, totalFiles, totalBytes);
    }
    emit finished(totalFiles, totalBytes, timer.elapsed(), m_cancelled.load());
}
