//
// Created by mechreuo on 2026/6/12.
//

#include "worker.h"
#include <QByteArray>
#include <QThread>

Worker::Worker(QObject *parent, QTcpServer *ser, QTcpSocket *cli) : QObject(parent) {
    server = ser;
    client = cli;
    checkTime = new QTimer(this);
    connect(cli, &QTcpSocket::readyRead, this, &Worker::read_msg_cli);
    connect(cli, &QTcpSocket::disconnected, this, &Worker::doExit);
    connect(checkTime, &QTimer::timeout, this, &Worker::doExit);
}

Worker::~Worker() {

}

// 工作体
void Worker::doWork() {
    checkTime->start(punpingTime);
}

// 结束体,处理用户下线
void Worker::doExit() {
    emit cli_exit(client);
    this->client->deleteLater();
    this->thread()->quit();
}

void Worker::read_msg_cli() {
    checkTime->start(punpingTime);
    QByteArray firstByte = client->peek(1);
    emit send_server(firstByte, client);
}

void Worker::timeoutHandle() {

}


