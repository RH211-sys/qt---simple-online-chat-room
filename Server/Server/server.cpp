#include "server.h"
#include "ui_server.h"
#include "../Worker/worker.h"
#include "../Worker/fileTransformer.h"
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
 * 这里把加客户端和减客户端单独封装函数的原因如下：
 * 1. 解耦：因为很多其他函数都要调这个，在工程变大后如果其他函数的++和--写错了会比较难找，而这里可以直接把问题定位在这个小模块
 * 2. 扩展性：后续可能会有 +x后出现什么事件，-x后出现什么事件，这样写可以增强工程的扩展性(虽然目前来看用不到)
 * 3. 方便调试
 * */
void Server::cli_cnt_change(int x) {
    cli_cnt += x;
    ui->curChatersCnt->setText("当前在线人数：" + QString::number(cli_cnt));
}


/*
 * 构造函数
 * 各种初始化，完成服务器初始化工作
 */
Server::Server(QWidget *parent) : QWidget(parent), ui(new Ui::Server) {
    ui->setupUi(this);
    cli_cnt = 0;
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
    // 绑定在线用户
    chaterModel = new QStringListModel(this);
    ui->logChatersInfo->setModel(chaterModel);

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

// 回收离开聊天室的客户端的资源
void Server::cleanCli(QTcpSocket* cli) {
    // 清理线程
    client_group[cli]->quit();
    client_group[cli]->wait();

    //清理红黑树
    auto its = client_group.find(cli);
    auto itIP = client_name.find(cli);
    auto reflect = name_to_ip.find(client_name[cli]);

    if(its != client_group.end()) client_group.erase(its);
    if(itIP != client_name.end()) client_name.erase(itIP);
    if(reflect != name_to_ip.end()) name_to_ip.erase(reflect);
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
        // 清理客户端
        cleanCli(x);
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
    for(int i = 0; i < n; ++i) {
        QString msg;    // 缓冲
        for(int j = 1; j < 4; ++j) {
            msg.push_back(chatTableModel->data(chatTableModel->index(i, j)).toString() + "  ");
        }
        msg.push_back(chatTableModel->data(chatTableModel->index(i, 4)).toString());
        cli->write((HISTORY_RECORD + msg + INTERUPT).toUtf8());
    }
    writeLog("已发送历史记录给该用户");

    // 发送当前存在的用户名
    for(auto& name: name_to_ip.keys()) {
        QString msg;
        msg.push_back(USER_INIT + name + INTERUPT);
        cli->write(msg.toUtf8());
    }
    writeLog("已发送目前在线客户端给该用户");


    // 将用户放入用户组，并给他一个线程
    QThread* cli_thread = new QThread();
    Worker* worker_cli = new Worker(nullptr, server, cli);
    FileTransformer* fileTrans_cli = new FileTransformer(nullptr, this, cli);     // 文件传输模块的Worker


    // 移到线程上
    worker_cli->moveToThread(cli_thread);
    fileTrans_cli->moveToThread(cli_thread);

    // 处理信号和槽
    connect(worker_cli, &Worker::send_server, this, &Server::receiveCliMsg);
    connect(worker_cli, &Worker::cli_exit, this, &Server::handleDisConnect);
    connect(cli_thread, &QThread::finished, worker_cli, &Worker::deleteLater);      // 资源回收预连接
    connect(cli_thread, &QThread::finished, cli_thread, &QThread::deleteLater);     // 资源回收预连接
    connect(cli_thread, &QThread::finished, fileTrans_cli, &FileTransformer::deleteLater);  // 资源回收预连接
    connect(cli_thread, &QThread::started, worker_cli, &Worker::doWork);        // 线程启动后开始检测心跳
    // 文件模块连接
    connect(fileTrans_cli, &FileTransformer::addNewPrivateFile, this, &Server::addNewPrivateFile);
    connect(fileTrans_cli, &FileTransformer::addNewSharedFile, this, &Server::addNewSharedFile);

    // 连接文件传输的信号和槽
    connect(this, &Server::receiveFile, fileTrans_cli, &FileTransformer::doReceiveFile);

    // 放入红黑树并启动线程
    this->client_group.insert(cli, cli_thread);
    this->client_firstBag_set.insert(cli);
    cli_thread->start();

    // 增客户端调用
    cli_cnt_change(1);
}

// 处理接收到的用户的数据，需要放到数据库并广播
// 需要注意第一位是标识符(flag)
void Server::receiveCliMsg(QByteArray _flag, QTcpSocket* cli) {
    QString flag = QString::fromUtf8(_flag);
    // 检查接收数据的类型,如果是心跳包，则直接略过，否则视为聊天消息进行处理
    if(flag == PUNPING_INFO) return;
    // 如果是第一个数据包，那么这个数据包是用户的name，需要存起来
    auto it = client_firstBag_set.find(cli);
    if(it != client_firstBag_set.end()) {
        // 注册用户名+随机值
        QString name = cli->readAll();
        int id = QRandomGenerator::global()->bounded(1000, 10000);
        QString cli_name = name + '#' + QString::number(id);
        // 插入到红黑树和缓存
        this->client_name.insert(cli, cli_name);
        client_firstBag_set.erase(it);
        name_to_ip.insert(cli_name, cli);
        // 更新服务端的列表ui
        chaterList.append(client_name[cli]);
        chaterModel->setStringList(chaterList);
        // 写入日志并广播
        writeLog(QString (client_name[cli] + '[' + cli->peerAddress().toString() + "] 加入了聊天室"));
        broadCast(QString(USER_UPDATE + client_name[cli]));     // 客户端那边自己更新
        return;
    }
    if(flag == CHAT_INFO) {
        // 为正常的聊天广播消息,构建该消息的结构，传到数据库
        QString msg = cli->readAll();
        QString time = QTime::currentTime().toString();
        QString name = client_name[cli];
        QString sender = cli->peerAddress().toString();
        addChatInfo(time, sender, name, msg.mid(1));
        broadCast(msg.mid(1), cli);
    } else if(flag == PRIVATE_SEND_REQUEST) {
        // 为私发消息
        QString msg = cli->readAll();
        QString text = msg.mid(1);          // 去掉协议号
        int pos = text.indexOf(INTERUPT);
        QString name = text.left(pos);      // INTERUPT 前面的用户名或IP
        QString content = text.mid(pos + 1); // INTERUPT 后面的消息内容

        // 查看是否存在该用户，若存在则转发，否则返回错误码
        if(name_to_ip.find(name) != name_to_ip.end()) {
            tellPointedUser(name + " 悄悄的告诉你: " + content, name_to_ip[name], PRIVATE_MSG);
            tellPointedUser("", cli, SEARCH_SUCCESS);
        } else {
            tellPointedUser("", cli, SEARCH_FAILED);
        }
    } else if (flag == FILE_TRANSFER_REQUEST) {
        // 如果是文件传输协议，则交给子线程处理
        cli->read(1);    // 消费掉这个表示符，因为之前获取的标识符是读方式
        emit receiveFile(cli);


    }
}

void Server::writeLog(QString text) {
    ui->logInfo->addItem(text);
}

// 服务器向指定客户端发送数据
void Server::tellPointedUser(QString content, QTcpSocket *cli, QString state) {
    QByteArray msg = (state + content + INTERUPT).toUtf8();
    cli->write(msg);
}

// 不需要在调用前加聊天类型
void Server::broadCast(QByteArray content, QTcpSocket* cli) {
    QByteArray msg = (CHAT_INFO + client_name[cli] + ": ").toUtf8() + content;
    for(auto& cli_temp: client_group.keys()) {
        cli_temp->write(msg + INTERUPT);
    }
    writeLog("已完成广播");
}

// 不需要在调用前加聊天类型
void Server::broadCast(QString content, QTcpSocket* cli) {
    QByteArray msg = (CHAT_INFO + client_name[cli] + ": " + content).toUtf8();
    for(auto& cli: client_group.keys()) {
        cli->write(msg + INTERUPT);
    }
    writeLog("已完成广播");
}

// 这个函数只广播不处理,需要在调用前事先处理好
void Server::broadCast(QString content) {
    QByteArray msg = content.toUtf8();
    for(auto& cli: client_group.keys()) {
        cli->write(msg + INTERUPT);
        cli->flush();
    }
    writeLog("已完成广播");
}

void Server::handleDisConnect(QTcpSocket *cli) {

    // 获取用户名
    QString user = client_name[cli];

    // 广播用户的下线消息
    broadCast(USER_UPDATE_OFF + user);

    // 删除在用户组的该用户的相关数据缓存
    cleanCli(cli);

    writeLog("已完成用户下线处理");

    // 减客户端调用
    chaterList.removeOne(user);
    chaterModel->setStringList(chaterList);
    cli_cnt_change(-1);
}

void Server::on_btnLogClear_clicked() {
    ui->logInfo->clear();
    ui->logInfo->update();
}

/*
    通过获取EditLimit中的用户名，来进行kick
 */
void Server::on_btnKick_clicked() {
    QString name = ui->kickPerson->text();
    if(name_to_ip.find(name) == name_to_ip.end()) {
        // 没有找到要踢的人
        ui->kickState->setText("状态：没有找到人");
        return;
    }
    // 获取这个用户的ip,然后告诉他他被踢了
    QTcpSocket* it = name_to_ip[name];
    tellPointedUser("服务器通知：你已被服务端踢出聊天室", it, SERVER_KICK);
    // 清理it
    cleanCli(it);


    ui->kickState->setText("状态：已踢出");
    chaterList.removeOne(name);
    chaterModel->setStringList(chaterList);

    // 广播所有人
    QString content = CHAT_INFO + name + "已被踢出了聊天室！！！";
    broadCast(USER_UPDATE_OFF + name);
    broadCast(content);
    cli_cnt_change(-1);
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

/* ========================= 文件传输模块(服务器) ======================== */

void Server::addNewSharedFile(QTcpSocket *cli_source, QString fileName) {
    sharedFiles.insert(fileName);
    tellPointedUser("文件已发送至服务器", cli_source, CHAT_INFO);
    QString msg = "ok";
    broadCast(FILE_TRANSFER_RESULT + msg);
}

void Server::addNewPrivateFile(QTcpSocket *cli_source, QString cliTargetName, QString fileName) {
    QTcpSocket* cli_target = name_to_ip[cliTargetName];
    privateFiles[cli_target].insert(fileName);
    tellPointedUser("文件已发送至服务器", cli_source, CHAT_INFO);
    QString msg = "ok";
    tellPointedUser("ok", cli_source, FILE_TRANSFER_RESULT);
}




