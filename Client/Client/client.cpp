#include "client.h"
#include "ui_Client.h"
#include "../worker/file_transfer_module/FilesTransFerer.h"
#include "../worker/files_receive_module/filesreceiver.h"
#include <QDebug>

Client::Client(QWidget *parent) : QWidget(parent), ui(new Ui::Client) {
    ui->setupUi(this);
    serverTar = new serverSocket(this);
    heartTime = new QTimer(this);
    QPoint pos = ui->connectCondition->pos();
    stateElli = QRect(pos.x() + 50, pos.y() + 3, 5, 5);
    setDisConnectBtnState();

    // 创建互斥锁，保护serverTar的跨线程访问
    socketMutex = new QRecursiveMutex();

    // 创建文件传输线程和传输工人(发送和接收)
    TransferWorker = new FilesTransFerer(nullptr, this, serverTar, socketMutex);
    filesReceiver = new FilesReceiver(nullptr, serverTar, this, socketMutex);
    fileTransferThread = new QThread(this);

    TransferWorker->moveToThread(fileTransferThread);

    // 启动子线程
    fileTransferThread->start();

    // 设置信号和槽
    connect(serverTar, &QAbstractSocket::connected, this, &Client::isConnected);
    connect(heartTime, &QTimer::timeout, this, &Client::sendHeartPing);
    connect(serverTar, &QTcpSocket::readyRead, this, &Client::receiveMsg);
    // 文件传输的信号和槽
    connect(this, &Client::TransferSharedFile, TransferWorker, &FilesTransFerer::TransferSharedFile);
    connect(this, &Client::TransferPrivateFile, TransferWorker, &FilesTransFerer::TransferPrivateFile);

    // 文件接收的信号和槽
    connect(this, &Client::receiveFile, filesReceiver, &FilesReceiver::bootProcess);
    connect(this, &Client::getFiles, filesReceiver, &FilesReceiver::receiveAvailableFiles);
    connect(this, &Client::downloadFile, filesReceiver, &FilesReceiver::downloadFile);

    // 设置过滤器
    ui->sendMsgInfo->installEventFilter(this);

    // 绑定在线用户
    chaterModel = new QStringListModel(this);
    ui->onlineChaters->setModel(chaterModel);
}

Client::~Client() {
    filesReceiver->~FilesReceiver();
    delete ui;
}

/* ========================== 通用函数模块 ========================== */

void Client::setConnectBtnState() {
    // 由于正在连接，所以不能连接和发送消息
    ui->disConnectBtn->setEnabled(true);
    ui->connectBtn->setEnabled(false);
    ui->sendBtn->setEnabled(false);
    /* ===== 文件传输模块按钮设置 ===== */
    ui->btnFilePrivate->setEnabled(false);
    ui->btnFileShared->setEnabled(false);
    ui->btnFilesReceive->setEnabled(false);
    ui->connectCondition->setText("正在连接");
    connectState = 2;
    paintStateDot();
}

void Client::setHavingConnectBtnState() {
    // 由于已经连接，所以不能重复连接，并且能发送消息
    ui->disConnectBtn->setEnabled(true);
    ui->connectBtn->setEnabled(false);
    ui->sendBtn->setEnabled(true);
    /* ===== 文件传输模块按钮设置 ===== */
    ui->btnFilePrivate->setEnabled(true);
    ui->btnFileShared->setEnabled(true);
    ui->btnFilesReceive->setEnabled(true);

    ui->connectCondition->setText("已连接");
    connectState = 1;
    paintStateDot();
}

void Client::setDisConnectBtnState() {
    // 由于没有连接，所以不能断开连接和发送消息
    ui->disConnectBtn->setEnabled(false);
    ui->connectBtn->setEnabled(true);
    ui->sendBtn->setEnabled(false);
    /* ===== 文件传输模块按钮设置 ===== */
    ui->btnFilePrivate->setEnabled(false);
    ui->btnFileShared->setEnabled(false);
    ui->btnFilesReceive->setEnabled(false);

    ui->connectCondition->setText("未连接");
    connectState = 0;
    paintStateDot();


}

// 当alterMode为1时表示添加，为-1时表示删除
void Client::updateOnlineUser(QString name, int alterMode) {
    if(alterMode == 1) {
        chaterList.append(name);
        chaterModel->setStringList(chaterList);
    }
    else if(alterMode == -1) {
        chaterList.removeOne(name);
        chaterModel->setStringList(chaterList);
    }
}

void Client::writeLog(QString log) {
    ui->logInfo->addItem(log);
}



