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

#include "NetworkQueue.h"
#include "WebLoader.h"


NetworkQueue* NetworkQueue::instance() {
    static NetworkQueue queue;
    return &queue;
}

WebRequest& NetworkQueue::registerRequest()
{
    m_requests.append(WebRequest{});
    return m_requests.last();
}

WebRequestParameters& NetworkQueue::registerRequestParameters()
{
    m_requestParameters.append(WebRequestParameters{});
    return m_requestParameters.last();
}

void NetworkQueue::enqueue(NetworkRequest* _request)
{
    Q_ASSERT_X(_request, Q_FUNC_INFO, "NetworkRequest shouldn't be a null pointer");

    //
    // Добавим запрос в очередь
    //
    NetworkQueueEntry queueEntry;
    queueEntry.request = _request;
    m_queue.enqueue(queueEntry);

    //
    // Подпишемся на удаление объекта, чтобы пометить его, как не нуждающийся в загрузке
    //
    connect(_request, &NetworkRequest::destroyed, [this, _request] { stop(_request); });

    //
    // Попробуем отправить запрос на загрузку прямо сейчас
    //
    processQueue();
}

void NetworkQueue::stop(NetworkRequest* _request)
{
    Q_ASSERT_X(_request, Q_FUNC_INFO, "NetworkRequest shouldn't be a null pointer");

    //
    // Просто помечаем запрос, как не нуждающийся в загрузке
    //
    for (NetworkQueueEntry& entry : m_queue) {
        if (entry.request == _request) {
            entry.isNeedToLoad = false;
            m_requests.removeAll(_request->m_request);
            m_requestParameters.removeAll(_request->m_requestParameters);
        }
    }
}

void NetworkQueue::stopAll()
{
    //
    // Очистим очередь ожидающих запросов
    //
    m_queue.clear();

    //
    // Остановим уже обрабатывающиеся запросы
    //
    for (WebLoader* loader : m_busyLoaders) {
        loader->stop();
    }
}

NetworkQueue::NetworkQueue()
{
    //
    // В нужном количестве создадим WebLoader'ы
    //
    for (int i = 0; i != std::max(QThread::idealThreadCount(), 4); ++i) {
        m_freeLoaders.append(new WebLoader(this));
    }
}

void NetworkQueue::processQueue()
{
    //
    // Если нет свободных загрузчиков или запросов на загрузку, ничего и не делаем
    //
    if (m_freeLoaders.isEmpty()
        || m_queue.isEmpty()) {
        return;
    }

    //
    // Извлечём запрос, который необходимо загрузить
    //
    NetworkQueueEntry requestEntry;
    while (!m_queue.isEmpty()) {
        requestEntry = m_queue.dequeue();
        if (requestEntry.isNeedToLoad) {
            break;
        }
    }

    //
    // Если не нашли ни одного запроса, который нужно загрузить, прерываемся
    //
    if (!requestEntry.isNeedToLoad) {
        return;
    }

    //
    // Перемещаем загрузчик в список занятых
    //
    WebLoader* loader = m_freeLoaders.takeLast();
    m_busyLoaders.append(loader);
    //
    // ... конфигурируем его
    //
    loader->setWebRequest(requestEntry.request->m_request);
    loader->setWebRequestParameters(requestEntry.request->m_requestParameters);
    //
    // ... соединяем с запросом
    //
    connect(loader, static_cast<void (WebLoader::*)(QByteArray, QUrl)>(&WebLoader::downloadComplete),
            requestEntry.request, &NetworkRequest::downloadComplete);
    connect(loader, static_cast<void (WebLoader::*)(int, QUrl)>(&WebLoader::uploadProgress),
            requestEntry.request, &NetworkRequest::uploadProgress);
    connect(loader, static_cast<void (WebLoader::*)(int, QUrl)>(&WebLoader::downloadProgress),
            requestEntry.request, &NetworkRequest::downloadProgress);
    connect(loader, &WebLoader::error, requestEntry.request, &NetworkRequest::error);
    connect(loader, &WebLoader::finished, requestEntry.request, &NetworkRequest::finished);
    connect(loader, &WebLoader::finished, this, &NetworkQueue::reinitFinishedLoader);
    //
    // ... и запускаем выполнение
    //
    loader->loadAsync();
}

void NetworkQueue::reinitFinishedLoader()
{
    WebLoader* loader = qobject_cast<WebLoader*>(sender());
    if (loader == nullptr) {
        return;
    }

    //
    // Отключаем загрузчик от всех соединений
    //
    loader->disconnect();

    //
    // Если загрузчик был в списке занятых, исключаем его оттуда и переводим в список свободных
    //
    const int loaderIndex = m_busyLoaders.indexOf(loader);
    const int invalidIndex = -1;
    if (loaderIndex != invalidIndex) {
        m_busyLoaders.takeAt(loaderIndex);
        m_freeLoaders.append(loader);
    }

    //
    // А потом пробуем что-нибудь загрузить
    //
    processQueue();
}
