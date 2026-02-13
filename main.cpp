#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qputenv("QT_FATAL_WARNINGS", "1");
    Widget w;
    w.show();
    return a.exec();
}
