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

#include "WebRequestParameters.h"

WebRequestParameters::WebRequestParameters()
{
}

void WebRequestParameters::setCookieJar(QNetworkCookieJar* _cookieJar)
{
    if (m_cookieJar != _cookieJar) {
        m_cookieJar = _cookieJar;
    }
}

QNetworkCookieJar* WebRequestParameters::cookieJar() const
{
    return m_cookieJar;
}

void WebRequestParameters::setRequestMethod(NetworkRequestMethod _method)
{
    m_requestMethod = _method;
}

NetworkRequestMethod WebRequestParameters::requestMethod() const
{
    return m_requestMethod;
}

void WebRequestParameters::setLoadingTimeout(int _loadingTimeout)
{
    m_loadingTimeout = _loadingTimeout;
}

int WebRequestParameters::loadingTimeout() const
{
    return m_loadingTimeout;
}

bool operator==(const WebRequestParameters& _lhs, const WebRequestParameters& _rhs)
{
    return &_lhs == &_rhs;
}
