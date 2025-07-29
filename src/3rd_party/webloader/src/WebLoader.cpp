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

#include "WebLoader.h"

#include <QEventLoop>
#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>

namespace {
    /**
     * @brief Не все сайты передают суммарный размер загружаемой страницы,
     *		  поэтому для отображения прогресса загрузки используется
     *		  заранее заданное число (средний размер веб-страницы)
     */
    const int kPossibleRecievedMaxFileSize = 120000;

    /**
     * @brief Преобразовать ошибку в читаемый вид
     */
    static QString networkErrorToString(QNetworkReply::NetworkError networkError) {
        QString result;
        switch (networkError) {
            case QNetworkReply::ConnectionRefusedError: result = "the remote server refused the connection (the server is not accepting requests)"; break;
            case QNetworkReply::RemoteHostClosedError: result = "the remote server closed the connection prematurely, before the entire reply was received and processed"; break;
            case QNetworkReply::HostNotFoundError: result = "the remote host name was not found (invalid hostname)"; break;
            case QNetworkReply::TimeoutError: result = "the connection to the remote server timed out"; break;
            case QNetworkReply::OperationCanceledError: result = "the operation was canceled via calls to abort() or close() before it was finished."; break;
            case QNetworkReply::SslHandshakeFailedError: result = "the SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() signal should have been emitted."; break;
            case QNetworkReply::TemporaryNetworkFailureError: result = "the connection was broken due to disconnection from the network, however the system has initiated roaming to another access point. The request should be resubmitted and will be processed as soon as the connection is re-established."; break;
            case QNetworkReply::NetworkSessionFailedError: result = "the connection was broken due to disconnection from the network or failure to start the network."; break;
            case QNetworkReply::BackgroundRequestNotAllowedError: result = "the background request is not currently allowed due to platform policy."; break;
            case QNetworkReply::ProxyConnectionRefusedError: result = "the connection to the proxy server was refused (the proxy server is not accepting requests)"; break;
            case QNetworkReply::ProxyConnectionClosedError: result = "the proxy server closed the connection prematurely, before the entire reply was received and processed"; break;
            case QNetworkReply::ProxyNotFoundError: result = "the proxy host name was not found (invalid proxy hostname)"; break;
            case QNetworkReply::ProxyTimeoutError: result = "the connection to the proxy timed out or the proxy did not reply in time to the request sent"; break;
            case QNetworkReply::ProxyAuthenticationRequiredError: result = "the proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)"; break;
            case QNetworkReply::ContentAccessDenied: result = "the access to the remote content was denied (similar to HTTP error 401)"; break;
            case QNetworkReply::ContentOperationNotPermittedError: result = "the operation requested on the remote content is not permitted"; break;
            case QNetworkReply::ContentNotFoundError: result = "the remote content was not found at the server (similar to HTTP error 404)"; break;
            case QNetworkReply::AuthenticationRequiredError: result = "the remote server requires authentication to serve the content but the credentials provided were not accepted (if any)"; break;
            case QNetworkReply::ContentReSendError: result = "the request needed to be sent again, but this failed for example because the upload data could not be read a second time."; break;
            case QNetworkReply::ContentConflictError: result = "the request could not be completed due to a conflict with the current state of the resource."; break;
            case QNetworkReply::ContentGoneError: result = "the requested resource is no longer available at the server."; break;
            case QNetworkReply::InternalServerError: result = "the server encountered an unexpected condition which prevented it from fulfilling the request."; break;
            case QNetworkReply::OperationNotImplementedError: result = "the server does not support the functionality required to fulfill the request."; break;
            case QNetworkReply::ServiceUnavailableError: result = "the server is unable to handle the request at this time."; break;
            case QNetworkReply::ProtocolUnknownError: result = "the Network Access API cannot honor the request because the protocol is not known"; break;
            case QNetworkReply::ProtocolInvalidOperationError: result = "the requested operation is invalid for this protocol"; break;
            case QNetworkReply::UnknownNetworkError: result = "an unknown network-related error was detected"; break;
            case QNetworkReply::UnknownProxyError: result = "an unknown proxy-related error was detected"; break;
            case QNetworkReply::UnknownContentError: result = "an unknown error related to the remote content was detected"; break;
            case QNetworkReply::ProtocolFailure: result = "a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)"; break;
            case QNetworkReply::UnknownServerError: result = "an unknown error related to the server response was detected"; break;
#if QT_VERSION >= 0x050600
            case QNetworkReply::TooManyRedirectsError: result = "while following redirects, the maximum limit was reached. The limit is by default set to 50 or as set by QNetworkRequest::setMaxRedirectsAllowed()."; break;
            case QNetworkReply::InsecureRedirectError: result = "while following redirects, the network access API detected a redirect from a encrypted protocol (https) to an unencrypted one (http)."; break;
#endif
            case QNetworkReply::NoError: result = "No error"; break;
        }

        return result;
    }
}


