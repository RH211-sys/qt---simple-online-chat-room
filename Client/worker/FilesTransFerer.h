
#ifndef PROJECT_CLIENT_FILESTRANSFERER_H
#define PROJECT_CLIENT_FILESTRANSFERER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDir>

class serverSocket;
class Client;  // 前置声明，替代 include

class FilesTransFerer : public QObject {
    Q_OBJECT
public:
    explicit FilesTransFerer(QObject *parent = nullptr, Client *cli = nullptr, serverSocket *ser = nullptr);
    ~FilesTransFerer() override;

public slots:
    void TransferSharedFile(QFileInfo& info);
    void TransferPrivateFile(QFileInfo& info, QString targetClientName);

private:
    Client* localClient;
    serverSocket* targetServer;

};


#endif //PROJECT_CLIENT_FILESTRANSFERER_H
