#pragma once

#include "HttpRequestWorker.h"

#include <QObject>
#include <QSignalSpy>
#include <QStringList>

/**
 * Functional smoke tests for HttpRequestWorker.
 *
 * Hits the public postman-echo.com echo endpoint, so the suite requires
 * network access and is not strictly hermetic.
 */
class RestClientTest : public QObject
{
    Q_OBJECT
public:
    RestClientTest();

private slots:
    /** Runs once before the first test. */
    void initTestCase();

    /** Sends a single GET and checks for HTTP 200. */
    void testSingleThreadWork();

    /** Fires several requests concurrently and checks each returns 200. */
    void testMultithreadWork();

    /** Runs once after the last test. */
    void cleanupTestCase();
};