WebLoader::WebLoader(QObject* _parent) :
    QThread(_parent)
{
}

WebLoader::~WebLoader()
{
    stop();
    if (m_networkManager != nullptr)
        m_networkManager->deleteLater();
}

void WebLoader::setWebRequest(const WebRequest& _request)
{
    m_request = _request;
}

void WebLoader::setWebRequestParameters(const WebRequestParameters& _parameters)
{
    m_parameters = _parameters;
}

void WebLoader::loadAsync()
{
    loadAsync(m_request.urlToLoad(), m_request.urlReferer());
}

void WebLoader::loadAsync(const QUrl& _urlToLoad, const QUrl& _referer)
{
    //
    // Останавливаем, если выполняется в данный момент
    //
    stop();

    //
    // Сбрасываем переменные времени выполненеия
    //
    m_isNeedStop = false;
    m_downloadedData.clear();

    //
    // Настраиваем запрос
    //
    m_request.setUrlToLoad(_urlToLoad);
    m_request.setUrlReferer(_referer);

    //
    // Запускаем загрузку
    //
    start();
}

void WebLoader::run()
{
    initNetworkManager();

    m_requestSourceUrl = m_request.urlToLoad();

    do
    {
        if (m_isNeedStop) {
            return;
        }

        //! Начало загрузки страницы m_request.url()
        emit uploadProgress(0, m_requestSourceUrl);
        emit downloadProgress(0, m_requestSourceUrl);

        QPointer<QNetworkReply> reply = nullptr;

        switch (m_parameters.requestMethod()) {

            default:
            case NetworkRequestMethod::Get: {
                const QNetworkRequest request = this->m_request.networkRequest();
                reply = m_networkManager->get(request);
                break;
            }

            case NetworkRequestMethod::Post: {
                const QNetworkRequest networkRequest = m_request.networkRequest(true);
                const QByteArray data = m_request.multiPartData();
                reply = m_networkManager->post(networkRequest, data);
                break;
            }

        } // switch

        connect(reply.data(), &QNetworkReply::uploadProgress,
                this, static_cast<void (WebLoader::*)(qint64, qint64)>(&WebLoader::uploadProgress));
        connect(reply.data(), &QNetworkReply::downloadProgress,
                this, static_cast<void (WebLoader::*)(qint64, qint64)>(&WebLoader::downloadProgress));
        connect(reply.data(), &QNetworkReply::errorOccurred, this, &WebLoader::downloadError);
#if QT_CONFIG(ssl)
        connect(reply.data(), &QNetworkReply::sslErrors, this, &WebLoader::downloadSslErrors);
        connect(reply.data(), &QNetworkReply::sslErrors,
                reply.data(), static_cast<void (QNetworkReply::*)()>(&QNetworkReply::ignoreSslErrors));
#endif

        //
        // Таймер для прерывания работы
        //
        QTimer timeoutTimer;
        connect(reply.data(), &QNetworkReply::uploadProgress, &timeoutTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
        connect(reply.data(), &QNetworkReply::downloadProgress, &timeoutTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
        connect(&timeoutTimer, &QTimer::timeout, reply.data(), [this, &reply]() {
            m_isTimeout = true;
            m_isNeedRedirect = false;
            reply->abort();
            quit();
        });
        m_isTimeout = false;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(m_parameters.loadingTimeout());

        //
        // Входим в поток обработки событий, ожидая завершения отработки networkManager'а
        //
        exec();

        if (m_isNeedStop) {
            return;
        }

        //
        // Если ответ получен, останавливаем таймер
        //
        if (!reply.isNull() && reply->isFinished()) {
            timeoutTimer.stop();
        }

    } while (m_isNeedRedirect);

    emit downloadComplete(m_downloadedData, m_requestSourceUrl);
}

void WebLoader::stop()
{
    m_isNeedStop = true;

    //
    // Сперва пробуем остановить аккуратно
    //
    if (isRunning()) {
        quit();
        wait(1000);
    }

    //
    // Если не удалось, прерываем жёстко
    //
    if (isRunning()) {
        terminate();
        wait();
    }
}

void WebLoader::uploadProgress(qint64 _uploadedBytes, qint64 _totalBytes)
{
    //! отправлено [uploaded] байт из [total]
    if (_totalBytes > 0) {
        emit uploadProgress(((float)_uploadedBytes / _totalBytes) * 100, m_requestSourceUrl);
    }
}

void WebLoader::downloadProgress(qint64 _recievedBytes, qint64 _totalBytes)
{
    //! загружено [recieved] байт из [total]
    // не все сайты передают суммарный размер загружаемой страницы,
    // поэтому для отображения прогресса загрузки используется
    // заранее заданное число (средний размер веб-страницы)
    if (_totalBytes < 0) {
        _totalBytes = kPossibleRecievedMaxFileSize;
    }
    emit downloadProgress(((float)_recievedBytes / _totalBytes) * 100, m_requestSourceUrl);
}

void WebLoader::downloadComplete(QNetworkReply* _reply)
{
    //! Завершена загрузка страницы [m_request.url()]

    // требуется ли редирект?
    if (!_reply->header(QNetworkRequest::LocationHeader).isNull()) {
        //! Осуществляется редирект по ссылке [redirectUrl]
        // Referer'ом становится ссылка по хоторой был осуществлен запрос
        QUrl refererUrl = m_request.urlToLoad();
        m_request.setUrlReferer(refererUrl);
        // Получаем ссылку для загрузки из заголовка ответа [Loacation]
        QUrl redirectUrl = _reply->header(QNetworkRequest::LocationHeader).toUrl();
        m_request.setUrlToLoad(redirectUrl);
        m_isNeedRedirect = true;
    } else {
        //! Загружены данные [reply->bytesAvailable()]
        if (_reply->isOpen()) {
            qint64 downloadedDataSize = _reply->bytesAvailable();
            QByteArray downloadedData = _reply->read(downloadedDataSize);
            m_downloadedData = downloadedData;
        }
        _reply->deleteLater();
        m_isNeedRedirect = false;
    }

    if (!isRunning()) {
        wait(300);
    }

    quit(); // прерываем цикл обработки событий потока (возвращаемся в run())
}

void WebLoader::downloadError(QNetworkReply::NetworkError _networkError)
{
    switch (_networkError) {

        case QNetworkReply::NoError: {
            break;
        }

        default: {
            if (m_isTimeout) {
                _networkError = QNetworkReply::TimeoutError;
            }
            const QString lastError =
                    tr("Sorry, we have some error while loading. Error is: %1")
                    .arg(networkErrorToString(_networkError));
            emit error(lastError, m_requestSourceUrl);
            break;
        }
    }
}

#if QT_CONFIG(ssl)
void WebLoader::downloadSslErrors(const QList<QSslError>& _errors)
{
    QString lastErrorDetails;
    foreach (const QSslError& error, _errors) {
        if (!lastErrorDetails.isEmpty()) {
            lastErrorDetails.append("\n");
        }
        lastErrorDetails.append(error.errorString());
    }

    emit error(lastErrorDetails, m_requestSourceUrl);
}
#endif

void WebLoader::initNetworkManager()
{
    //
    // Создаём загрузчика, если нужно
    //
    if (m_networkManager == nullptr) {
        m_networkManager = new QNetworkAccessManager;
    }

    //
    // Настраиваем куки
    //
    QNetworkCookieJar* cookie = m_parameters.cookieJar();
    if (cookie != nullptr) {
        m_networkManager->setCookieJar(cookie);
        cookie->setParent(nullptr); // Убираем родителя, т.к. удалением управляет клиент
    }

    //
    // Настраиваем прокси
    //
    m_networkManager->setProxy(m_parameters.proxy());

    //
    // Оключаем от предыдущих соединений и настраиваем новое
    //
    m_networkManager->disconnect();
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, static_cast<void (WebLoader::*)(QNetworkReply*)>(&WebLoader::downloadComplete));
}
