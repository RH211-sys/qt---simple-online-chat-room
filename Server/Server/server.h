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
#include <QStringListModel>

#define HISTORY_RECORD "0"
#define CHAT_INFO "1"
#define PUNPING_INFO "2"
#define SERVER_CLOSE "3"
#define SERVER_KICK "4"
#define PRIVATE_SEND_REQUEST "5"
#define SEARCH_SUCCESS "6"
#define SEARCH_FAILED "7"
#define INTERUPT "|"
#define PRIVATE_MSG "8"
#define USER_UPDATE "9"
#define USER_INIT '\b'
#define USER_UPDATE_OFF '\r'

#define FILE_TRANSFER_REQUEST "A"
#define FILE_TRANSFER_RESULT "B"


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
    friend class FileTransformer;
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
    // 网络函数
    void handleConnect();       // 处理用户连接
    void handleDisConnect(QTcpSocket *cli);    // 处理下线
    void receiveCliMsg(QByteArray flag, QTcpSocket* cli);   // 广播信号
    void broadCast(QByteArray content, QTcpSocket* cli);   // 广播消息
    void broadCast(QString content, QTcpSocket* cli);      // 广播消息重载
    void broadCast(QString content);
    void tellPointedUser(QString content, QTcpSocket* cli, QString state);     // 向指定用户发送数据

    // 文件传输模块的槽
    void addNewSharedFile(QTcpSocket* cli_source, QString fileName);     // 添加新共享文件
    void addNewPrivateFile(QTcpSocket* cli_source, QString cliTargetName, QString fileName);   // 添加私发文件

    signals:
    void receiveFile(QTcpSocket* cli);

private:
    void flushDB();     // 用于刷新数据库，可能在其他函数中调用
    void cli_cnt_change(int x);     // 加客户端

private:
    Ui::Server *ui;     // 服务器的UI组件
    QString log;        // 日志信息暂存变量

    // 网络相关对象
    QTcpServer *server;  // Tcp服务器对象
    QMap<QTcpSocket *, QThread *> client_group;
    QMap<QTcpSocket *, QString> client_name;
    QMap<QString, QTcpSocket *> name_to_ip;     // 反射

    QSet<QTcpSocket *> client_firstBag_set;     // 首次发包前的缓存

    // 数据库操作相关对象
    QSqlDatabase chatDB;
    QSqlTableModel *chatTableModel;

    // 当前在线用户
    QStringListModel *chaterModel;
    QStringList chaterList;

    // 文件传输模块相关属性
    QMap<QTcpSocket*, QSet<QString>> privateFiles;     // 私人文件
    QSet<QString> sharedFiles;      // 共享文件

    int N = 5;    // 数据表的最大历史信息条数(启动后默认是5)
    const quint16 ser_port = 12345;     // 监听端口
    int cli_cnt = 0;    // 连接的客户端个数



private:
    void addChatInfo(QString time, QString sender, QString name, QString msg);
    void writeLog(QString text);
    void cleanCli(QTcpSocket* cli);

};




#endif //CUR_PROJECT_SERVER_H
