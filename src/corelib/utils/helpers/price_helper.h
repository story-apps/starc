#pragma once

#include <QtGlobal>

#include <corelib_global.h>


/**
 * @brief Вспомогательные функции для работы с изображениями
 */
class CORE_LIBRARY_EXPORT PriceHelper
{
public:
    /**
     * @brief Адоптировать цену в зависимости от локали
     */
    static qreal adoptPrice(qreal _priceInUsd);

    /**
     * @brief Адоптировать текстовую метку с ценой
     */
    static QString adoptPriceLabel(const QString& _text);
};