/* ========================== 槽函数/基本模块 ========================== */

/*
 * 点击连接后，获取信息栏中的服务器IP,Port,客户端的name
 * 和服务器建立TCP连接
 * 更新按钮状态
 */
void Client::on_connectBtn_clicked() {
    // 获取用户键入的信息
    name = ui->nameEdit->text();
    // 数字检测
    bool ok;
    int num = name.toInt(&ok);

    if(ok || name.contains('.') || name.contains('|') || name.contains('/') || name.contains("+")) {
        ui->msgInfo->addItem("请输入正确的name，名字不能纯数字或包含\"|\",\".\",\"/\",\"+\"等特殊字符");
        return;
    }
    if(name.isEmpty()) name = "Unknown";
    serIP = ui->IPEdit->text();
    serPort = ui->portEdit->text().toUInt();
    serverTar->setPeerName(name);
    setConnectBtnState();

    // 连接服务器
    serverTar->connectToHost(serIP, serPort);
}

void Client::on_disConnectBtn_clicked() {
    {
        QMutexLocker locker(socketMutex);
        serverTar->disconnectFromHost();
    }
    heartTime->stop();
    // ui状态清理
    setDisConnectBtnState();
    chaterList.clear();
    chaterModel->setStringList(chaterList);
    writeLog("连接已尝试断开");
}

void Client::on_sendBtn_clicked() {
    QByteArray body = ui->sendMsgInfo->toPlainText().toUtf8();
    // QByteArray(CHAT_INFO) + CHAT_BROADCAST + size + INTERUPT + body
    QByteArray msg = QByteArray(CHAT_INFO) + CHAT_BROADCAST
                   + QByteArray::number(body.size()) + INTERUPT
                   + body;
    QMutexLocker locker(socketMutex);
    serverTar->write(msg);
    serverTar->flush();
    ui->sendMsgInfo->clear();
    writeLog("数据已发送");
}

void Client::on_clearLogBtn_clicked() {
    ui->logInfo->clear();
}

void Client::on_clearChatLog_clicked() {
    ui->msgInfo->clear();
}


// 成功连接
void Client::isConnected() {
    // 设置连接后的按钮状态
    setHavingConnectBtnState();
#if 1
    // 开始向服务器发送心跳包
    heartTime->start(3000);     // 每3s发一次
    {
        QMutexLocker locker(socketMutex);
        QByteArray nameBytes = name.toUtf8();
        QByteArray firstBag = QByteArray(FIRST_BAG) + QByteArray::number(nameBytes.size()) + INTERUPT + nameBytes;
        serverTar->write(firstBag);
        serverTar->flush();
    }
#endif
    writeLog("完成连接处理");
}

void Client::sendHeartPing() {
#if 1
    {
        QMutexLocker locker(socketMutex);
        serverTar->write(PUNPING_INFO);
        serverTar->flush();
    }
    heartTime->start(3000);     // 重置时间
    // writeLog("已发送心跳包");  // 不好看日志信息
#endif
}

