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

#ifndef NETWORKQUEUE_H
#define NETWORKQUEUE_H

#include "WebRequest.h"
#include "WebRequestParameters.h"

#include <QObject>
#include <QQueue>

class NetworkRequest;
class WebLoader;


/**
 * @brief Класс, реализующий очередь запросов
 * Реализован как паттерн Singleton
 */
class NetworkQueue : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Метод, возвращающий указатель на инстанс класса
     */
    static NetworkQueue* instance();

public:
    /**
     * @brief Зарегистрировать запрос
     */
    WebRequest& registerRequest();

    /**
     * @brief Зарегистрировать параметры запроса
     */
    WebRequestParameters& registerRequestParameters();

    /**
     * @brief Добавить запрос в очередь
     */
    void enqueue(NetworkRequest* _request);

    /**
     * @brief Запросить остановку запроса
     * @note Метод не гарантирует остановку запроса, если он уже выполняется, для этого стоит использовать stopAll
     */
    void stop(NetworkRequest* _request);

    /**
     * @brief Метод, останавливающий все текущие запросы и очищающий очередь
     */
    void stopAll();

private:
    /**
     * @brief Приватные конструкторы и оператор присваивания
     * Для реализации паттерна Singleton
     */
    NetworkQueue();
    NetworkQueue(const NetworkQueue&);
    NetworkQueue& operator=(const NetworkQueue&);

    /**
     * @brief Выполнить шаг обработки очереди
     */
    void processQueue();

    /**
     * @brief Перенастроить загрузчик завершивший свою работу
     */
    void reinitFinishedLoader();

private:
    /**
     * @brief Список зарегистрированных (потенциальных) запросов
     */
    QList<WebRequest> m_requests;

    /**
     * @brief Список зарегистрированных параметров запросов
     */
    QList<WebRequestParameters> m_requestParameters;

    /**
     * @brief Свободные загрузчики
     */
    QVector<WebLoader*> m_freeLoaders;

    /**
     * @brief Занятые загрузчики
     */
    QVector<WebLoader*> m_busyLoaders;

    /**
     * @brief Объект очереди на загрузку
     */
    struct NetworkQueueEntry {
        /**
         * @brief Необходимо ли загрузить
         */
        bool isNeedToLoad = true;

        /**
         * @brief Объект запроса
         */
        NetworkRequest* request = nullptr;
    };

    /**
     * @brief Собственно очередь запросов
     */
    QQueue<NetworkQueueEntry> m_queue;
};

#endif // NETWORKQUEUE_H
