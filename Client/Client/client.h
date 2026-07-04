/*
 * 使用过的：const char* : "0" - "4"
 *          int: -1 ~ 3         0除外
 * */

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
/*  第一位是标识符，和服务器的通信协议
 * 1表示为聊天消息
 * 2表示心跳包
 * 3表示服务器关闭
 * 4表示服务器踢出
 * 5表示向服务器请求查找用户
 * 6表示服务器找到用户IP
 * 7表示服务器没有找到该用户
 */
#define HISTORY_RECORD "0"
#define CHAT_INFO "1"
#define PUNPING_INFO "2"
#define SERVER_CLOSE "3"
#define SERVER_KICK "4"
#define SEARCH_REQUEST "5"
#define SEARCH_SUCCESS "6"
#define SEARCH_FAILED "7"


/*
 * 判断用户输入的协议(在输入指定用户名/IP那一栏)
 * 当输入错误，返回ERROR_EDIT_INFO
 * 输入IP时，返回IP_EDIT_INFO
 * 输入用户名时，返回NAME_EDIT_INFO
 * 输入IP但是没有连接服务器，返回WITHOUT_CONNECTION
 * 如果需要服务器查找，则返回UNKNOWN，因为不知道找到了没
 */
#define ERROR_EDIT_INFO -1
#define UNKNOWN -2
#define IP_EDIT_INFO 1
#define NAME_EDIT_INFO 2
#define WITHOUT_CONNECTION 3


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
    void on_connectBtn_clicked();   // 连接服务器
    void on_disConnectBtn_clicked();    // 和服务器断开连接
    void on_sendBtn_clicked();      // 发送广播消息
    void on_clearLogBtn_clicked();  // 清空日志缓存
    void on_clearChatLog_clicked(); // 清空聊天记录缓存
    void on_btnMsgPrivate_clicked();    // 消息私发
    void on_btnFileDirSearch_clicked(); // 文件“浏览”按钮
    void on_btnFileShared_clicked();    // 确认文件共享
    void on_btnFilePrivate_clicked();   // 文件私发
    void on_btnFileReceive_clicked();   // 文件接收

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
    struct {
        QHostAddress p2p_oppose;
        bool isAvaliable;
    } privateOppose;    // 表示在p2p模式下的对方主机地址，这个结构体主要用来处理私发模块中的判断用户输入

private:
    void setConnectBtnState();     // 设置连接后的按钮状态
    void setDisConnectBtnState();   // 设置未连接时候的按钮状态
    void setHavingConnectBtnState();    // 设置已连接状态
    void writeLog(QString log);        // 写下程序执行日志
    void paintStateDot();
    int checkUserEdit(QHostAddress& oppose);        // 关于私发模块中的ip/用户名的正确性检查

};


#endif //PROJECT_SERVER_CLIENT_H
