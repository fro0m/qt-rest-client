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

#include "HttpRequestWorker.h"

#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QRandomGenerator>
#include <QTimer>
#include <QUrl>

HttpRequestInput::HttpRequestInput(const QString& url, HttpMethod method)
    : urlStr(url), httpMethod(method)
{
}

void HttpRequestInput::addVar(const QString& key, const QByteArray& value)
{
    vars[key] = value;
}

void HttpRequestInput::addHeader(const QByteArray& header, const QByteArray& value)
{
    headers[header] = value;
}

void HttpRequestInput::addFile(const QString& variableName, const QString& localFilename,
                               const QString& requestFilename, const QByteArray& mimeType)
{
    HttpRequestInputFilePathElement file;
    file.variableName = variableName;
    file.localFilename = localFilename;
    file.requestFilename = requestFilename;
    file.mimeType = mimeType;
    files.append(file);
}

void HttpRequestInput::addFile(const QString& variableName,
                               const QByteArray& localFileData,
                               const QString& requestFilename, const QByteArray& mimeType)
{
    HttpRequestInputFileDataElement file;
    file.variableName = variableName;
    file.localFileData = localFileData;
    file.requestFilename = requestFilename;
    file.mimeType = mimeType;
    filesData.append(file);
}

HttpRequestWorker::HttpRequestWorker(QObject* parent)
    : QObject(parent), manager(new QNetworkAccessManager(this))
{
    connect(manager, &QNetworkAccessManager::finished,
            this, &HttpRequestWorker::managerFinished);
}

HttpRequestWorker::~HttpRequestWorker() = default;

QByteArray HttpRequestWorker::httpAttributeEncode(const QString& attributeName,
                                                  const QString& input)
{
    // result structure follows RFC 5987
    // Iterate over UTF-16 code points; percent-encode anything that isn't a
    // bare ASCII attr-char so the comparison ranges are never applied to a
    // signed char (which would be UB for bytes >= 0x80).
    bool needUtfEncoding = false;
    QString simple;
    simple.reserve(input.size());

    for (const QChar ch : input)
    {
        const ushort uc = ch.unicode();
        if (uc == 0)
        {
            needUtfEncoding = true;
        }
        else if (uc < 0x20 || uc > 0x7e)
        {
            needUtfEncoding = true;
        }
        else if (uc == '"')
        {
            simple += QStringLiteral("\\\"");
        }
        else
        {
            simple += ch;
        }
    }

    if (simple.isEmpty())
        needUtfEncoding = true;

    if (!needUtfEncoding)
        return QString::fromLatin1("%1=\"%2\"").arg(attributeName, simple).toLatin1();

    // Build the percent-encoded UTF-8 form for the extended syntax.
    const QByteArray utf8 = input.toUtf8();
    QString encoded;
    encoded.reserve(static_cast<int>(utf8.size() * 3));
    for (const unsigned char b : utf8)
    {
        const char c = static_cast<char>(b);
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        {
            encoded += QLatin1Char(c);
        }
        else
        {
            encoded += QLatin1Char('%');
            encoded += QString::number(b, 16).toUpper().rightJustified(2, '0');
        }
    }

    return QString::fromLatin1("%1=\"%2\"; %1*=utf-8''%3")
        .arg(attributeName, simple, encoded)
        .toLatin1();
}

void HttpRequestWorker::buildUrlEncoded(QByteArray& body, HttpRequestInput* input, bool asAddress)
{
    if (input->vars.isEmpty())
        return;

    body.reserve(body.size() + input->vars.size() * 16);
    bool first = true;
    for (auto it = input->vars.cbegin(); it != input->vars.cend(); ++it)
    {
        if (!first)
            body.append('&');
        first = false;

        body.append(QUrl::toPercentEncoding(it.key()));
        body.append('=');
        body.append(QUrl::toPercentEncoding(it.value()));
    }

    if (asAddress)
    {
        input->urlStr += '?' + QString::fromLatin1(body);
        body.clear();
    }
}

void HttpRequestWorker::appendMultipartFile(QByteArray& body, const QByteArray& boundary,
                                            const QString& variableName,
                                            const QString& requestFilename,
                                            const QByteArray& mimeType,
                                            const QByteArray& content)
{
    const QByteArray boundaryDelimiter = "--";
    const QByteArray newLine = "\r\n";

    body.append(boundaryDelimiter);
    body.append(boundary);
    body.append(newLine);

    body.append(QString("Content-Disposition: form-data; %1; %2")
                    .arg(QString::fromLatin1(httpAttributeEncode("name", variableName)),
                         QString::fromLatin1(httpAttributeEncode("filename", requestFilename)))
                    .toLatin1());
    body.append(newLine);

    if (!mimeType.isNull() && !mimeType.isEmpty())
    {
        body.append("Content-Type: ");
        body.append(mimeType);
        body.append(newLine);
    }

    body.append("Content-Transfer-Encoding: binary");
    body.append(newLine);
    body.append(newLine);

    body.append(content);
    body.append(newLine);
}

