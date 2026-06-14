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

}

Client::~Client() {
    delete ui;
}

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

/* ========================== 槽函数 ========================== */

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


    ui->msgInfo->addItem(msg.mid(1));
    // 判断消息类型
    if(flag == SERVER_CLOSE) {   // 服务器关闭
        on_disConnectBtn_clicked();     // 断开连接
    }

}

/* ========================== 槽函数 ========================== */

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
