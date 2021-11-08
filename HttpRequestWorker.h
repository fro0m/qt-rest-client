/****************************************************************************
**
** Copyright (C) 2007-2015 Speedovation
** Copyright (C) 2015 creativepulse.gr
** Contact: Speedovation Lab (info@speedovation.com)
**
** KineticWing IDE CrashHandler
** http:// kineticwing.com
** This file is part of the core classes of the KiWi Editor (IDE)
**
** Author: creativepulse.gr
**   https://github.com/codeyash
**   https://github.com/fro0m
** License : Apache License 2.0
**
** All rights are reserved.
*/

#ifndef HTTPREQUESTWORKER_H
#define HTTPREQUESTWORKER_H

#include <QMap>
#include <QNetworkReply>
#include <QObject>
#include <QString>

class QNetworkAccessManager;

namespace Literals
{
static const QString getMethod = QStringLiteral("GET");
static const QString postMethod = QStringLiteral("POST");
static const QString putMethod = QStringLiteral("PUT");
static const QString headMethod = QStringLiteral("HEAD");
static const QString deleteMethod = QStringLiteral("DELETE");
} // namespace Literals

enum HttpRequestVarLayout
{
    NOT_SET,
    ADDRESS,
    URL_ENCODED,
    JSON,
    MULTIPART
};

class HttpRequestInputFilePathElement
{
public:
    QString variableName;
    QString localFilename;
    QString requestFilename;
    QByteArray mimeType;
};

class HttpRequestInputFileDataElement
{
public:
    QString variableName;
    QByteArray localFileData;
    QString requestFilename;
    QByteArray mimeType;
};

class HttpRequestInput
{
public:
    QString urlStr;
    // qint32 port = -1;
    QString httpMethod;
    HttpRequestVarLayout varLayout;
    QMap<QString, QByteArray> vars;
    QMap<QByteArray, QByteArray> headers;
    QList<HttpRequestInputFilePathElement> files;
    QList<HttpRequestInputFileDataElement> filesData;

    explicit HttpRequestInput(const QString& v_urlStr = "",
                              const QString& v_httpMethod = Literals::getMethod);

    void addVar(const QString& key, const QByteArray& value);
    void addHeader(const QByteArray& header, const QByteArray& value);
    void addFile(const QString& variableName, const QString& localFilename,
                 const QString& requestFilename, const QByteArray& mimeType);
    void addFile(const QString& variableName, const QByteArray& localFileData,
                 const QString& requestFilename, const QByteArray& mimeType);
};

class HttpRequestWorker : public QObject
{
    Q_OBJECT

public:
    explicit HttpRequestWorker(QObject* parent = nullptr);
    ~HttpRequestWorker();
    QByteArray response;
    QNetworkReply::NetworkError errorType;
    QString errorStr;
    int statusCode; // HTTP status code

    QByteArray httpAttributeEncode(const QString& attribute_name, const QString& input);
    QNetworkReply::NetworkError execute(HttpRequestInput* input);

    void setReadResponseOnError(bool t_readResponseOnError);
    void setReadErrorString(bool readErrorString);
signals:
    void executionFinished(HttpRequestWorker* worker);

private:
    QNetworkAccessManager* manager;
    bool m_readResponseOnError = false;
    bool m_readErrorString = false;

private slots:
    void managerFinished(QNetworkReply* reply);
};

#endif // HTTPREQUESTWORKER_H