void HttpRequestWorker::buildMultipart(QByteArray& body, const QByteArray& boundary,
                                       HttpRequestInput* input)
{
    const QByteArray boundaryDelimiter = "--";
    const QByteArray newLine = "\r\n";

    // rough size hint so the body isn't reallocated on every append
    qint64 hint = input->vars.size() * 128;
    for (const auto& f : input->files)
        hint += QFileInfo(f.localFilename).size();
    for (const auto& f : input->filesData)
        hint += f.localFileData.size();
    if (hint > 0)
        body.reserve(static_cast<int>(qMin(hint, qint64(INT_MAX))));

    // add variables
    for (auto it = input->vars.cbegin(); it != input->vars.cend(); ++it)
    {
        body.append(boundaryDelimiter);
        body.append(boundary);
        body.append(newLine);

        //! The Content-Disposition response-header field has been proposed
        //! as a means for the origin server to suggest a default filename
        //! if the user requests that the content is saved to a file. This
        //! usage is derived from the definition of Content-Disposition
        //! in RFC 1806
        body.append("Content-Disposition: form-data; ");
        body.append(httpAttributeEncode("name", it.key()));
        body.append(newLine);
        body.append("Content-Type: text/plain");
        body.append(newLine);
        body.append(newLine);

        body.append(it.value());
        body.append(newLine);
    }

    // add files referenced by path
    for (const HttpRequestInputFilePathElement& fileInfo : input->files)
    {
        QFileInfo fi(fileInfo.localFilename);

        if (fileInfo.localFilename.isEmpty() || fileInfo.variableName.isEmpty() ||
            !fi.exists() || !fi.isFile() || !fi.isReadable())
        {
            qDebug() << "problem with file in request";
            continue;
        }

        QFile file(fileInfo.localFilename);
        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "could not open file in request:" << fileInfo.localFilename;
            continue;
        }

        QString requestFilename = fileInfo.requestFilename;
        if (requestFilename.isEmpty())
        {
            requestFilename = fi.fileName();
            if (requestFilename.isEmpty())
                requestFilename = QStringLiteral("file");
        }

        appendMultipartFile(body, boundary, fileInfo.variableName, requestFilename,
                            fileInfo.mimeType, file.readAll());
        file.close();
    }

    // add files supplied as raw data
    for (const HttpRequestInputFileDataElement& fileInfo : input->filesData)
    {
        if (fileInfo.localFileData.isEmpty() || fileInfo.variableName.isEmpty())
        {
            qDebug() << "problem with file data in request";
            continue;
        }

        QString requestFilename = fileInfo.requestFilename;
        if (requestFilename.isEmpty())
            requestFilename = QStringLiteral("file");

        appendMultipartFile(body, boundary, fileInfo.variableName, requestFilename,
                            fileInfo.mimeType, fileInfo.localFileData);
    }

    // closing boundary
    body.append(boundaryDelimiter);
    body.append(boundary);
    body.append(boundaryDelimiter);
}

void HttpRequestWorker::buildJson(QByteArray& body, HttpRequestInput* input)
{
    // Each variable value is parsed as JSON; if it is a valid object or
    // array it is embedded as-is, otherwise it is stored as a JSON string.
    // This means "123" becomes a number and "{\"a\":1}" becomes an object —
    // callers that want literal strings for such values should pre-encode.
    QJsonObject obj;
    for (auto it = input->vars.cbegin(); it != input->vars.cend(); ++it)
    {
        const QJsonDocument doc = QJsonDocument::fromJson(it.value());
        if (doc.isObject())
            obj[it.key()] = doc.object();
        else if (doc.isArray())
            obj[it.key()] = doc.array();
        else
            obj[it.key()] = QString::fromUtf8(it.value());
    }

    QJsonDocument doc;
    doc.setObject(obj);
    body.append(doc.toJson(QJsonDocument::Compact));
}

