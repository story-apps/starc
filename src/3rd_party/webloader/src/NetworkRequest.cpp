/*
* Copyright (C) 2016 Alexey Polushkin, armijo38@yandex.ru
* Copyright (C) 2018 Dimka Novikov, to@dimkanovikov.pro
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

#include "NetworkRequest.h"
#include "NetworkQueue.h"
#include "WebLoader.h"
#include "WebRequest.h"

#include <QEventLoop>
#include <QTimer>


void NetworkRequest::stopAllConnections()
{
    NetworkQueue::instance()->stopAll();
}

NetworkRequest::NetworkRequest(QObject* _parent)
    : QObject(_parent)
    , m_request(NetworkQueue::instance()->registerRequest())
    , m_requestParameters(NetworkQueue::instance()->registerRequestParameters())
{
    connect(this, &NetworkRequest::downloadComplete,
            [this](const QByteArray& _downloadedData) { m_downloadedData = _downloadedData; });
}

void NetworkRequest::setCookieJar(QNetworkCookieJar* _cookieJar)
{
    stop();
    m_requestParameters.setCookieJar(_cookieJar);
}

QNetworkCookieJar* NetworkRequest::cookieJar() const
{
    return m_requestParameters.cookieJar();
}

void NetworkRequest::setProxy(const QNetworkProxy& _proxy)
{
    stop();
    m_requestParameters.setProxy(_proxy);
}

QNetworkProxy NetworkRequest::proxy() const
{
    return m_requestParameters.proxy();
}

void NetworkRequest::setRequestMethod(NetworkRequestMethod _method)
{
    stop();
    m_requestParameters.setRequestMethod(_method);
}

NetworkRequestMethod NetworkRequest::requestMethod() const
{
    return m_requestParameters.requestMethod();
}

void NetworkRequest::setLoadingTimeout(int _loadingTimeout)
{
    stop();
    m_requestParameters.setLoadingTimeout(_loadingTimeout);
}

int NetworkRequest::loadingTimeout() const
{
    return m_requestParameters.loadingTimeout();
}

QByteArray NetworkRequest::authToken() const
{
    return m_request.authToken();
}

void NetworkRequest::setAuthToken(const QByteArray& _token)
{
    stop();
    m_request.setAuthToken(_token);
}

void NetworkRequest::clearRequestAttributes()
{
    stop();
    m_request.clearAttributes();
}

void NetworkRequest::addRequestAttribute(const QString& _name, const QVariant& _value)
{
    stop();
    m_request.addAttribute(_name, _value);
}

void NetworkRequest::addRequestAttributeFile(const QString& _name, const QString& _filePath)
{
    stop();
    m_request.addAttributeFile(_name, _filePath);
}

void NetworkRequest::setRawRequestData(const QByteArray& _data)
{
    stop();
    m_request.setRawData(_data);
}

void NetworkRequest::setRawRequestData(const QByteArray& _data, const QString& _mime)
{
    stop();
    m_request.setRawData(_data, _mime);
}

void NetworkRequest::loadAsync(const QString& _urlToLoad, const QString& _referer)
{
    loadAsync(QUrl(_urlToLoad), QUrl(_referer));
}

void NetworkRequest::loadAsync(const QUrl& _urlToLoad, const QUrl& _referer)
{
    //
    // Останавливаем в очереди запрос, связанный с данным запросом (если имеется)
    //
    NetworkQueue::instance()->stop(this);

    //
    // Настраиваем параметры и кладем в очередь
    //
    m_request.setUrlToLoad(_urlToLoad);
    m_request.setUrlReferer(_referer);
    NetworkQueue::instance()->enqueue(this);
}

QByteArray NetworkRequest::loadSync(const QString& _urlToLoad, const QString& _referer)
{
    return loadSync(QUrl(_urlToLoad), QUrl(_referer));
}

QByteArray NetworkRequest::loadSync(const QUrl& _urlToLoad, const QUrl& _referer)
{
    //
    // Для синхронного запроса используем QEventLoop и асинхронный запрос
    //
    QEventLoop loop;
    connect(this, &NetworkRequest::finished, &loop, &QEventLoop::quit);
    loadAsync(_urlToLoad, _referer);
    loop.exec();

    return m_downloadedData;
}

void NetworkRequest::stop()
{
    NetworkQueue::instance()->stop(this);
}

void NetworkRequest::done()
{
    emit finished();
}
