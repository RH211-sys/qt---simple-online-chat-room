//
// Created by mechreuo on 2026/7/5.
//

#ifndef PROJECT_SERVER_FILETRANSFORMER_H
#define PROJECT_SERVER_FILETRANSFORMER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFile>
#include <QDir>
#include "../Server/server.h"

class FileTransformer : public QObject {
    Q_OBJECT
public:
    FileTransformer(QObject *parent = nullptr, Server *ser = nullptr, QTcpSocket *cli = nullptr);
    ~FileTransformer() override;

    // 和服务器连通的信号
    signals:
    void addNewSharedFile(QTcpSocket* cli_source, QString fileName);     // 添加新共享文件
    void addNewPrivateFile(QTcpSocket* cli_source, QString cli_target, QString fileName);   // 添加私发文件


public slots:
    // 槽函数
    void doReceiveFile(QTcpSocket* cli);   // 接收来自客户端的文件
    void doTransfer(QString filename);       // 将文件传送到指定客户端

private:
    Server* server;     // 指向服务端

    QTcpSocket* client;     // 指向客户端
    const QString downloadFilePath = "../fileDepot/";   // 文件接收仓库地址名
    QDir downloadFileDepot;
};


#endif //PROJECT_SERVER_FILETRANSFORMER_H
