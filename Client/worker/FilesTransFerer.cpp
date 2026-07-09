#include "../Client/client.h"
#include "FilesTransFerer.h"

// 发送文件正文 + 结束标记
static void fileTransfer(QFileInfo info, serverSocket *ser) {
    qint64 n = info.size();
    QFile file(info.filePath());
    file.open(QIODevice::ReadOnly);
    qint64 sended = 0;
    while (sended < n) {
        QByteArray chunk = file.read(qMin(n - sended, 65536LL));  // 每次至多读取64KB
        ser->write(chunk);
        sended += chunk.size();
    }
    file.close();
    // 发送结束标记
    ser->write(FILE_TRANSFER_END);
    ser->flush();
}

FilesTransFerer::FilesTransFerer(QObject *parent, Client *cli, serverSocket *ser, QRecursiveMutex* mutex) {
    localClient = cli;
    targetServer = ser;
    socketMutex = mutex;
}

FilesTransFerer::~FilesTransFerer() {

}

void FilesTransFerer::TransferSharedFile(QString filePath) {
    QMutexLocker locker(socketMutex);
    // 获取文件信息
    QFileInfo info(filePath);
    QString filename = info.fileName();
    qint64 n = info.size();

    // A + FT_SHARED_UPLOAD + fileName + INTERUPT + fileSize + INTERUPT + [data] + FILE_TRANSFER_END
    QByteArray header = QByteArray(FILE_TRANSFER_REQUEST) + FT_SHARED_UPLOAD
                      + filename.toUtf8() + INTERUPT
                      + QByteArray::number(n) + INTERUPT;
    targetServer->write(header);

    // 上传文件正文
    fileTransfer(info, targetServer);
}

void FilesTransFerer::TransferPrivateFile(QString filePath, QString targetClientName) {
    QMutexLocker locker(socketMutex);
    // 获取文件信息
    QFileInfo info(filePath);
    QString filename = info.fileName();
    qint64 n = info.size();

    // A + FT_PRIVATE_UPLOAD + targetName + INTERUPT + fileName + INTERUPT + fileSize + INTERUPT + [data] + FILE_TRANSFER_END
    QByteArray header = QByteArray(FILE_TRANSFER_REQUEST) + FT_PRIVATE_UPLOAD
                      + targetClientName.toUtf8() + INTERUPT
                      + filename.toUtf8() + INTERUPT
                      + QByteArray::number(n) + INTERUPT;
    targetServer->write(header);

    // 上传文件正文
    fileTransfer(info, targetServer);
}
