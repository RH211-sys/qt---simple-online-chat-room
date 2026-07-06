#include "client.h"
#include "ui_Client.h"
#include "../worker/FilesTransFerer.h"
#include <QDebug>





Client::Client(QWidget *parent) : QWidget(parent), ui(new Ui::Client) {
    ui->setupUi(this);
    serverTar = new serverSocket(this);
    heartTime = new QTimer(this);
    QPoint pos = ui->connectCondition->pos();
    stateElli = QRect(pos.x() + 50, pos.y() + 3, 5, 5);
    setDisConnectBtnState();
    // 创建文件传输线程和传输工人
    TransferWorker = new FilesTransFerer(nullptr, this, serverTar);
    fileTransferThread = new QThread(this);
    TransferWorker->moveToThread(fileTransferThread);

    // 设置信号和槽
    connect(serverTar, &QAbstractSocket::connected, this, &Client::isConnected);
    connect(heartTime, &QTimer::timeout, this, &Client::sendHeartPing);
    connect(serverTar, &QTcpSocket::readyRead, this, &Client::receiveMsg);
    // 文件传输的信号和槽
    connect(this, &Client::TransferSharedFile, TransferWorker, &FilesTransFerer::TransferSharedFile);
    connect(this, &Client::TransferPrivateFile, TransferWorker, &FilesTransFerer::TransferPrivateFile);
    // 设置过滤器
    ui->sendMsgInfo->installEventFilter(this);

    // 绑定在线用户
    chaterModel = new QStringListModel(this);
    ui->onlineChaters->setModel(chaterModel);
}

Client::~Client() {
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
    serverTar->disconnectFromHost();
    heartTime->stop();
    // ui状态清理
    setDisConnectBtnState();
    chaterList.clear();
    chaterModel->setStringList(chaterList);
    writeLog("连接已尝试断开");
}

void Client::on_sendBtn_clicked() {
    msg = CHAT_INFO + ui->sendMsgInfo->toPlainText();
    serverTar->write(msg.toUtf8());
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
    // 开始向服务器发送心跳包
    heartTime->start(3000);     // 每3s发一次
    serverTar->write(name.toUtf8());
    writeLog("完成连接处理");
}

void Client::sendHeartPing() {
    serverTar->write(PUNPING_INFO);
    heartTime->start(3000);     // 重置时间
    writeLog("已发送心跳包");
}

void Client::receiveMsg() {
    msg = QString::fromUtf8(serverTar->readAll());
    // 分割信息
    QStringList msgs = msg.split('|');
    QString flag;
    // 判断消息类型
    for(auto& line: msgs) {
        if (line.isEmpty()) continue;
        flag = line[0];
        if (flag == SERVER_CLOSE || flag == SERVER_KICK) {   // 服务器关闭或者被踢出
            ui->msgInfo->addItem(line.mid(1));
            on_disConnectBtn_clicked();     // 断开连接
        } else if (flag == CHAT_INFO || flag == HISTORY_RECORD) {
            ui->msgInfo->addItem(line.mid(1));
        } else if (flag == USER_UPDATE) {
            updateOnlineUser(line.mid(1), 1);
            ui->msgInfo->addItem(line.mid(1) + " 加入了聊天室");
        } else if (flag == USER_INIT) {
            updateOnlineUser(line.mid(1), 1);
        } else if (flag == SEARCH_SUCCESS) {
            ui->msgInfo->addItem(line.mid(1));
            writeLog("私信已发送");
        } else if (flag == SEARCH_FAILED) {
            writeLog("私信发送失败，用户名填写错误");
        } else if (flag == PRIVATE_MSG) {
            ui->msgInfo->addItem(line.mid(1));
            writeLog("已接受私信");
        } else if(flag == USER_UPDATE_OFF) {
            updateOnlineUser(line.mid(1), -1);
            ui->msgInfo->addItem(line.mid(1) + "已下线");
        }
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

// 私信
void Client::on_btnMsgPrivate_clicked() {
    QString text = ui->privateUserEdit->text();
    if(serverTar->state() != QAbstractSocket::ConnectedState) {
        // 未连接服务器
        writeLog("未连接服务器");
        return;
    }
    serverTar->write(QByteArray((PRIVATE_SEND_REQUEST + text +
                                "|" + ui->sendMsgInfo->toPlainText())
                                .toUtf8()));
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
    // 上传文件头
    QString fileMsg = FILE_TRANSFER_REQUEST;
    fileMsg += "000" + info.fileName() + INTERUPT + QString::number(info.size()) + INTERUPT;
    serverTar->write(fileMsg.toUtf8());
    fileMsg.clear();

    // 上传文件正文
    emit TransferSharedFile(dir);
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
    // 上传文件正文
    emit TransferPrivateFile(dir, name);
}

// 文件接收
void Client::on_btnFileReceive_clicked() {

}


