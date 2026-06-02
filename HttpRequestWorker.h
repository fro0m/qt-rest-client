/****************************************************************************
**
** Copyright (C) 2007-2015 Speedovation
** Copyright (C) 2015 creativepulse.gr
** Copyright (C) qt-rest-client contributors
**
** A lightweight REST/HTTP client built on QNetworkAccessManager.
**
** Authors:
**   creativepulse.gr
**   https://github.com/codeyash
**   https://github.com/fro0m
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** See the LICENSE file for details.
**
** All rights are reserved.
*/

#ifndef HTTPREQUESTWORKER_H
#define HTTPREQUESTWORKER_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QNetworkReply>
#include <QObject>
#include <QString>

class QNetworkAccessManager;

/**
 * @brief HTTP verbs supported by HttpRequestWorker.
 *
 * Use @ref Custom together with HttpRequestInput::customMethod to send an
 * uncommon verb (PATCH, CONNECT, ...). The well-known verbs are covered by
 * the named enumerators and are dispatched without any string comparison.
 */
enum class HttpMethod
{
    Get,     ///< GET
    Post,    ///< POST
    Put,     ///< PUT
    Head,    ///< HEAD
    Delete,  ///< DELETE
    Custom   ///< verb taken from HttpRequestInput::customMethod
};

/**
 * @brief Where the request variables end up in the outgoing request.
 *
 * When left as @ref NotSet, HttpRequestWorker::execute() picks a sensible
 * default based on the HTTP method and on whether files were attached.
 */
enum class HttpRequestVarLayout
{
    NotSet,      ///< let execute() decide
    Address,     ///< append vars to the URL query string
    UrlEncoded,  ///< application/x-www-form-urlencoded body
    Json,        ///< application/json body
    Multipart    ///< multipart/form-data body (forced when files are attached)
};

/**
 * @brief A file to upload, referenced by path.
 *
 * The file is read from disk at request time; if it cannot be opened the
 * part is skipped (a debug message is emitted).
 */
class HttpRequestInputFilePathElement
{
public:
    QString variableName;    ///< form field name
    QString localFilename;   ///< path read from disk
    QString requestFilename; ///< filename advertised to the server
    QByteArray mimeType;     ///< Content-Type for this part (optional)
};

/**
 * @brief A file to upload, supplied as an in-memory byte array.
 */
class HttpRequestInputFileDataElement
{
public:
    QString variableName;     ///< form field name
    QByteArray localFileData; ///< raw file contents
    QString requestFilename;  ///< filename advertised to the server
    QByteArray mimeType;      ///< Content-Type for this part (optional)
};

/**
 * @brief Describes a single HTTP request.
 *
 * Fill in the url/method, then add variables, headers and optional files and
 * hand the object to HttpRequestWorker::execute(). The object is not modified
 * by execute() apart from urlStr (when the layout is @ref Address the encoded
 * vars are appended to it) and varLayout (auto-selected when NotSet).
 */
class HttpRequestInput
{
public:
    QString urlStr;
    HttpMethod httpMethod = HttpMethod::Get;
    QByteArray customMethod; ///< verb used when httpMethod == HttpMethod::Custom
    HttpRequestVarLayout varLayout = HttpRequestVarLayout::NotSet;

    QMap<QString, QByteArray> vars;
    QMap<QByteArray, QByteArray> headers;
    QList<HttpRequestInputFilePathElement> files;
    QList<HttpRequestInputFileDataElement> filesData;

    explicit HttpRequestInput(const QString& urlStr = {},
                              HttpMethod httpMethod = HttpMethod::Get);

    void addVar(const QString& key, const QByteArray& value);
    void addHeader(const QByteArray& header, const QByteArray& value);
    void addFile(const QString& variableName, const QString& localFilename,
                 const QString& requestFilename, const QByteArray& mimeType);
    void addFile(const QString& variableName, const QByteArray& localFileData,
                 const QString& requestFilename, const QByteArray& mimeType);
};

/**
 * @brief Executes an HttpRequestInput and exposes the result.
 *
 * The request runs asynchronously; connect to executionFinished() to be
 * notified. The result fields (response, errorType, errorStr, statusCode) are
 * populated when the signal fires.
 *
 * Each worker owns its own QNetworkAccessManager, so workers are independent
 * of each other and can be used from different threads (one worker per
 * thread).
 */
class HttpRequestWorker : public QObject
{
    Q_OBJECT

public:
    explicit HttpRequestWorker(QObject* parent = nullptr);
    ~HttpRequestWorker() override;

    QByteArray response;                       ///< raw response body
    QNetworkReply::NetworkError errorType = QNetworkReply::NoError;
    QString errorStr;                          ///< reply->errorString() (see setReadErrorString)
    int statusCode = 0;                        ///< HTTP status code, 0 if none

    QByteArray httpAttributeEncode(const QString& attributeName, const QString& input);

    /**
     * @brief Start the request described by @p input.
     *
     * Returns NoError once the request has been successfully scheduled. The
     * real outcome (network error, HTTP status, body) arrives later via the
     * executionFinished() signal. A null pointer or a null reply (possible
     * when the access manager is shutting down) is reported as
     * QNetworkReply::UnknownNetworkError.
     */
    QNetworkReply::NetworkError execute(HttpRequestInput* input);

    void setReadResponseOnError(bool readResponseOnError);
    void setReadErrorString(bool readErrorString);

signals:
    /**
     * @brief Emitted once the reply has been fully processed.
     * @param worker this worker (same as the receiver of the signal).
     *
     * Read response/errorType/errorStr/statusCode from @p worker in a direct
     * (or queued) connection.
     */
    void executionFinished(HttpRequestWorker* worker);

private:
    QNetworkAccessManager* manager = nullptr;
    bool m_readResponseOnError = false;
    bool m_readErrorString = true;

    void buildUrlEncoded(QByteArray& body, HttpRequestInput* input, bool asAddress);
    void buildMultipart(QByteArray& body, const QByteArray& boundary, HttpRequestInput* input);
    void buildJson(QByteArray& body, HttpRequestInput* input);
    void appendMultipartFile(QByteArray& body, const QByteArray& boundary,
                             const QString& variableName, const QString& requestFilename,
                             const QByteArray& mimeType, const QByteArray& content);
    QNetworkReply* dispatch(QNetworkRequest& request, const QByteArray& body,
                            HttpRequestInput* input);

private slots:
    void managerFinished(QNetworkReply* reply);
};

#endif // HTTPREQUESTWORKER_H
