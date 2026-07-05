//
//
//

#include "fileTransformer.h"

FileTransformer::FileTransformer(QObject *parent, QTcpServer *ser, QTcpSocket *cli) {
    server = ser;
    client = cli;
    connect(ser, &Server::transfer, this, &FileTransformer::read_msg_cli);
}

FileTransformer::~FileTransformer() {

}

void FileTransformer::doReceiveFile(const char *dir) {

}

void FileTransformer::doTransfer(const char *dir, QTcpSocket *cli) {

}
