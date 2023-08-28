/*
* Copyright (C) 2015-2018 Dimka Novikov, to@dimkanovikov.pro
* Copyright (C) 2016 Alexey Polushkin, armijo38@yandex.ru
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* Full license: http://dimkanovikov.pro/license/LGPLv3
*/

#include "WebRequest.h"
#include "HttpMultiPart.h"

#include <QDebug>
#include <QFile>
#include <QMimeDatabase>
#include <QSslConfiguration>
#include <QStringList>

namespace {
/**
 * @brief Заголовки запроса
 */
/** @{ */
const QByteArray kUserAgentHeader = "User-Agent";
const QByteArray kRefererHeader = "Referer";
/** @} */

/**
 * @brief UserAgent запроса по-умолчанию
 */
const QByteArray kUserAgent = "Web Robot v.0.12.1";

/**
 * @brief Boundary для разделения атрибутов запроса, при использовании  multi-part form data
 */
const QByteArray kBoundary = "---------------------------7d935033608e2";

/**
 * @brief ContentType запроса по-умолчанию
 */
/** @{ */
const QString kContentTypeDefault = "application/x-www-form-urlencoded";
const QString kContentType = "multipart/form-data; boundary=" + kBoundary;
/** @} */
} // namespace


WebRequest::WebRequest()
{
}

WebRequest::~WebRequest()
{
}

QUrl WebRequest::urlToLoad() const
{
    return m_urlToLoad;
}

QString WebRequest::urlQuery() const
{
    QString query;
    for (auto attribute : m_attributes) {
        if (!query.isEmpty()) {
            query.append("&");
        }
        query.append(QString("%1=%2").arg(attribute.first, attribute.second.toString()));
    }
    return query;
}

void WebRequest::setUrlToLoad(const QUrl& _url)
{
    m_urlToLoad = _url;
}

QUrl WebRequest::urlReferer() const
{
    return m_urlReferer;
}

void WebRequest::setUrlReferer(const QUrl& _url)
{
    m_urlReferer = _url;
}

QByteArray WebRequest::authToken() const
{
    return m_authToken;
}

void WebRequest::setAuthToken(const QByteArray& _token)
{
    m_authToken = _token;
}

void WebRequest::clearAttributes()
{
    m_attributes.clear();
    m_attributeFiles.clear();
    m_rawData.clear();
}

void WebRequest::addAttribute(const QString& _name, const QVariant& _value)
{
    if (m_useRawData && !m_rawData.isEmpty()) {
        qWarning() << "You are trying to mix methods. Raw data will be cleaned";
        m_rawData.clear();
    }

    m_useRawData = false;
    addAttribute({ _name, _value });
}

void WebRequest::addAttributeFile(const QString& _name, const QString& _filePath)
{
    if (m_useRawData && !m_rawData.isEmpty()) {
        qWarning() << "You are trying to mix methods. Raw data will be cleaned";
        m_rawData.clear();
    }

    m_useRawData = false;
    addAttributeFile({ _name, _filePath });
}

void WebRequest::setRawData(const QByteArray& _data)
{
    const QString mimeType = QMimeDatabase().mimeTypeForData(_data).name();
    setRawData(_data, mimeType);
}

void WebRequest::setRawData(const QByteArray& _data, const QString& _mime)
{
    if (!m_useRawData && (!m_attributes.isEmpty() || !m_attributeFiles.isEmpty())) {
        qWarning() << "You are trying to mix methods. Attributes will be cleaned";
        m_attributes.clear();
        m_attributeFiles.clear();
    }

    m_useRawData = true;
    m_rawData = _data;
    m_mimeRawData = _mime;
}

QNetworkRequest WebRequest::networkRequest(bool _addContentHeaders)
{
    QNetworkRequest request(urlToLoad());

    //
    // Установка заголовков запроса
    //
    request.setRawHeader(kUserAgentHeader, kUserAgent);
    if (!urlReferer().isEmpty()) {
        request.setRawHeader(kRefererHeader, urlReferer().toString().toUtf8().data());
    }
    //
    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeDefault);
    if (_addContentHeaders) {
        if (m_useRawData) {
            request.setHeader(QNetworkRequest::ContentTypeHeader, m_mimeRawData);
        } else {
            request.setHeader(QNetworkRequest::ContentTypeHeader, kContentType);
        }
        request.setHeader(QNetworkRequest::ContentLengthHeader, multiPartData().size());
    }
    //
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", m_authToken);
    }

    return request;
}

QByteArray WebRequest::multiPartData()
{
    if (m_useRawData) {
        return m_rawData;
    }

    HttpMultiPart multiPart;
    multiPart.setBoundary(kBoundary);

    //
    // Добавление текстовых атрибутов
    //
    QPair<QString, QVariant> attribute;
    foreach (attribute, attributes()) {
        HttpPart textPart(HttpPart::Text);
        QString attributeName = attribute.first;
        QString attributeValue = attribute.second.toString();
        textPart.setText(attributeName, attributeValue);
        multiPart.addPart(textPart);
    }

    //
    // Добавление атрибутов-файлов
    //
    QPair<QString, QString> attributeFile;
    foreach (attributeFile, attributeFiles()) {
        HttpPart filePart(HttpPart::File);
        QString attributeName = attributeFile.first;
        QString attributeFilePath = attributeFile.second;
        filePart.setFile(attributeName, attributeFilePath);
        multiPart.addPart(filePart);
    }

    return multiPart.data();
}

QVector<QPair<QString, QVariant>> WebRequest::attributes() const
{
    return m_attributes;
}

void WebRequest::addAttribute(const QPair<QString, QVariant>& _attribute)
{
    if (!attributes().contains(_attribute)) {
        m_attributes.append(_attribute);
    }
}

QVector<QPair<QString, QString>> WebRequest::attributeFiles() const
{
    return m_attributeFiles;
}

void WebRequest::addAttributeFile(const QPair<QString, QString>& _attributeFile)
{
    if (!attributeFiles().contains(_attributeFile)) {
        m_attributeFiles.append(_attributeFile);
    }
}

bool operator==(const WebRequest& _lhs, const WebRequest& _rhs)
{
    return &_lhs == &_rhs;
}