void Client::receiveMsg() {
    QMutexLocker locker(socketMutex);
    // 循环处理粘包：每次读一个完整消息，直到缓冲区空
    while (serverTar->bytesAvailable() > 0) {
        char flag;
        if (!serverTar->getChar(&flag)) break;

        // 服务器关闭 / 被踢出：flag + size + INTERUPT + body
        if (flag == SERVER_CLOSE[0] || flag == SERVER_KICK[0]) {
            QByteArray sizeBuf;
            char ch;
            while (serverTar->getChar(&ch) && ch != INTERUPT[0]) sizeBuf.append(ch);
            int bodySize = sizeBuf.toInt();
            QByteArray body = serverTar->read(bodySize);
            ui->msgInfo->addItem(QString::fromUtf8(body));
            on_disConnectBtn_clicked();

        } else if (flag == CHAT_INFO[0]) {
            // 读子类型（3字节）
            QByteArray subType = serverTar->read(3);

            if (subType == CHAT_BROADCAST) {
                // "000" + size + INTERUPT + body
                QByteArray sizeBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) sizeBuf.append(ch);
                int bodySize = sizeBuf.toInt();
                QByteArray body = serverTar->read(bodySize);
                ui->msgInfo->addItem(QString::fromUtf8(body));

            } else if (subType == CHAT_PRIVATE_FWD) {
                // "995" + senderName + INTERUPT + size + INTERUPT + body
                QByteArray senderBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) senderBuf.append(ch);
                QString senderName = QString::fromUtf8(senderBuf);
                QByteArray sizeBuf;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) sizeBuf.append(ch);
                int bodySize = sizeBuf.toInt();
                QByteArray body = serverTar->read(bodySize);
                ui->msgInfo->addItem(senderName + " 悄悄的告诉你: " + QString::fromUtf8(body));
                writeLog("已接收私信");

            } else if (subType == CHAT_HISTORY) {
                // "996" + bodySize + INTERUPT + body{record + INTERUPT + ...}
                QByteArray sizeBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) sizeBuf.append(ch);
                int bodySize = sizeBuf.toInt();
                QByteArray body = serverTar->read(bodySize);
                int start = 0;
                for (int i = 0; i < body.size(); i++) {
                    if (body[i] == INTERUPT[0]) {
                        QString record = QString::fromUtf8(body.mid(start, i - start));
                        if (!record.isEmpty()) ui->msgInfo->addItem(record);
                        start = i + 1;
                    }
                }

            } else if (subType == CHAT_USER_OFFLINE) {
                // "997" + name + INTERUPT
                QByteArray nameBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) nameBuf.append(ch);
                QString name = QString::fromUtf8(nameBuf);
                updateOnlineUser(name, -1);
                ui->msgInfo->addItem(name + "已下线");

            } else if (subType == CHAT_USER_INIT) {
                // "998" + bodySize + INTERUPT + body{name + INTERUPT + ...}
                QByteArray sizeBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) sizeBuf.append(ch);
                int bodySize = sizeBuf.toInt();
                QByteArray body = serverTar->read(bodySize);
                int start = 0;
                for (int i = 0; i < body.size(); i++) {
                    if (body[i] == INTERUPT[0]) {
                        QString name = QString::fromUtf8(body.mid(start, i - start));
                        if (!name.isEmpty()) updateOnlineUser(name, 1);
                        start = i + 1;
                    }
                }

            } else if (subType == CHAT_USER_JOIN) {
                // "999" + name + INTERUPT
                QByteArray nameBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != INTERUPT[0]) nameBuf.append(ch);
                QString name = QString::fromUtf8(nameBuf);
                updateOnlineUser(name, 1);
                ui->msgInfo->addItem(name + " 加入了聊天室");
            }

        } else if (flag == SEARCH_SUCCESS[0]) {
            // 私信发送成功回执
            QByteArray content;
            char ch;
            while (serverTar->getChar(&ch) && ch != INTERUPT[0]) content.append(ch);
            if (!content.isEmpty()) ui->msgInfo->addItem(QString::fromUtf8(content));
            writeLog("私信已发送");
            ui->msgInfo->addItem("私发成功");

        } else if (flag == SEARCH_FAILED[0]) {
            // 私信发送失败回执
            QByteArray content;
            char ch;
            while (serverTar->getChar(&ch) && ch != INTERUPT[0]) content.append(ch);
            writeLog("私信发送失败，用户名填写错误");

        } else if (flag == FILE_TRANSFER_RESULT[0]) {
            // 文件传输协议 —— 读子类型（3字节）
            QByteArray subType = serverTar->read(3);

            if (subType == FT_SHARED_NOTIFY) {
                // B + "999" + fileName + FILE_TRANSFER_END
                QByteArray nameBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != FILE_TRANSFER_END[0]) nameBuf.append(ch);
                QString fileName = QString::fromUtf8(nameBuf);
                ui->msgInfo->addItem("[新共享文件] " + fileName);
                writeLog("收到共享文件通知: " + fileName);

            } else if (subType == FT_PRIVATE_NOTIFY) {
                // B + "998" + fileName + FILE_TRANSFER_END
                QByteArray nameBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != FILE_TRANSFER_END[0]) nameBuf.append(ch);
                QString fileName = QString::fromUtf8(nameBuf);
                ui->msgInfo->addItem("[新私发文件] " + fileName);
                writeLog("收到私发文件通知: " + fileName);

            } else if (subType == FT_QUERY_SUCCESS) {
                emit getFiles();

            } else if (subType == FT_QUERY_FAIL) {
                // B + "995"  无body
                filesReceiver->writeLog("暂无可用文件");

            } else if (subType == FT_SEND_FILE) {
                // B + "994" + fileName + INTERUPT + fileSize + INTERUPT + [data] + FILE_TRANSFER_END
                emit downloadFile();

            } else if (subType == FT_ACK_SENDER) {
                // B + "993" + status + FILE_TRANSFER_END
                QByteArray statusBuf;
                char ch;
                while (serverTar->getChar(&ch) && ch != FILE_TRANSFER_END[0]) statusBuf.append(ch);
                QString statusStr = QString::fromUtf8(statusBuf);
                if (statusStr.startsWith("000")) {
                    writeLog("共享文件上传成功");
                } else if (statusStr.startsWith("001")) {
                    if (statusStr.contains("fail")) {
                        writeLog("私发文件上传失败，目标用户不在线");
                    } else {
                        writeLog("私发文件上传成功");
                    }
                }
            }

        }
        // 是其他字符，可能是FILE_TRANSFER_END等
        else continue;

    }
}

