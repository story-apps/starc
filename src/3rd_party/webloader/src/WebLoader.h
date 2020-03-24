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

#ifndef WEBLOADER_H
#define WEBLOADER_H

#include "NetworkRequest.h"
#include "WebRequest.h"
#include "WebRequestParameters.h"

#include <QNetworkReply>
#include <QThread>

class QNetworkAccessManager;
class QNetworkCookieJar;


/**
 * @brief Класс для осуществления запросов по http(s)-протоколу
 */
class WebLoader : public QThread
{
	Q_OBJECT

public:
    explicit WebLoader(QObject* _parent = nullptr);
    virtual ~WebLoader();

    /**
     ** @brief Установить запрос
     */
    void setWebRequest(const WebRequest& _request);

    /**
     * @brief Установить паараметры запросы
     */
    void setWebRequestParameters(const WebRequestParameters& _parameters);

    /**
     * @brief Отправка запроса (асинхронное выполнение)
      */
    /** @{ */
    void loadAsync();
    void loadAsync(const QUrl& _urlToLoad, const QUrl& _referer = QUrl());
    /** @} */

    /**
     * @brief Остановить выполнение
     */
    void stop();

signals:
    /**
     * @brief Прогресс отправки запроса на сервер
     */
    void uploadProgress(int, QUrl);

    /**
     * @brief Прогресс загрузки данных с сервера
     */
    void downloadProgress(int, QUrl);

    /**
     * @brief Данные загружены
      */
    void downloadComplete(QByteArray, QUrl);

    /**
     * @brief Сообщение об ошибке при загрузке
     */
    /** @{ */
    void error(QString, QUrl);
    void errorDetails(QString, QUrl);
    /** @} */

protected:
    /**
     * @brief Работа потока
     */
    void run() override;

private:
    /**
     * @brief Прогресс отправки запроса на сервер
     * @param uploadedBytes - отправлено байт
     * @param totalBytes - байт к отправке
     */
    void uploadProgress(qint64 _uploadedBytes, qint64 _totalBytes);

    /**
     * @brief Прогресс загрузки данных с сервера
     * @param recievedBytes - загружено байт
     * @param totalBytes - байт к отправке
     */
    void downloadProgress(qint64 _recievedBytes, qint64 _totalBytes);

    /**
     * @brief Окончание загрузки страницы
     */
    void downloadComplete(QNetworkReply* _reply);

    /**
     * @brief Ошибка при загрузки страницы
     */
    void downloadError(QNetworkReply::NetworkError _networkError);

    /**
     ** @brief Ошибки при защищённом подключении
	 */
	void downloadSslErrors(const QList<QSslError>& _errors);

    /**
     * @brief Подготовить менеджер загрузок для того, чтобы он работал в текущем потоке
     */
	void initNetworkManager();

private:
    /**
     * @brief Необходимо ли остановить выполненеие процесса
     */
    bool m_isNeedStop = false;

    /**
     * @brief Собственно загрузчик, который делает всю черновую работу
     */
    QNetworkAccessManager* m_networkManager = nullptr;

    /**
     * @brief Объекст запроса
     */
    WebRequest m_request;

    /**
     * @brief Параметры запроса
     */
    WebRequestParameters m_parameters;

    /**
     * @brief Нужен ли редирект
     */
    bool m_isNeedRedirect = false;

    /**
     * @brief Исходная ссылка для загрузки
     * @note Во время редиректов ссылка в WebRequest'е может указывать не на исходно загружаемую страницу
     */
    QUrl m_requestSourceUrl;

    /**
     * @brief Загруженные данные
     */
    QByteArray m_downloadedData;
};

#endif // WEBLOADER_H
