#include "server.h"
#include "ui_server.h"
#include "../Worker/worker.h"
#include <QDebug>
#include <QTime>
#include <QDir>


/*
 * 数据库刷新
 * 当前刷新依据：行数是否超限
 */
void Server::flushDB() {
    int currentRow = chatTableModel->rowCount();
    if(currentRow >= N) {
        chatTableModel->removeRows(0, currentRow - N + 1);
    }
    chatTableModel->submitAll();
    chatTableModel->select();
}


/*
 * 构造函数
 * 各种初始化，完成服务器初始化工作
 */
Server::Server(QWidget *parent) : QWidget(parent), ui(new Ui::Server) {
    ui->setupUi(this);
    // 完成网络初始化
    server = new QTcpServer(this);
    server->listen(QHostAddress::AnyIPv4, ser_port);    // 服务器监听
    connect(server, &QTcpServer::newConnection, this, &Server::handleConnect);

    // 完成数据库初始化
    chatDB = QSqlDatabase::addDatabase("QSQLITE");
    chatDB.setDatabaseName("chatInfo.db");
    bool ok = chatDB.open();
    if(ok) {
        writeLog("成功打开数据库");
    }
    // 创建表
    QSqlQuery query(chatDB);
    query.exec("CREATE TABLE IF NOT EXISTS chat_record ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "time CHAR(10), "
               "sender CHAR(20), "
               "name CHAR(20), "
               "msg CHAR(100)"
               ")");


    // 绑定Model
    chatTableModel = new QSqlTableModel(this, chatDB);
    chatTableModel->setTable("chat_record");
    chatTableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    chatTableModel->select();
    // 绑定View
    ui->chatInfo->setModel(chatTableModel);
    // 初始化刷新调试
    chatTableModel->select();
    writeLog(QDir::currentPath() + "/chatInfo.db");
}

Server::~Server() {
    delete ui;
}

void Server::addChatInfo(QString time, QString sender, QString name, QString msg) {
    int currentRow = chatTableModel->rowCount();
    chatTableModel->insertRow(currentRow, QModelIndex());
    chatTableModel->setData(chatTableModel->index(currentRow, 1), time.toUtf8());
    chatTableModel->setData(chatTableModel->index(currentRow, 2), sender.toUtf8());
    chatTableModel->setData(chatTableModel->index(currentRow, 3), name.toUtf8());
    chatTableModel->setData(chatTableModel->index(currentRow, 4), msg.toUtf8());
    if(currentRow >= N) {
        chatTableModel->removeRows(0, currentRow - N + 1);
    }

    chatTableModel->submitAll();
    chatTableModel->select();
}

// 清空数据库按钮
void Server::on_btnDBReset_clicked() {
    int currentRow = chatTableModel->rowCount();
    chatTableModel->removeRows(0, currentRow);
    // 重置主键
    QSqlQuery query(chatDB);
    query.exec("DELETE FROM sqlite_sequence WHERE name='chat_record'");

    chatTableModel->submitAll();
    chatTableModel->select();
}


// 目标功能：关闭服务器，回收所有资源
void Server::on_btnSerClose_clicked() {
    // 先广播服务器已关闭的消息
    broadCast(SERVER_CLOSE + QString("服务器已关闭"));
    server->close();

    for(auto& x:client_group.keys())
    {
        // 清理线程
        client_group[x]->quit();
        client_group[x]->wait();
        // 清理红黑树
        client_group.erase(client_group.find(x));
        client_name.erase((client_name.find(x)));
        delete x;
    }
    // 暂时不搞循环开关，直接退出进程
    this->close();
}

