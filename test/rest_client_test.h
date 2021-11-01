#pragma once

#include "HttpRequestWorker.h"

#include <QObject>
#include <QStringList>
#include <QSignalSpy>
#include <QStandardPaths>

/**
 * Тест подсистемы Core::ServiceManager
 *
 * Чек-лист:
 * - выдача сигнала о создании очередного сервиса;
 * - выдачи сигнала о завершении создания сервисов;
 * - выборка заведомо созданного сервиса;
 * - выборка заведомо несозданного сервиса;
 * - освобождение сервисов.
 */
class RestClientTest : public QObject
{
    Q_OBJECT
public:

    RestClientTest();

private slots:
    /**
     * Инициализация теста:
     * - создание сервисов,
     * - подключение сигналов,
     * - повысылка сигнала creationCompleted().
     */
    void initTestCase();

    void testSingleThreadWork();
    void testMultithreadWork();

    /**
     * Деинициализация:
     * - проверка сигналов created();
     * - освобождение менеджера сервисов.
     * - проверка фактического удаления сервисов.
     */
    void cleanupTestCase();
};
