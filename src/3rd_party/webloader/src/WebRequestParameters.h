/*
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

#ifndef WEBREQUESTPARAMETERS_H
#define WEBREQUESTPARAMETERS_H

#include "NetworkTypes.h"

#include <QNetworkProxy>

class QNetworkCookieJar;


/**
 * @brief Параметры запроса для загрузчика
 */
class WebRequestParameters
{
public:
    WebRequestParameters();

    /**
     * @brief Сookie для загрузчика
     */
    void setCookieJar(QNetworkCookieJar* _cookieJar);
    QNetworkCookieJar* cookieJar() const;

    /**
     * @brief Прокси запроса
     */
    void setProxy(const QNetworkProxy& _proxy);
    QNetworkProxy proxy() const;

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

private:
    /**
     * @brief Куки процесса
     */
    QNetworkCookieJar* m_cookieJar = nullptr;

    /**
     * @brief Прокси запроса
     */
    QNetworkProxy m_proxy;

    /**
     * @brief Метод запроса
     */
    NetworkRequestMethod m_requestMethod = NetworkRequestMethod::Undefined;

    /**
     * @brief Таймаут загрузки ссылки, милисекунд
     */
    int m_loadingTimeout = 20000;
};

/**
 * @brief Одинаковыми могут быть только два одинаковых куска памяти
 */
bool operator==(const WebRequestParameters& _lhs, const WebRequestParameters& _rhs);

#endif // WEBREQUESTPARAMETERS_H
