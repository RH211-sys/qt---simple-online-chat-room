//
// Created by mechreuo on 2026/6/13.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Client.h" resolved

#include "client.h"
#include "ui_Client.h"
#include <QDebug>




Client::Client(QWidget *parent) : QWidget(parent), ui(new Ui::Client) {
    ui->setupUi(this);
    serverTar = new serverSocket(this);
    heartTime = new QTimer(this);
    QPoint pos = ui->connectCondition->pos();
    stateElli = QRect(pos.x() + 50, pos.y() + 3, 5, 5);
    setDisConnectBtnState();
    // 设置信号和槽
    connect(serverTar, &QAbstractSocket::connected, this, &Client::isConnected);
    connect(heartTime, &QTimer::timeout, this, &Client::sendHeartPing);
    connect(serverTar, &QTcpSocket::readyRead, this, &Client::receiveMsg);
    // 设置过滤器
    ui->sendMsgInfo->installEventFilter(this);
    // 初始化结构体
    privateOppose.isAvaliable = false;
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
    ui->connectCondition->setText("正在连接");
    connectState = 2;
    paintStateDot();
}

void Client::setHavingConnectBtnState() {
    // 由于已经连接，所以不能重复连接，并且能发送消息
    ui->disConnectBtn->setEnabled(true);
    ui->connectBtn->setEnabled(false);
    ui->sendBtn->setEnabled(true);
    ui->connectCondition->setText("已连接");
    connectState = 1;
    paintStateDot();
}

void Client::setDisConnectBtnState() {
    // 由于没有连接，所以不能断开连接和发送消息
    ui->disConnectBtn->setEnabled(false);
    ui->connectBtn->setEnabled(true);
    ui->sendBtn->setEnabled(false);
    ui->connectCondition->setText("未连接");
    connectState = 0;
    paintStateDot();
}

void Client::writeLog(QString log) {
    ui->logInfo->addItem(log);
}

/* 检测用户在p2p通信对方信息的正确性
 * 判断用户输入的协议(在输入指定用户名/IP那一栏)
 * 当输入错误，返回ERROR_EDIT_INFO
 * 输入IP时，返回IP_EDIT_INFO
 * 输入用户名时，返回NAME_EDIT_INFO
 * 输入IP但是没有连接服务器，返回WITHOUT_CONNECTION
 * 这里这样封装的原因是优化oppose,以防每次都要复制并检验
 * 这里成功能返回正确并拷贝，否则不拷贝且返回错误，最后还是要转为IP
 */
int Client::checkUserEdit(QHostAddress& oppose) {
    QString text = ui->privateUserEdit->text();
    if(oppose.setAddress(text)) {   // 是否为IP地址，如果是则返回
        return IP_EDIT_INFO;
    } else if(serverTar->state() != QAbstractSocket::ConnectedState) {  //是否连接了服务器，若不是则返回
        // 未连接
        return WITHOUT_CONNECTION;
    } else {
        // 连接了服务器，需要查找用户名对应的IP地址
        QByteArray bytes(SEARCH_REQUEST + text.toUtf8());
        serverTar->write(bytes);
        /*
         * 为方便开发，这里不进行解耦，返回值由receiveMsg里面设置
         * */
    }
    return UNKNOWN;
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
    setDisConnectBtnState();
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
    QString flag = msg[0];

    // 判断消息类型
    if(flag == SERVER_CLOSE || flag == SERVER_KICK) {   // 服务器关闭或者被踢出
        ui->msgInfo->addItem(msg.mid(1));
        on_disConnectBtn_clicked();     // 断开连接
    } else if(flag == CHAT_INFO) {
        ui->msgInfo->addItem(msg.mid(1));
    } else if(flag == SEARCH_SUCCESS) {
        // 服务器查找到用户，返回
        QString text = msg.mid(1);
        privateOppose.isAvaliable = true;
        privateOppose.p2p_oppose.setAddress(text);
    } else if(flag == SEARCH_FAILED) {
        privateOppose.isAvaliable = false;
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
    QHostAddress oppose;
    int check = checkUserEdit(oppose);
    if(check == WITHOUT_CONNECTION) {
        writeLog("没有连接服务器，无法查找");
    } else if(check == IP_EDIT_INFO) {

    } else if(check == UNKNOWN) {
        // 需要获取服务器查找的结果
        if(privateOppose.isAvaliable) {
            // 找到了
            oppose = privateOppose.p2p_oppose;
            // 处理相关发送
        } else {
            // 没有找到
            writeLog("该用户不在线，或者是随机值没输入");
        }

    }
}

// 文件浏览
void Client::on_btnFileDirSearch_clicked() {

}

// 文件共享
void Client::on_btnFileShared_clicked() {

}

// 文件私发
void Client::on_btnFilePrivate_clicked() {

}

// 文件接收
void Client::on_btnFileReceive_clicked() {

}


