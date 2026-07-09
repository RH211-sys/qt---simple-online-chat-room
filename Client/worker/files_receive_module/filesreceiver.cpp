#include "filesreceiver.h"
#include "../../Client/client.h"
#include "../FilesTransFerer.h"
#include "ui_FilesReceiver.h"


FilesReceiver::FilesReceiver(QWidget *parent, serverSocket *ser, Client *cli, QRecursiveMutex* mutex) : QWidget(parent), ui(new Ui::FilesReceiver) {
    ui->setupUi(this);

    targetServer = ser;
    localClient = cli;
    socketMutex = mutex;
}

FilesReceiver::~FilesReceiver() {
    delete ui;
}


/* ===================== 通用函数 ==================== */
void FilesReceiver::recycle() {
    // 清空可接收文件列表
    ui->receivableFilesList->clear();
    // 清空日志
    ui->logInfo->clear();
    // 清空下载文件名输入框
    ui->downloadFileEdit->clear();

}
// 写入日志
void FilesReceiver::writeLog(QString log) {
    ui->logInfo->addItem(log);
}

void FilesReceiver::saveFile(QString fileName, qint64 fileSize) {
    // 获取下载地址
    QString downloadDir; // 默认地址
    downloadDir = ui->downloadPathEdit->text() + fileName;

    QFile file(downloadDir);
    file.open(QIODevice::WriteOnly);
    qint64 received = 0;
    while (received < fileSize) {
        QByteArray chunk = targetServer->read(qMin(fileSize - received, 65536LL));       // 64KB读取
        file.write(chunk);
        received += chunk.size();
    }
    file.close();
    QString flag = targetServer->peek(1);
    // 消费并检查是否读完
    if(flag == FILE_TRANSFER_END) {
        targetServer->read(1);  // 消费 FILE_TRANSFER_END
        ui->logInfo->addItem("下载成功");
    } else {
        ui->logInfo->addItem("下载的文件可能不完整");
    }
}

/* ===================== 通信槽函数 ==================== */

void FilesReceiver::bootProcess() {
    this->show();
}

void FilesReceiver::downloadFile() {
    QMutexLocker locker(socketMutex);
    // B + "994" + fileName + INTERUPT + fileSize + INTERUPT + [data] + FILE_TRANSFER_END
    // 获取文件名
    QByteArray filenameB;
    char ch;
    while (targetServer->getChar(&ch) && ch != INTERUPT[0]) filenameB.append(ch);
    // 获取文件大小
    QByteArray sizeB;
    while (targetServer->getChar(&ch) && ch != INTERUPT[0]) sizeB.append(ch);

    saveFile(QString::fromUtf8(filenameB), sizeB.toInt());
}

// 接收可下载文件(刷新)
void FilesReceiver::receiveAvailableFiles() {
    QMutexLocker locker(socketMutex);
    // B + "997" + bodySize + INTERUPT + body{fileName + INTERUPT + ...}
    QByteArray sizeBuf;
    char ch;
    while (targetServer->getChar(&ch) && ch != INTERUPT[0]) sizeBuf.append(ch);
    int bodySize = sizeBuf.toInt();
    QByteArray body = targetServer->read(bodySize);
    // 拆分文件列表
    int start = 0;
    for (int i = 0; i < body.size(); i++) {
        if (body[i] == INTERUPT[0]) {
            QString fileName = QString::fromUtf8(body.mid(start, i - start));
            if (!fileName.isEmpty()) {
                ui->receivableFilesList->addItem(fileName);
            }
            start = i + 1;
        }
    }
}

/* ===================== 按钮槽函数 ==================== */

// 关闭窗口，回收资源
void FilesReceiver::on_btnCloseList_clicked() {
    recycle();
    this->close();
}

// 文件浏览
void FilesReceiver::on_btnDownloadPathSearch_clicked() {
    QString filePath = QFileDialog::getExistingDirectory(
            this,                       // 父窗口
            "选择下载目录",              // 弹窗标题
            QDir::homePath()             // 默认打开的目录（用户主目录）
    ) + "/";
    ui->downloadPathEdit->setText(filePath);
}

// 发送文件下载请求
void FilesReceiver::on_btnDownload_clicked() {
    // 获取文件名
    QString fileName = ui->downloadFileEdit->text();
    // 组装协议
    QString msg = FILE_TRANSFER_REQUEST;
    msg += FT_DOWNLOAD_REQ + fileName + FILE_TRANSFER_END;
    // 发送请求
    QMutexLocker locker(socketMutex);
    targetServer->write(msg.toUtf8());
    targetServer->flush();
}

// 发送获取可下载列表请求
void FilesReceiver::on_btnFlushList_clicked() {
    // 先清除原有的文件信息
    ui->receivableFilesList->clear();
    ui->logInfo->clear();
    // 组装消息协议
    QString msg = FILE_TRANSFER_REQUEST;
    msg += FT_QUERY_FILES;
    msg += FILE_TRANSFER_END;
    // 发送
    QMutexLocker locker(socketMutex);
    targetServer->write(msg.toUtf8());
    targetServer->flush();
}





