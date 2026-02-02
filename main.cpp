#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Widget w;
    qRegisterMetaType<QByteArray>("QByteArray");

    w.show();
    return a.exec();
}
