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

#ifndef NETWORKREQUEST_H
#define NETWORKREQUEST_H

#include "NetworkTypes.h"

#include <QObject>
#include <QUrl>

class NetworkRequestPrivate;
class QNetworkCookieJar;
class WebRequest;
class WebRequestParameters;

/**
 * @brief Пользовательский класс для создания GET и POST запросов
 */

class NetworkRequest : public QObject
{
    Q_OBJECT
    friend class NetworkQueue;

public:
    /**
     * @brief Остановить все текущие соединения
     */
    static void stopAllConnections();

public:
    explicit NetworkRequest(QObject* _parent = nullptr);

    /**
     * @brief Установка cookie для загрузчика
     */
    void setCookieJar(QNetworkCookieJar* _cookieJar);

    /**
     * @brief Получение cookie загрузчика
     */
    QNetworkCookieJar* cookieJar() const;

    /**
     * @brief Установка метода запроса
     */
    void setRequestMethod(NetworkRequestMethod _method);

    /**
     * @brief Получение метода запроса
     */
    NetworkRequestMethod requestMethod() const;

    /**
     * @brief Установка таймаута загрузки
     */
    void setLoadingTimeout(int _loadingTimeout);

    /**
     * @brief Получение таймаута загрузки
     */
    int loadingTimeout() const;

    /**
     * @brief Очистить все старые атрибуты запроса
     */
    void clearRequestAttributes();

    /**
     * @brief Добавление атрибута в запрос
     */
    void addRequestAttribute(const QString& _name, const QVariant& _value);

    /**
     * @brief Добавление файла в запрос
     */
    void addRequestAttributeFile(const QString& _name, const QString& _filePath);

    /**
     * @brief Установить сырые данные для запроса
     */
    /** @{ */
    void setRawRequestData(const QByteArray& _data);
    void setRawRequestData(const QByteArray& _data, const QString& _mime);
    /** @} */

    /**
     * @brief Асинхронная загрузка запроса
     */
    /** @{ */
    void loadAsync(const QString& _urlToLoad, const QString& _referer = QString());
    void loadAsync(const QUrl& _urlToLoad, const QUrl& _referer = QUrl());
    /** @} */

    /**
     * @brief Синхронная загрузка запроса
     */
    /** @{ */
    QByteArray loadSync(const QString& _urlToLoad, const QString& _referer = QString());
    QByteArray loadSync(const QUrl& _urlToLoad, const QUrl& _referer = QUrl());
    /** @} */

    /**
     * @brief Остановка выполнения запроса, связанного с текущим объектом
     */
    void stop();

    /**
     * @brief Уведомить клиентов о том, что запрос завершился
     */
    void done();

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
     * @brief Выполнение запроса завершено
     */
    void finished();

    /**
     * @brief Сообщение об ошибке при загрузке
     */
    /** @{ */
    void error(QString, QUrl);
    void errorDetails(QString, QUrl);
    /** @} */

private:
    /**
     * @brief Ссылка на запроса
     */
    WebRequest& m_request;

    /**
     * @brief Ссылка на параметры запроса
     */
    WebRequestParameters& m_requestParameters;

    /**
     * @brief Загруженные данные в случае, если используется синхронная загрузка
     */
    QByteArray m_downloadedData;
};

#endif // NETWORKREQUEST_H
