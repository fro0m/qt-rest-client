#include <QCoreApplication>
#include <QtTest>
#include "rest_client_test.h"

int main(int argc, char * argv[]) {
	QCoreApplication a(argc, argv);

    RestClientTest test;
    return QTest::qExec(&test, argc, argv);
}
