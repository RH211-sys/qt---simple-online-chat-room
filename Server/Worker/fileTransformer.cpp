//
//
//

#include "fileTransformer.h"

static QByteArray readUntil(QTcpSocket* cli, QString _flag) {
    QByteArray flag = _flag.toUtf8();
    QByteArray res;
    char ch;
    while (cli->getChar(&ch)) {
        if (ch == flag[0]) break;
        res.append(ch);
    }
    return res;
}

FileTransformer::FileTransformer(QObject *parent, Server *ser, QTcpSocket *cli) {
    server = ser;
    client = cli;
    name = cli->peerName();
    downloadFileDepot = QDir(downloadFilePath);
}

FileTransformer::~FileTransformer() {

}

void FileTransformer::doReceiveFile(QTcpSocket* cli) {
    if(cli != client) return;
    // 获取传送方式(shared还是private)[阅读3位：保留拓展性]
    QString transferWay = QString::fromUtf8(client->read(3));
    if(transferWay == "001") {
        // 需要获取私发的用户名
        QString name = QString::fromUtf8(readUntil(client, INTERUPT));
        auto it = server->name_to_ip.find(name);
        if(it == server->name_to_ip.end()) {
            // 未找到该用户
            server->tellPointedUser("errName", cli, FILE_TRANSFER_RESULT);
            // 获取发来的文件名和文件大小
            QString filename = QString::fromUtf8(readUntil(client, INTERUPT));
            long long n = readUntil(client, INTERUPT).toLongLong();
            qint64 received = 0;
            // 分片读取
            while (received < n) {
                QByteArray chunk = client->read(qMin(n - received, 65536LL));  // 每次64KB
                received += chunk.size();
            }
            return;
        }
    }


    // 获取发来的文件名和文件大小
    QString filename = QString::fromUtf8(readUntil(client, INTERUPT));
    int name_size = filename.size();
    long long n = readUntil(client, INTERUPT).toLongLong();
    // 创建下载文件的文件名(用用户名的方式创建可以确保重写覆盖，节约空间)
    filename +='(' + filename + ')';
    // 创建文件，然后开始下载
    QFile file(downloadFilePath + filename);
    file.open(QIODevice::WriteOnly);
    qint64 received = 0;
    // 分片读取
    while (received < n) {
        QByteArray chunk = client->read(qMin(n - received, 65536LL));  // 每次64KB
        file.write(chunk);
        received += chunk.size();
    }
    // 读完了，关闭
    file.close();

    // 接着发送信号
    if(transferWay == "000") {
        // 文件共享
        emit addNewSharedFile(client, filename);
    } else if(transferWay == "001") {
        // 文件私发
        emit addNewPrivateFile(client, name, filename);
    }

}

void FileTransformer::doTransfer(const char *dir, QTcpSocket *cli) {

}
