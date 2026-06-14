#ifndef PROJECT_CLIENT_SERVERSOCKET_H
#define PROJECT_CLIENT_SERVERSOCKET_H

#include <QTcpSocket>

class serverSocket : public QTcpSocket {
    Q_OBJECT
public:
    using QTcpSocket::QTcpSocket;
    using QAbstractSocket::setPeerName;
};


#endif //PROJECT_CLIENT_SERVERSOCKET_H
