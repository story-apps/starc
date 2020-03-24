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

#ifndef WEBREQUEST_H
#define WEBREQUEST_H

#include <QNetworkRequest>
#include <QString>
#include <QVariant>
#include <QVector>


/**
 * @brief Класс запроса
 *        Хранит в себе все параметры необходимые для запроса и умеет формировать из них подготовленный QNetworkRequest
 */
class WebRequest
{
public:
    WebRequest();
    ~WebRequest();

    /**
     * @brief Ссылка запроса
     */
    QUrl urlToLoad() const;

    /**
     * @brief Параметры запроса
     */
    QString urlQuery() const;

    /**
     * @brief Установка ссылки для запроса
     */
    void setUrlToLoad(const QUrl& _url);

    /**
     * @brief Ссылка referer
     */
    QUrl urlReferer() const;

    /**
     * @brief Установка ссылки referer'а
     */
    void setUrlReferer(const QUrl& _url);

    /**
     * @brief Очистить список атрибутов
     */
    void clearAttributes();

    /**
     * @brief Добавление текстового атрибута в запрос
     * @param name - название атрибута
     * @param value - значение атрибута
     */
    void addAttribute(const QString& _name, const QVariant& _value);

    /**
     * @brief Добавление атрибута-файла в запрос
     * @param name - название атрибута
     * @param filePath - путь к файлу
     */
    void addAttributeFile(const QString& _name, const QString& _filePath);

    /**
     * @brief Установить сырые данные в качестве данных запроса
     */
    /** @{ */
    void setRawData(const QByteArray& _data);
    void setRawData(const QByteArray& _data, const QString& _mime);
    /** @} */

    /**
     * @brief Сформировать объект класса QNetworkRequest
     */
    QNetworkRequest networkRequest(bool _addContentHeaders = false);

    /**
     * @breif Получить данные запроса
     */
    QByteArray  multiPartData();

private:
    /**
     * @brief Текстовые атрибуты запроса
     */
    QVector<QPair<QString, QVariant>> attributes() const;

    /**
     * @brief Добавление текстового атрибута в запрос
     * @param attribute - <имя, значения атрибута>
     */
    void addAttribute(const QPair<QString, QVariant>& _attribute);

    /**
     * @brief Атрибуты-файлы запроса
     */
    QVector<QPair<QString, QString>> attributeFiles() const;

    /**
     * @brief Добавить атрибут с файлом в запрос
     * @param attributeFile - <имя атрибута, путь к файлу>
     */
    void addAttributeFile(const QPair<QString, QString>& _attributeFile);

private:
    /**
     * @brief Ссылка для загрузки
     */
    QUrl m_urlToLoad;

    /**
     * @brief Ссылка referer
     */
    QUrl m_urlReferer;

    /**
     * @brief Список параметров-атрибутов запроса
     */
    QVector<QPair<QString, QVariant>> m_attributes;

    /**
     * @brief Список параметров-файлов запроса (тут хранятся пути к файлам)
     */
    QVector<QPair<QString, QString>> m_attributeFiles;

    /**
     * @brief Использовать ли сырые данные для запроса
     */
    bool m_useRawData = false;

    /**
     * @brief Сырые данные запроса
     */
    QByteArray m_rawData;

    /**
     * @brief Майм-тип сырых данных
     */
    QString m_mimeRawData;
};

/**
 * @brief Одинаковыми могут быть только два одинаковых куска памяти
 */
bool operator==(const WebRequest& _lhs, const WebRequest& _rhs);

#endif // WEBREQUEST_H