QNetworkReply* HttpRequestWorker::dispatch(QNetworkRequest& request, const QByteArray& body,
                                           HttpRequestInput* input)
{
    QNetworkReply* reply = nullptr;
    switch (input->httpMethod)
    {
    case HttpMethod::Get:
        reply = manager->get(request);
        break;
    case HttpMethod::Post:
        reply = manager->post(request, body);
        break;
    case HttpMethod::Put:
        reply = manager->put(request, body);
        break;
    case HttpMethod::Head:
        reply = manager->head(request);
        break;
    case HttpMethod::Delete:
        reply = manager->deleteResource(request);
        break;
    case HttpMethod::Custom:
    {
        // The buffer must outlive the call; parent it to the reply so it is
        // destroyed together with the reply rather than at scope exit.
        QBuffer* buffer = new QBuffer;
        buffer->setData(body);
        buffer->open(QIODevice::ReadOnly);
        reply = manager->sendCustomRequest(request, input->customMethod, buffer);
        if (reply)
            buffer->setParent(reply);
        else
            delete buffer;
        break;
    }
    }

    Q_ASSERT_X(reply, "HttpRequestWorker::dispatch",
               "QNetworkAccessManager returned a null reply");
    return reply;
}

QNetworkReply::NetworkError HttpRequestWorker::execute(HttpRequestInput* input)
{
    Q_ASSERT_X(input, "HttpRequestWorker::execute", "input must not be null");

    // reset result fields
    response.clear();
    errorType = QNetworkReply::NoError;
    errorStr.clear();
    statusCode = 0;

    // decide on the variable layout
    if (input->files.size() > 0 || input->filesData.size() > 0)
    {
        input->varLayout = HttpRequestVarLayout::Multipart;
    }
    else if (input->varLayout == HttpRequestVarLayout::NotSet)
    {
        input->varLayout = (input->httpMethod == HttpMethod::Get ||
                            input->httpMethod == HttpMethod::Head)
                               ? HttpRequestVarLayout::Address
                               : HttpRequestVarLayout::UrlEncoded;
    }

    // build the request body
    QByteArray body;
    QByteArray boundary;

    switch (input->varLayout)
    {
    case HttpRequestVarLayout::Address:
        buildUrlEncoded(body, input, /*asAddress=*/true);
        break;
    case HttpRequestVarLayout::UrlEncoded:
        buildUrlEncoded(body, input, /*asAddress=*/false);
        break;
    case HttpRequestVarLayout::Multipart:
    {
        // random-ish boundary; the leading -- is part of the delimiter, the
        // extra dashes just make it unlikely to collide with body content
        boundary = "-------------------------" +
                   QString::number(QDateTime::currentSecsSinceEpoch()).toLatin1() +
                   QString::number(QRandomGenerator::system()->generate()).toLatin1();
        buildMultipart(body, boundary, input);
        break;
    }
    case HttpRequestVarLayout::Json:
        buildJson(body, input);
        break;
    case HttpRequestVarLayout::NotSet:
        break;
    }

    // prepare the connection
    const QUrl url(input->urlStr);
    if (!url.isValid())
    {
        errorType = QNetworkReply::UnknownNetworkError;
        errorStr = QStringLiteral("Invalid URL: ") + input->urlStr;
        QTimer::singleShot(0, this, [this] { emit executionFinished(this); });
        return errorType;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("qt-rest-client"));

    switch (input->varLayout)
    {
    case HttpRequestVarLayout::UrlEncoded:
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          QStringLiteral("application/x-www-form-urlencoded"));
        break;
    case HttpRequestVarLayout::Json:
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        break;
    case HttpRequestVarLayout::Multipart:
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          "multipart/form-data; boundary=" + boundary);
        break;
    default:
        break;
    }

    // apply caller-supplied headers
    for (auto it = input->headers.cbegin(); it != input->headers.cend(); ++it)
        request.setRawHeader(it.key(), it.value());

    QNetworkReply* reply = dispatch(request, body, input);
    if (!reply)
    {
        errorType = QNetworkReply::UnknownNetworkError;
        errorStr = QStringLiteral("Could not start the request");
        emit executionFinished(this);
        return errorType;
    }

    return reply->error();
}

void HttpRequestWorker::setReadResponseOnError(bool readResponseOnError)
{
    m_readResponseOnError = readResponseOnError;
}

void HttpRequestWorker::setReadErrorString(bool readErrorString)
{
    m_readErrorString = readErrorString;
}

void HttpRequestWorker::managerFinished(QNetworkReply* reply)
{
    Q_ASSERT_X(reply, "HttpRequestWorker::managerFinished", "reply must not be null");
    if (!reply)
    {
        emit executionFinished(this);
        return;
    }

    statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    errorType = reply->error();
    if (errorType == QNetworkReply::NoError || m_readResponseOnError)
        response = reply->readAll();
    if (m_readErrorString)
        errorStr = reply->errorString();

    reply->deleteLater();
    emit executionFinished(this);
}
