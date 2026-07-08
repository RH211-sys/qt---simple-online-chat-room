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
#include <QStringListModel>
#include <QFileDialog>
#include <QFileInfo>
#include <QThread>
#include <QMutex>


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

/* ==================== 文件传输模块类声明 ==================== */
class FilesTransFerer;
class FilesReceiver;

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QWidget {
    friend class FilesTransFerer;
    friend class FilesReceiver;
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
    void on_btnFilesReceive_clicked();   // 文件接收

    void isConnected(); // 连接后的槽函数
    void sendHeartPing();      // 连接后发送心跳包
    void receiveMsg();      // 接收服务器的消息

    /* ================== 客户端发射信号(跨线程沟通) =================== */
    signals:
    void TransferSharedFile(QFileInfo& fileInfo);
    void TransferPrivateFile(QFileInfo& fileInfo, QString targetClientName);
    void receiveFile();
    void getFiles();
    void downloadFile();


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

    // 当前在线用户
    QStringListModel *chaterModel;
    QStringList chaterList;

    /* ================= 文件传输模块 ================= */
    QThread* fileTransferThread;
    QRecursiveMutex* socketMutex;  // 保护serverTar的跨线程互斥锁（递归锁）
    FilesTransFerer* TransferWorker;
    FilesReceiver* filesReceiver;



private:
    void setConnectBtnState();     // 设置连接后的按钮状态
    void setDisConnectBtnState();   // 设置未连接时候的按钮状态
    void setHavingConnectBtnState();    // 设置已连接状态
    void writeLog(QString log);        // 写下程序执行日志
    void paintStateDot();
    void updateOnlineUser(QString name, int alterMode);     // 客户端在线人数实时显示
};


#endif //PROJECT_SERVER_CLIENT_H
