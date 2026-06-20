// Minimal example: GET https://postman-echo.com/get and print the response.
//
// The previous main.cpp referenced a MainWindow class that was never part of
// this project, so it couldn't compile. This is a small, self-contained
// console example that actually exercises the library.

#include "HttpRequestWorker.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    HttpRequestInput input(QStringLiteral("https://postman-echo.com/get?foo1=bar1&foo2=bar2"),
                           HttpMethod::Get);

    HttpRequestWorker worker;
    worker.setReadResponseOnError(true);
    worker.execute(&input);

    QEventLoop loop;
    QObject::connect(&worker, &HttpRequestWorker::executionFinished,
                     [&loop](HttpRequestWorker* w) {
                         qDebug().noquote() << "status:" << w->statusCode;
                         qDebug().noquote() << "body:" << w->response;
                         if (w->errorType != QNetworkReply::NoError)
                             qDebug().noquote() << "error:" << w->errorStr;
                         loop.quit();
                     });
    loop.exec();

    return worker.errorType == QNetworkReply::NoError ? 0 : 1;
}
