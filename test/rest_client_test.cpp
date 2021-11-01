#include "rest_client_test.h"
#include <QtTest/QtTest>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent>
#include <QList>

RestClientTest::RestClientTest()
{

}

void RestClientTest::initTestCase()
{
}

void RestClientTest::testSingleThreadWork()
{
    QEventLoop loop;
    HttpRequestInput* request =  new HttpRequestInput("https://postman-echo.com/get?foo1=bar1&foo2=bar2", Literals::getMethod);

    HttpRequestWorker* worker = new HttpRequestWorker();

    worker->setReadResponseOnError(true);
    worker->execute(request);

    bool ok = false;

    QObject::connect(worker, &HttpRequestWorker::executionFinished, [&loop, &ok, request](HttpRequestWorker * worker) mutable {
        QByteArray response = worker->response;
        qDebug() << "status code " << worker->statusCode;
        worker->deleteLater();
        delete request;
        if (worker->statusCode == 200)
        {
            ok = true;
        }
        loop.exit();
    });
    loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    QCOMPARE(ok, true);
}

void RestClientTest::testMultithreadWork()
{
    const int threadCount = 10000;
    QFuture<bool> statuses[threadCount];
    auto function = [=] () -> bool {
        QEventLoop loop;
        HttpRequestInput* request =  new HttpRequestInput("https://postman-echo.com/get?foo1=bar1&foo2=bar2", Literals::getMethod);

        HttpRequestWorker* worker = new HttpRequestWorker();

        worker->setReadResponseOnError(true);
        worker->execute(request);

        bool ok = false;

        QObject::connect(worker, &HttpRequestWorker::executionFinished, [&loop, &ok, request](HttpRequestWorker * worker) mutable {
            QByteArray response = worker->response;
            qWarning() << "status code " << worker->statusCode;
            worker->deleteLater();
            delete request;
            if (worker->statusCode == 200)
            {
                ok = true;
            }
            loop.exit(ok);
        });
        return loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    };

    for(int i = 0; i < threadCount; ++i)
    {
        statuses[i] = QtConcurrent::run(function);
    }

    for(int i = 0; i < threadCount; ++i)
    {
        statuses[i].waitForFinished();
    }

    bool ok = true;
    for(int i = 0; i < threadCount; ++i)
    {
        if (!statuses[i].result())
        {
            qDebug() << "result of " << i << " operation is false";
            ok = false;
        }
    }
    QCOMPARE(ok, true);
}

void RestClientTest::cleanupTestCase()
{
}
