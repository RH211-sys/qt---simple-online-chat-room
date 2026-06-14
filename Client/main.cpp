#include <QApplication>
#include "Client/client.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    Client win;
    win.show();

    return QApplication::exec();
}
