//
// Created by mechreuo on 2026/6/11.
//

#ifndef CUR_PROJECT_SERVER_H
#define CUR_PROJECT_SERVER_H


#include <QWidget>
#include <QTcpServer>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QTableView>
#include <QTcpSocket>
#include <QMap>
#include <QThread>
#include <QByteArray>
#include <QSet>
#include <QRandomGenerator>
#include <QSqlQuery>

#define HISTORY_RECORD "0"
#define CHAT_INFO "1"
#define PUNPING_INFO "2"
#define SERVER_CLOSE "3"


QT_BEGIN_NAMESPACE
namespace Ui { class Server; }
QT_END_NAMESPACE

/*
 * 服务器类
 * 功能：程序执行服务端程序时，先实例化一个服务器对象，然后绑定IP和端口，打开监听
 *      核心是处理服务器逻辑的类
 * */
class Server : public QWidget {
    Q_OBJECT
public:
    explicit Server(QWidget *parent = nullptr);     // 服务器构造函数
    ~Server() override;        // 服务器析构函数

    // 槽函数
public slots:
    // 按钮槽函数
    void on_btnDBReset_clicked();
    void on_btnSerClose_clicked();
    void on_btnLogClear_clicked();
    void on_btnKick_clicked();      // 踢出按钮点击
    void on_btnSerMsgLimit_clicked(); // 设置最大消息数量
    // 网络槽函数
    void handleConnect();       // 处理用户连接
    void handleDisConnect(QTcpSocket *cli);    // 处理下线
    void receiveCliMsg(QByteArray content, QTcpSocket* cli);   // 广播信号
    void broadCast(QByteArray content, QTcpSocket* cli);   // 广播消息
    void broadCast(QString content, QTcpSocket* cli);      // 广播消息重载
    void broadCast(QString content);

private:
    void flushDB();     // 用于刷新数据库，可能在其他函数中调用

private:
    Ui::Server *ui;     // 服务器的UI组件
    QString log;        // 日志信息暂存变量

    // 网络相关对象
    QTcpServer *server;  // Tcp服务器对象
    QMap<QTcpSocket *, QThread *> client_group;
    QMap<QTcpSocket *, QString> client_name;
    QSet<QTcpSocket *> client_firstBag_set;

    // 数据库操作相关对象
    QSqlDatabase chatDB;
    QSqlTableModel *chatTableModel;

    int N = 5;    // 数据表的最大历史信息条数(启动后默认是5)
    const quint16 ser_port = 12345;     // 监听端口



private:
    void addChatInfo(QString time, QString sender, QString name, QString msg);
    void writeLog(QString text);
};




#endif //CUR_PROJECT_SERVER_H
