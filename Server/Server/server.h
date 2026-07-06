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

/* ==================== 基础协议标志 ==================== */
#define CHAT_INFO "1"             // 聊天消息父标志
#define PUNPING_INFO "2"         // 心跳包
#define SERVER_CLOSE "3"         // 服务器关闭
#define SERVER_KICK "4"          // 服务器踢人
#define FILE_TRANSFER_REQUEST "A" // 文件传输：客户端→服务端
#define FILE_TRANSFER_RESULT "B" // 文件传输：服务端→客户端

/* ==================== 私发回执（保留独立flag） ==================== */
#define SEARCH_SUCCESS "6"        // 私发成功回执
#define SEARCH_FAILED "7"         // 私发失败（目标用户不存在）

/* ==================== CHAT_INFO 子类型 ==================== */
#define CHAT_BROADCAST  "000"  // C→S 广播消息
#define CHAT_PRIVATE_REQ "001"  // C→S 私发请求
#define CHAT_PRIVATE_FWD  "995"  // S→C 私发转发
#define CHAT_HISTORY      "996"  // S→C 历史记录
#define CHAT_USER_OFFLINE "997"  // S→C 用户离线
#define CHAT_USER_INIT    "998"  // S→C 在线用户初始化
#define CHAT_USER_JOIN    "999"  // S→C 新用户加入

/* ==================== FILE_TRANSFER 子类型 ==================== */
// 客户端→服务端
#define FT_SHARED_UPLOAD  "000"  // 共享文件上传
#define FT_PRIVATE_UPLOAD "001"  // 私发文件上传
#define FT_QUERY_FILES    "002"  // 查询文件列表
#define FT_DOWNLOAD_REQ   "003"  // 请求下载文件
// 服务端→客户端
#define FT_SHARED_NOTIFY  "999"  // 共享文件通知
#define FT_PRIVATE_NOTIFY "998"  // 私发文件通知
#define FT_QUERY_SUCCESS  "997"  // 查询文件列表成功
#define FT_QUERY_FAIL     "995"  // 查询文件列表失败
#define FT_SEND_FILE      "994"  // 发送文件数据
#define FT_ACK_SENDER     "993"  // 回应发送源

/* ==================== 通用分隔符 ==================== */
#define INTERUPT "\x1E"               // 字段分隔符（两端统一）
#define FILE_TRANSFER_END "\x03"     // 文件传输结束标记


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
