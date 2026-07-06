#include "../Client/client.h"
#include "FilesTransFerer.h"

static void fileTransfer(QFileInfo info, serverSocket *ser) {
    quint16 n = info.size();
    // 上传正文
    QFile file(info.filePath());
    file.open(QIODevice::ReadOnly);
    qint64 sended = 0;
    // 分片读取
    while (sended < n) {
        QByteArray chunk = file.read(qMin(n - sended, 65536LL));  // 每次至多读取64KB
        ser->write(chunk);
        sended += chunk.size();
    }
    // 读完了且完成传输，关闭
    file.close();
}

FilesTransFerer::FilesTransFerer(QObject *parent, Client *cli, serverSocket *ser) {
    localClient = cli;
    targetServer = ser;
}

FilesTransFerer::~FilesTransFerer() {

}

void FilesTransFerer::TransferSharedFile(QDir dir) {
    // 获取文件信息
    QFileInfo info(dir.path());
    QString filename = info.fileName();
    quint16 n = info.size();

    /* =================== 文件上传模块 ================== */
    // 上传文件头
    QString fileMsg = FILE_TRANSFER_REQUEST;
    fileMsg += "000" + filename + INTERUPT + QString::number(info.size()) + INTERUPT;
    targetServer->write(fileMsg.toUtf8());
    fileMsg.clear();

    // 上传文件
    fileTransfer(info, targetServer);
}

void FilesTransFerer::TransferPrivateFile(QDir dir, QString targetClientName) {
    // 获取文件信息
    QFileInfo info(dir.path());
    QString filename = info.fileName();
    quint16 n = info.size();

    /* =================== 文件上传模块 ================== */
    // 上传文件头
    QString fileMsg = FILE_TRANSFER_REQUEST;
    fileMsg += "001" + targetClientName + INTERUPT + filename
            + INTERUPT + QString::number(info.size()) + INTERUPT;
    targetServer->write(fileMsg.toUtf8());
    fileMsg.clear();

    // 上传文件
    fileTransfer(info, targetServer);
}
