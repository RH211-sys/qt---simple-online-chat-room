#include <QApplication>
#include "Server/server.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    Server win;
    win.show();

    return QApplication::exec();
}
