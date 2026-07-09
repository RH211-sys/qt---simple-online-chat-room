//
//
//

#include "fileTransformer.h"

// 读取直到遇到分隔符的第一个字节（消费该分隔符）
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

FileTransformer::FileTransformer(QObject *parent, Server *ser, QTcpSocket *cli, QMutex* mutex) {
    server = ser;
    client = cli;
    socketMutex = mutex;
    downloadFileDepot = QDir(downloadFilePath);
}

FileTransformer::~FileTransformer() {

}

// 将接收到的文件数据保存到磁盘，返回服务端存储的文件名
QString FileTransformer::saveFile(QTcpSocket* cli, const QString& originalName, qint64 fileSize) {
    // 用客户端名作为前缀，避免重名文件互相覆盖
    QString savedName = server->client_name[cli] + '-' + originalName;
    QFile file(downloadFilePath + savedName);
    file.open(QIODevice::WriteOnly);

    qint64 received = 0;
    while (received < fileSize) {
        QByteArray chunk = cli->read(qMin(fileSize - received, 65536LL));       // 64KB读取
        file.write(chunk);
        received += chunk.size();
    }
    file.close();

    // 消费 FILE_TRANSFER_END
    cli->read(1);

    return savedName;
}

void FileTransformer::doReceiveFile(QTcpSocket* cli) {
    server->writeLog("doReceiveFile subType: " + QString::fromUtf8(client->peek(4)));
    QString subType;
    if(cli != client) return;

    QMutexLocker locker(socketMutex);
    cli->read(1);
    // 读取子类型（3字节）
    subType = QString::fromUtf8(client->read(3));

    /* ============== 文件上传模块 =============== */
    if (subType == FT_SHARED_UPLOAD) {
        // "A" + "000" + fileName + INTERUPT + fileSize + INTERUPT + [file data] + FILE_TRANSFER_END
        QString fileName = QString::fromUtf8(readUntil(client, INTERUPT));
        qint64 fileSize = readUntil(client, INTERUPT).toLongLong();

        QString savedName = saveFile(client, fileName, fileSize);
        emit addNewSharedFile(client, savedName);

    } else if (subType == FT_PRIVATE_UPLOAD) {
        // "A" + "001" + targetName + INTERUPT + fileName + INTERUPT + fileSize + INTERUPT + [file data] + FILE_TRANSFER_END
        QString targetName = QString::fromUtf8(readUntil(client, INTERUPT));
        QString fileName = QString::fromUtf8(readUntil(client, INTERUPT));
        qint64 fileSize = readUntil(client, INTERUPT).toLongLong();

        // 双重保险：检查目标用户是否在线（客户端已做判断）
        auto it = server->name_to_ip.find(targetName);
        if (it == server->name_to_ip.end()) {
            // 目标不存在，消费掉文件数据然后返回错误
            qint64 received = 0;
            while (received < fileSize) {
                QByteArray chunk = cli->read(qMin(fileSize - received, 65536LL));
                received += chunk.size();
            }
            cli->read(1);  // FILE_TRANSFER_END
            // 通知发送者失败
            QByteArray failAck = QByteArray(FILE_TRANSFER_RESULT) + FT_ACK_SENDER
                               + "001fail" + FILE_TRANSFER_END;
            cli->write(failAck);
            client->flush();
            return;
        }

        QString savedName = saveFile(client, fileName, fileSize);
        emit addNewPrivateFile(client, targetName, savedName);

    /* ============== 文件查询模块 =============== */
    } else if (subType == FT_QUERY_FILES) {
        // "A" + "002" + FILE_TRANSFER_END，消费尾部结束标记
        cli->read(1);
        QByteArray fileListBody;
        for (auto& x : server->sharedFiles) {
            fileListBody += x.toUtf8() + INTERUPT;
        }
        for (auto& x : server->privateFiles[cli]) {
            fileListBody += x.toUtf8() + INTERUPT;
        }

        if (fileListBody.isEmpty()) {
            // 无可下载文件
            QString res = FILE_TRANSFER_RESULT;
            res += FT_QUERY_FAIL;
            QByteArray response = res.toUtf8();
            quint16 ok = client->write(response);
            quint16 check = -1;
            if(ok == check) {
                // write函数错误返回
                server->writeLog("无法向客户端写socket");
            } else {
                server->writeLog("成功响应");
            }
            client->flush();

        } else {
            // B + "997" + bodySize + INTERUPT + body{fileName + INTERUPT...}
            QByteArray response = QByteArray(FILE_TRANSFER_RESULT) + FT_QUERY_SUCCESS
                                + QByteArray::number(fileListBody.size()) + INTERUPT
                                + fileListBody;
            client->write(response);
            client->flush();
        }

    /* ============== 文件下载模块 =============== */
    } else if (subType == FT_DOWNLOAD_REQ) {
        // "A" + "003" + fileName + FILE_TRANSFER_END
        QString fileName = QString::fromUtf8(readUntil(client, FILE_TRANSFER_END));

        if (server->sharedFiles.contains(fileName) || server->privateFiles[cli].contains(fileName)) {
            doTransfer(fileName);

        } else {    // 一般不会进这个分支，因为客户端先前会向服务器查询可下载文件
            server->writeLog("客户端 " + server->client_name[cli] + " 所需要的文件不存在");
            QByteArray response = QByteArray(FILE_TRANSFER_RESULT) + FT_QUERY_FAIL;
            client->write(response);
            client->flush();
        }
    }
}

void FileTransformer::doTransfer(QString filename) {
    // 获取本地文件信息
    QFileInfo info(downloadFilePath + filename);
    qint64 fileSize = info.size();

    // 发送文件头：B + "994" + fileName + INTERUPT + fileSize + INTERUPT
    QByteArray header = QByteArray(FILE_TRANSFER_RESULT) + FT_SEND_FILE
                      + filename.toUtf8() + INTERUPT
                      + QByteArray::number(fileSize) + INTERUPT;
    client->write(header);

    // 发送文件正文（分片）
    QFile file(downloadFilePath + filename);
    file.open(QIODevice::ReadOnly);
    qint64 sended = 0;
    while (sended < fileSize) {
        QByteArray chunk = file.read(qMin(fileSize - sended, 65536LL));
        client->write(chunk);
        sended += chunk.size();
    }
    file.close();

    // 发送结束标记
    client->write(FILE_TRANSFER_END);
    client->flush();

    server->writeLog("完成文件 [" + filename + "] 向用户 [" + server->client_name[client] + "] 的传输");
}

void FileTransformer::modifyDepot(QString path) {
    downloadFilePath = path;
    downloadFileDepot.setPath(downloadFilePath);
}