/* ==========================  绘图函数模块 ========================== */

void Client::paintStateDot() {
    QPixmap pixmap(ui->connectCondition->size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(Qt::gray);
    if(connectState == 0) {
        painter.drawText(pixmap.rect(), Qt::AlignLeft, "未连接");
        painter.setBrush(Qt::red);
    } else if(connectState == 1) {
        painter.drawText(pixmap.rect(), Qt::AlignLeft, "已连接");
        painter.setBrush(Qt::green);
    } else if(connectState == 2) {
        painter.drawText(pixmap.rect(), Qt::AlignLeft, "正在连接");
        painter.setBrush(Qt::yellow);
    }
    painter.drawEllipse(stateElli);
    ui->connectCondition->setPixmap(pixmap);
}

/* ==========================  事件过滤模块 ========================== */

bool Client::eventFilter(QObject *obj, QEvent *event) {
    if(obj == ui->sendMsgInfo && event->type() == QKeyEvent::KeyPress) {
        auto keyEvent = (QKeyEvent*)event;
        auto k = keyEvent->key();
        if(k == Qt::Key_Return || k == Qt::Key_Enter){
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Shift+Enter：正常换行，不拦截
                return QWidget::eventFilter(obj, event);
            } else {
                on_sendBtn_clicked();
            }
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

/* ==========================  私信函数模块 ========================== */

// 私信：QByteArray(CHAT_INFO) + CHAT_PRIVATE_REQ + targetName + INTERUPT + size + INTERUPT + body
void Client::on_btnMsgPrivate_clicked() {
    QMutexLocker locker(socketMutex);
    QString targetName = ui->privateUserEdit->text();
    if(serverTar->state() != QAbstractSocket::ConnectedState) {
        writeLog("未连接服务器");
        return;
    }
    QByteArray body = ui->sendMsgInfo->toPlainText().toUtf8();
    QByteArray msg = QByteArray(CHAT_INFO) + CHAT_PRIVATE_REQ
                   + targetName.toUtf8() + INTERUPT
                   + QByteArray::number(body.size()) + INTERUPT
                   + body;
    serverTar->write(msg);
    serverTar->flush();
}

/* ==========================  文件传输函数模块 ========================== */

// 文件浏览
void Client::on_btnFileDirSearch_clicked() {
    QString filePath = QFileDialog::getOpenFileName(
            this,                       // 父窗口
            "选择要发送的文件",            // 弹窗标题
            QDir::homePath(),            // 默认打开的目录（用户主目录）
            "所有文件 (*.*)"              // 文件类型过滤器
    );
    ui->fileDirEdit->setText(filePath);
}

// 文件共享
void Client::on_btnFileShared_clicked() {
    // 先获取文件名
    QString filePath = ui->fileDirEdit->text();
    QFileInfo info(filePath);
    QDir dir = info.dir();
    // 判断文件是否存在
    if(!info.exists()) {
        ui->fileDirEdit->setText("该文件不存在");
        return;
    }
    // 文件存在，开始发送
    /* =================== 文件上传模块 ================== */
    emit TransferSharedFile(info.absoluteFilePath());
}

// 文件私发
void Client::on_btnFilePrivate_clicked() {
    // 获取要私发的用户名
    QString name = ui->privateUserEdit->text();
    if(!chaterList.contains(name)) {
        writeLog("该用户不在线");
        return;
    }

    // 获取文件名
    QString filePath = ui->fileDirEdit->text();
    QFileInfo info(filePath);
    QDir dir = info.dir();
    // 判断文件是否存在,正常情况都是存在
    if(!info.exists()) {
        ui->fileDirEdit->setText("该文件不存在");
        return;
    }
    // 文件存在，开始发送
    /* =================== 文件上传模块 ================== */
    emit TransferPrivateFile(info.absoluteFilePath(), name);
}

// 文件接收
void Client::on_btnFilesReceive_clicked() {
    emit receiveFile();
}




