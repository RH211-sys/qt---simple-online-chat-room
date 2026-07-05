//
// Created by mechreuo on 2026/7/5.
//

#ifndef PROJECT_SERVER_FILETRANSFORMER_H
#define PROJECT_SERVER_FILETRANSFORMER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include "../Server/server.h"

class FileTransformer : public QObject {
    Q_OBJECT
public:
    FileTransformer(QObject *parent = nullptr, QTcpServer *ser = nullptr, QTcpSocket *cli = nullptr);
    ~FileTransformer() override;
public slots:
    // 槽函数
    void doReceiveFile(const char* dir);   // 接收来自客户端的文件
    void doTransfer(const char* dir, QTcpSocket* cli);       // 将文件传送到指定客户端

private:
    QTcpServer* server;     // 指向服务端
    QTcpSocket* client;     // 指向客户端
    QString name;           // 客户端名称(还未写客户端，暂时没有途径获取测试数据)
    const char* downloadFileDir = "../fileDepot/";   // 文件接收仓库
};


#endif //PROJECT_SERVER_FILETRANSFORMER_H
