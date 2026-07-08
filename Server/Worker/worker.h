//
// Created by mechreuo on 2026/6/12.
//

#ifndef CUR_PROJECT_WORKER_H
#define CUR_PROJECT_WORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QMutex>

class Worker : public QObject {
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr, QTcpServer *ser = nullptr, QTcpSocket *cli = nullptr, QMutex* mutex = nullptr);
    ~Worker() override;

public slots:
    void doWork();      // 开始工作
    void doExit();      // 停止工作
    void read_msg_cli();  // 接受用户发来的消息
    void timeoutHandle();      // 心跳超时处理
    signals:
    void send_server(QByteArray, QTcpSocket*);
    void cli_exit(QTcpSocket*);


private:
    QTcpServer* server;     // 指向服务端
    QTcpSocket* client;     // 指向客户端
    QMutex* socketMutex;    // 保护client的跨线程互斥锁
    QString name;           // 客户端名称(还未写客户端，暂时没有途径获取测试数据)
    QTimer* checkTime;       // 定时器
    const int punpingTime = 1000000;  // 设置最晚心跳时长为10000ms
    // 调试用，先长点
};


#endif //CUR_PROJECT_WORKER_H
