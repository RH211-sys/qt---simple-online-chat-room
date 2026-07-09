
#ifndef PROJECT_CLIENT_FILESTRANSFERER_H
#define PROJECT_CLIENT_FILESTRANSFERER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDir>
#include <QMutex>

class serverSocket;
class Client;  // 前置声明，替代 include

class FilesTransFerer : public QObject {
    Q_OBJECT
public:
    explicit FilesTransFerer(QObject *parent = nullptr, Client *cli = nullptr, serverSocket *ser = nullptr, QRecursiveMutex* mutex = nullptr);
    ~FilesTransFerer() override;

public slots:
    void TransferSharedFile(QString filePath);
    void TransferPrivateFile(QString filePath, QString targetClientName);

private:
    Client* localClient;
    serverSocket* targetServer;
    QRecursiveMutex* socketMutex;

};


#endif //PROJECT_CLIENT_FILESTRANSFERER_H
