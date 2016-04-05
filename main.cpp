#include "mainwindowe.h"
#include "HttpRequestWorker.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindowe w;
    w.show();

    return a.exec();
}
