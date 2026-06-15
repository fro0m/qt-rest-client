#include "rest_client_test.h"

#include <QDebug>
#include <QEventLoop>
#include <QFuture>
#include <QtConcurrent>
#include <QtTest/QtTest>

namespace {
const char* const kEchoUrl = "https://postman-echo.com/get?foo1=bar1&foo2=bar2";

// Lowered from the original 10000: that many concurrent threads just exhausts
// the global thread pool and makes the test take forever without testing
// anything extra. 50 concurrent requests still exercises real concurrency.
const int kThreadCount = 50;
} // namespace

RestClientTest::RestClientTest()
{
}

void RestClientTest::initTestCase()
{
}

void RestClientTest::testSingleThreadWork()
{
    QEventLoop loop;
    auto* request = new HttpRequestInput(kEchoUrl, HttpMethod::Get);

    auto* worker = new HttpRequestWorker();
    worker->setReadResponseOnError(true);
    worker->execute(request);

    bool ok = false;

    QObject::connect(worker, &HttpRequestWorker::executionFinished,
                     [&loop, &ok, request](HttpRequestWorker* w) {
                         qDebug() << "status code" << w->statusCode;
                         if (w->statusCode == 200)
                             ok = true;
                         w->deleteLater();
                         delete request;
                         loop.quit();
                     });
    loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    QVERIFY(ok);
}

void RestClientTest::testMultithreadWork()
{
    QVector<QFuture<bool>> statuses;
    statuses.reserve(kThreadCount);

    auto function = []() -> bool {
        QEventLoop loop;
        auto* request = new HttpRequestInput(kEchoUrl, HttpMethod::Get);

        auto* worker = new HttpRequestWorker();
        worker->setReadResponseOnError(true);
        worker->execute(request);

        bool ok = false;

        QObject::connect(worker, &HttpRequestWorker::executionFinished,
                         [&loop, &ok, request](HttpRequestWorker* w) {
                             if (w->statusCode == 200)
                                 ok = true;
                             w->deleteLater();
                             delete request;
                             loop.exit(ok);
                         });
        return loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    };

    for (int i = 0; i < kThreadCount; ++i)
        statuses.append(QtConcurrent::run(function));

    bool ok = true;
    for (int i = 0; i < kThreadCount; ++i)
    {
        statuses[i].waitForFinished();
        if (!statuses[i].result())
        {
            qDebug() << "request" << i << "failed";
            ok = false;
        }
    }
    QVERIFY(ok);
}

void RestClientTest::cleanupTestCase()
{
}
