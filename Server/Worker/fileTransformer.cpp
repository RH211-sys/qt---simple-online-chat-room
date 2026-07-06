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
    downloadFileDepot = QDir(downloadFilePath);
}

FileTransformer::~FileTransformer() {

}

void FileTransformer::doReceiveFile(QTcpSocket* cli) {
    if(cli != client) return;
    // 获取传送方式(shared还是private)[阅读3位：保留拓展性]
    QString transferWay = QString::fromUtf8(client->read(3));
    QString name;   // 这是私发文件模块中的目标客户端用户名的变量
    if(transferWay == "001") {
        // 需要获取私发的用户名
        name = QString::fromUtf8(readUntil(client, INTERUPT));
        // 需要去查找这个用户是否在线(客户端也有判断，不过做双重保险)，正常情况不会进入if分支
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

    QString filename;
    // 如果是发送文件，则先存储到服务器
    if(transferWay == "000" || transferWay == "001") {
        // 获取发来的文件名和文件大小
        filename = QString::fromUtf8(readUntil(client, INTERUPT));
        int name_size = filename.size();
        long long n = readUntil(client, INTERUPT).toLongLong();
        // 创建下载文件的文件名(用用户名的方式创建可以确保重写覆盖，节约空间)
        filename += '(' + filename + ')';
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
    }

    /* ============== 发送文件后的处理 =============== */
    if(transferWay == "000") {
        // 文件共享通知
        emit addNewSharedFile(client, filename);
    } else if(transferWay == "001") {
        // 文件私发通知
        emit addNewPrivateFile(client, name, filename);
    }
    /* ============== 下面是接收文件模块 =============== */
    else if(transferWay == "010") {
        // 查询可接收的文件
        cli->read(1);   // 消费掉INTERRUPT
        QString files = FILE_TRANSFER_RESULT;
        files += "010";
        for(auto& x : server->sharedFiles) {
            files += x;
            files += INTERUPT;
        }
        for(auto& x : server->privateFiles[cli]) {
            files += x;
            files += INTERUPT;
        }
        // 通过服务器函数通知
        server->tellPointedUser(files, client, FILE_TRANSFER_RESULT);
    } else if(transferWay == "011") {
        // 发送文件
        filename = QString::fromUtf8(readUntil(client, INTERUPT));
        if(server->sharedFiles.contains(filename) || server->privateFiles[cli].contains(filename)) {
            doTransfer(filename);
        }
        // 一般不会进这个分支
        else {
            // 该文件不存在
            server->writeLog("客户端 " + server->client_name[cli] + " 所需要的文件不存在");
        }
    }
}

void FileTransformer::doTransfer(QString filename) {
    // 获取本地文件的大小
    QFileInfo info(downloadFilePath + filename);
    qint64 n = info.size();  // 是字节数

    // 组装协议头部
    QString body = FILE_TRANSFER_RESULT;
    body += "011" + filename + INTERUPT + QString::number(n) + INTERUPT;
    client->write(body.toUtf8());
    body.clear();

    // 传输正文
    QFile file(downloadFilePath + filename);
    file.open(QIODevice::ReadOnly);
    qint64 sended = 0;
    // 分片读取
    while (sended < n) {
        QByteArray chunk = file.read(qMin(n - sended, 65536LL));  // 每次64KB
        client->write(chunk);
        sended += chunk.size();
    }
    // 读完了且完成传输，关闭
    file.close();

    // 写入日志
    server->writeLog("完成文件 [" + filename + "] 向用户 [" + server->client_name[client] +"] 的传输");
}
