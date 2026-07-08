#ifndef PROJECT_CLIENT_FILESRECEIVER_H
#define PROJECT_CLIENT_FILESRECEIVER_H

#include <QWidget>
#include <QDir>
#include <QFileDialog>
#include <QStringListModel>
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


class Client;
class serverSocket;

QT_BEGIN_NAMESPACE
namespace Ui { class FilesReceiver; }
QT_END_NAMESPACE

class FilesReceiver : public QWidget {
    Q_OBJECT
    friend class Client;
    friend class serverSocket;

public:
    explicit FilesReceiver(QWidget *parent = nullptr, serverSocket *ser = nullptr, Client *cli = nullptr, QRecursiveMutex* mutex = nullptr);
    ~FilesReceiver() override;

public slots:
    void on_btnFlushList_clicked();
    void on_btnCloseList_clicked();
    void on_btnDownloadPathSearch_clicked();
    void on_btnDownload_clicked();

private:
    void recycle();
    void saveFile(QString fileName, qint64 fileSize);
    void writeLog(QString log);

public slots:
    void bootProcess();
    void downloadFile();
    void receiveAvailableFiles();


private:
    Ui::FilesReceiver *ui;  // ui界面

private:
    serverSocket* targetServer;
    Client* localClient;
    QRecursiveMutex* socketMutex;

};


#endif //PROJECT_CLIENT_FILESRECEIVER_H