// 处理客户端的连接
void Server::handleConnect() {
    // 先获取客户端的信息
    QTcpSocket* cli = server->nextPendingConnection();

    // 从数据库获取前N条数据，发送给用户
    int n = chatTableModel->rowCount();
    QString msg;    // 缓冲
    for(int i = 0; i < n; ++i) {
        for(int j = 1; j < 4; ++j) {
            msg.push_back(chatTableModel->data(chatTableModel->index(i, j)).toString() + "  ");
        }
        msg.push_back(chatTableModel->data(chatTableModel->index(i, 4)).toString() + '\n');
    }
    cli->write((HISTORY_RECORD + msg).toUtf8());
    writeLog("已发送历史记录给该用户");

    // 将用户放入用户组，并给他一个线程
    QThread* cli_thread = new QThread();
    Worker* worker_cli = new Worker(nullptr, server, cli);

    worker_cli->moveToThread(cli_thread);
    // 处理信号和槽
    connect(worker_cli, &Worker::send_server, this, &Server::receiveCliMsg);
    connect(worker_cli, &Worker::cli_exit, this, &Server::handleDisConnect);
    connect(cli_thread, &QThread::finished, worker_cli, &Worker::deleteLater);      // 资源回收预连接
    connect(cli_thread, &QThread::finished, cli_thread, &QThread::deleteLater);     // 资源回收预连接
    connect(cli_thread, &QThread::started, worker_cli, &Worker::doWork);        // 线程启动后开始检测心跳

    // 放入红黑树并启动线程
    this->client_group.insert(cli, cli_thread);
    this->client_firstBag_set.insert(cli);
    cli_thread->start();
}

// 处理接收到的用户的数据，需要放到数据库并广播
// 需要注意第一位是标识符
void Server::receiveCliMsg(QByteArray content, QTcpSocket* cli) {
    QString msg = QString::fromUtf8(content);
    // 检查接收数据的类型,如果是心跳包，则直接略过，否则视为聊天消息进行处理
    if(msg == PUNPING_INFO) return;
    // 如果是第一个数据包，那么这个数据包是用户的name，需要存起来
    auto it = client_firstBag_set.find(cli);
    if(it != client_firstBag_set.end()) {
        int id = QRandomGenerator::global()->bounded(1000, 10000);
        QString cli_name = msg + '#' + QString::number(id);
        this->client_name.insert(cli, cli_name);
        client_firstBag_set.erase(it);
        writeLog(QString (client_name[cli] + '[' + cli->peerAddress().toString() + "] 加入了聊天室"));
        broadCast(QString(CHAT_INFO + client_name[cli] + " 加入了聊天室"));
        return;
    }
    // 为正常的聊天消息,构建该消息的结构，传到数据库
    QString time = QTime::currentTime().toString();
    QString name = client_name[cli];
    QString sender = cli->peerAddress().toString();
    addChatInfo(time, sender, name, content.mid(1));
    broadCast(content.mid(1), cli);
}

void Server::writeLog(QString text) {
    ui->logInfo->addItem(text);
}

// 不需要在调用前加聊天类型
void Server::broadCast(QByteArray content, QTcpSocket* cli) {
    QByteArray msg = (CHAT_INFO + client_name[cli] + ": ").toUtf8() + content;
    for(auto& cli_temp: client_group.keys()) {
        cli_temp->write(msg);
    }
    writeLog("已完成广播");
}

// 不需要在调用前加聊天类型
void Server::broadCast(QString content, QTcpSocket* cli) {
    QByteArray msg = (CHAT_INFO + client_name[cli] + ": " + content).toUtf8();
    for(auto& cli: client_group.keys()) {
        cli->write(msg);
    }
    writeLog("已完成广播");
}

// 这个函数只广播不处理,需要在调用前事先处理好
void Server::broadCast(QString content) {
    QByteArray msg = content.toUtf8();
    for(auto& cli: client_group.keys()) {
        cli->write(msg);
        cli->flush();
    }
    writeLog("已完成广播");
}

void Server::handleDisConnect(QTcpSocket *cli) {
    // 广播用户的下线消息
    QString content = client_name[cli] + " 已下线";
    broadCast(CHAT_INFO + content);

    // 删除在用户组的该用户的相关数据缓存
    auto it = client_group.find(cli);
    auto itIP = client_name.find(cli);
    if(it != client_group.end()) client_group.erase(it);
    if(itIP != client_name.end()) client_name.erase(itIP);

    writeLog("已完成用户下线处理");
}

void Server::on_btnLogClear_clicked() {
    ui->logInfo->clear();
    ui->logInfo->update();
}

/*
    通过获取EditLimit中的用户名，来进行kick
 */
void Server::on_btnKick_clicked() {

}

/*
    通过输入的n，来设置数据库的最大消息数
*/
void Server::on_btnSerMsgLimit_clicked() {
    int curN;
    bool inputCheck;
    curN = ui->msgNumEdit->text().toInt(&inputCheck);
    if(inputCheck == false) {
        ui->msgNumState->setText("状态：错误的输入");
    } else {
        N = curN;
        ui->msgNumState->setText("状态：操作完成");
    }
    flushDB();
}
