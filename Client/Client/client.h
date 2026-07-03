//
// Created by mechreuo on 2026/6/13.
//

#ifndef PROJECT_SERVER_CLIENT_H
#define PROJECT_SERVER_CLIENT_H

#include "../serverSocket/serverSocket.h"
#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QByteArray>
#include <QPainter>
#include <QPixmap>
#include <QKeyEvent>
/*  第一位是标识符
 * 1表示为聊天消息
 * 2表示心跳包
 */
#define HISTORY_RECORD "0"
#define CHAT_INFO "1"
#define PUNPING_INFO "2"
#define SERVER_CLOSE "3"
#define SERVER_KICK "4"


QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QWidget {
    Q_OBJECT
public:
    explicit Client(QWidget *parent = nullptr);
    ~Client() override;
public slots:
    // 按钮的槽函数
    void on_connectBtn_clicked();
    void on_disConnectBtn_clicked();
    void on_sendBtn_clicked();
    void on_clearLogBtn_clicked();
    void on_clearChatLog_clicked();

    void isConnected(); // 连接后的槽函数
    void sendHeartPing();      // 连接后发送心跳包
    void receiveMsg();      // 接收服务器的消息
public:
    bool eventFilter(QObject *obj, QEvent *event) override;



private:
    Ui::Client *ui;
    QTimer* heartTime;   // 心跳周期
    QString name;       // 客户端名称
    QString serIP;      // 服务器IP
    quint16 serPort;    // 服务器端口号
    serverSocket *serverTar; // 服务器的socket
    QString msg;    // 发送的消息

    int connectState;   // 连接状态,0表示未连接，1表示已连接，2表示正在连接
    QRect stateElli;

private:
    void setConnectBtnState();     // 设置连接后的按钮状态
    void setDisConnectBtnState();   // 设置未连接时候的按钮状态
    void setHavingConnectBtnState();    // 设置已连接状态
    void writeLog(QString log);        // 写下程序执行日志
    void paintStateDot();

};


#endif //PROJECT_SERVER_CLIENT_H
