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
**          https://github.com/codeyash
**          https://github.com/fro0m
** License : Apache License 2.0
**
** All rights are reserved.
*/

#include "HttpRequestWorker.h"

#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QRandomGenerator>
#include <QUrl>

HttpRequestInput::HttpRequestInput(const QString& v_urlStr, const QString& v_httpMethod)
    : urlStr(v_urlStr), httpMethod(v_httpMethod), varLayout(NOT_SET)

{}

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

HttpRequestWorker::HttpRequestWorker(QObject* parent) : QObject(parent), manager(NULL)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this,
            SLOT(managerFinished(QNetworkReply*)));
}

HttpRequestWorker::~HttpRequestWorker()
{}

QByteArray HttpRequestWorker::httpAttributeEncode(const QString& attribute_name,
                                                  const QString& input)
{
    // result structure follows RFC 5987
    bool need_utf_encoding = false;
    QString result = "";
    QByteArray input_c = input.toLocal8Bit();
    char c;
    for (int i = 0; i < input_c.length(); i++)
    {
        c = input_c.at(i);
        if (c == '\\' || c == '/' || c == '\0' || c < ' ' || c > '~')
        {
            // ignore and request utf-8 version
            need_utf_encoding = true;
        }
        else if (c == '"')
        {
            result += "\\\"";
        }
        else
        {
            result += c;
        }
    }

    if (result.length() == 0)
    {
        need_utf_encoding = true;
    }

    if (!need_utf_encoding)
    {
        // return simple version
        return QString("%1=\"%2\"").arg(attribute_name, result).toLatin1();
    }

    QString result_utf8 = "";
    for (int i = 0; i < input_c.length(); i++)
    {
        c = input_c.at(i);
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        {
            result_utf8 += c;
        }
        else
        {
            result_utf8 +=
                "%" +
                QString::number(static_cast<unsigned char>(input_c.at(i)), 16).toUpper();
        }
    }

    // return enhanced version with UTF-8 support
    return QString("%1=\"%2\"; %1*=utf-8''%3")
        .arg(attribute_name, result, result_utf8)
        .toLatin1();
}

