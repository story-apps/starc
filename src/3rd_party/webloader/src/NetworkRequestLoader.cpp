/*
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

#include "NetworkRequestLoader.h"


QByteArray NetworkRequestLoader::loadSync(const QUrl& _urlToLoad)
{
	NetworkRequest request;
	return request.loadSync(_urlToLoad);
}

QByteArray NetworkRequestLoader::loadSync(const QString& _urlToLoad)
{
	return loadSync(QUrl(_urlToLoad));
}