QNetworkReply::NetworkError HttpRequestWorker::execute(HttpRequestInput* input)
{
    // reset variables

    QByteArray requestContent = "";
    response = "";
    errorType = QNetworkReply::NoError;
    errorStr = "";

    // decide on the variable layout

    if (input->files.length() > 0 || input->filesData.length() > 0)
    {
        input->varLayout = MULTIPART;
    }
    else if (input->varLayout == NOT_SET)
    {
        input->varLayout = input->httpMethod == Literals::getMethod ||
                                   input->httpMethod == Literals::headMethod
                               ? ADDRESS
                               : URL_ENCODED;
    }

    // prepare request content

    QByteArray boundary = "";

    if (input->varLayout == ADDRESS || input->varLayout == URL_ENCODED)
    {
        // variable layout is ADDRESS or URL_ENCODED

        if (input->vars.count() > 0)
        {
            bool first = true;
            foreach (QString key, input->vars.keys())
            {
                if (!first)
                {
                    requestContent.append("&");
                }
                first = false;

                requestContent.append(QUrl::toPercentEncoding(key));
                requestContent.append("=");
                requestContent.append(QUrl::toPercentEncoding(input->vars.value(key)));
            }

            if (input->varLayout == ADDRESS)
            {
                input->urlStr += "?" + requestContent;
                requestContent = "";
            }
        }
    }
    else if (input->varLayout == MULTIPART)
    {
        // variable layout is MULTIPART

        boundary = "__-----------------------" +
                   QString::number(QDateTime::currentDateTime().toTime_t()).toLatin1() +
                   QString::number(QRandomGenerator::system()->generate()).toLatin1();
        QByteArray boundaryDelimiter = "--";
        QByteArray newLine = "\r\n";

        // add variables
        foreach (QString key, input->vars.keys())
        {
            // add boundary
            requestContent.append(boundaryDelimiter);
            requestContent.append(boundary);
            requestContent.append(newLine);

            // add header
            //! The Content-Disposition response-header field has been proposed
            //! as a means for the origin server to suggest a default filename
            //! if the user requests that the content is saved to a file. This
            //! usage is derived from the definition of Content-Disposition
            //! in RFC 1806
            requestContent.append("Content-Disposition: form-data; ");
            requestContent.append(httpAttributeEncode("name", key));
            requestContent.append(newLine);
            requestContent.append("Content-Type: text/plain");
            requestContent.append(newLine);

            // add header to body splitter
            requestContent.append(newLine);

            // add variable content
            requestContent.append(input->vars.value(key));
            requestContent.append(newLine);
        }

        // add files from paths
        for (QList<HttpRequestInputFilePathElement>::iterator fileInfo =
                 input->files.begin();
             fileInfo != input->files.end(); ++fileInfo)
        {
            QFileInfo fi(fileInfo->localFilename);

            // ensure necessary variables are available
            if (
                /*fileInfo->localFilename == NULL ||*/ fileInfo->localFilename
                    .isEmpty() ||
                fileInfo->variableName == NULL || fileInfo->variableName.isEmpty() ||
                !fi.exists() || !fi.isFile() || !fi.isReadable())
            {
                // silent abort for the current file
                qDebug() << "problem with file in request";
                continue;
            }

            QFile file(fileInfo->localFilename);
            if (!file.open(QIODevice::ReadOnly))
            {
                // silent abort for the current file
                continue;
            }

            // ensure filename for the request
            if (/*fileInfo->requestFilename == NULL ||*/ fileInfo->requestFilename
                    .isEmpty())
            {
                fileInfo->requestFilename = fi.fileName();
                if (fileInfo->requestFilename.isEmpty())
                {
                    fileInfo->requestFilename = "file";
                }
            }

            // add boundary
            requestContent.append(boundaryDelimiter);
            requestContent.append(boundary);
            requestContent.append(newLine);

            // add header
            requestContent.append(
                QString("Content-Disposition: form-data; %1; %2")
                    .arg(httpAttributeEncode("name", fileInfo->variableName),
                         httpAttributeEncode("filename", fileInfo->requestFilename))
                    .toLatin1());
            requestContent.append(newLine);

            if (!(fileInfo->mimeType.isNull() || fileInfo->mimeType.isEmpty()))
            {
                requestContent.append("Content-Type: ");
                requestContent.append(fileInfo->mimeType);
                requestContent.append(newLine);
            }

            requestContent.append("Content-Transfer-Encoding: binary");
            requestContent.append(newLine);

            // add header to body splitter
            requestContent.append(newLine);

            // add file content
            requestContent.append(file.readAll());
            requestContent.append(newLine);

            file.close();
        }
        // add files from files data
        for (QList<HttpRequestInputFileDataElement>::iterator fileInfo =
                 input->filesData.begin();
             fileInfo != input->filesData.end(); ++fileInfo)
        {
            // ensure necessary variables are available
            if (fileInfo->localFileData.isEmpty() || fileInfo->variableName == NULL ||
                fileInfo->variableName.isEmpty())
            {
                // silent abort for the current file
                qDebug() << "problem with file in request";
                continue;
            }

            // ensure filename for the request
            if (fileInfo->requestFilename == NULL)
            {
                if (fileInfo->requestFilename.isEmpty())
                {
                    fileInfo->requestFilename = "file";
                }
            }

            // add boundary
            requestContent.append(boundaryDelimiter);
            requestContent.append(boundary);
            requestContent.append(newLine);

            // add header
            requestContent.append(
                QString("Content-Disposition: form-data; %1; %2")
                    .arg(httpAttributeEncode("name", fileInfo->variableName),
                         httpAttributeEncode("filename", fileInfo->requestFilename))
                    .toLatin1());
            requestContent.append(newLine);

            if (!(fileInfo->mimeType.isNull() || fileInfo->mimeType.isEmpty()))
            {
                requestContent.append("Content-Type: ");
                requestContent.append(fileInfo->mimeType);
                requestContent.append(newLine);
            }

            requestContent.append("Content-Transfer-Encoding: binary");
            requestContent.append(newLine);

            // add header to body splitter
            requestContent.append(newLine);

            // add file content
            requestContent.append(fileInfo->localFileData);
            requestContent.append(newLine);
        }

        // add end of body
        requestContent.append(boundaryDelimiter);
        requestContent.append(boundary);
        requestContent.append(boundaryDelimiter);
    }
    else
    {
        QVariantMap vmap;
        QMapIterator<QString, QByteArray> it{input->vars};
        while (it.hasNext())
        {
            it.next();
            vmap[it.key()] = it.value();
        }
        requestContent.append(
            QJsonDocument::fromVariant(vmap).toJson(QJsonDocument::Compact));
    }

    // prepare connection
    QUrl url;
    url.setUrl(input->urlStr);

    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Frumkin K. E. REST Client");

    if (input->varLayout == URL_ENCODED)
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          "application/x-www-form-urlencoded");
    }
    else if (input->varLayout == JSON)
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }
    else if (input->varLayout == MULTIPART)
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          "multipart/form-data; boundary=" + boundary);
    }

    //! add custom headers
    foreach (const QByteArray& key, input->headers.keys())
    {
        request.setRawHeader(key, input->headers[key]);
    }
    QNetworkReply* reply;
    if (input->httpMethod == Literals::getMethod)
    {
        reply = manager->get(request);
    }
    else if (input->httpMethod == Literals::postMethod)
    {
        reply = manager->post(request, requestContent);
    }
    else if (input->httpMethod == Literals::putMethod)
    {
        reply = manager->put(request, requestContent);
    }
    else if (input->httpMethod == Literals::headMethod)
    {
        reply = manager->head(request);
    }
    else if (input->httpMethod == Literals::deleteMethod)
    {
        reply = manager->deleteResource(request);
    }
    else
    {
        QBuffer buff(&requestContent);
        reply = manager->sendCustomRequest(request, input->httpMethod.toLatin1(), &buff);
    }

    return reply->error();
}

void HttpRequestWorker::setReadResponseOnError(bool t_readResponseOnError)
{
    m_readResponseOnError = t_readResponseOnError;
}

void HttpRequestWorker::setReadErrorString(bool readErrorString)
{
    m_readErrorString = readErrorString;
}

void HttpRequestWorker::managerFinished(QNetworkReply* reply)
{
    statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    errorType = reply->error();
    if (errorType == QNetworkReply::NoError || m_readResponseOnError)
    {
        response = reply->readAll();
    }
    if (m_readErrorString)
    {
        errorStr = reply->errorString();
    }
    reply->deleteLater();
    emit executionFinished(this);
}
