#pragma once

#include <corelib_global.h>


/**
 * @brief Вспомогательные функции для работы с единицами измерениями
 */
class CORE_LIBRARY_EXPORT MeasurementHelper
{
public:
    /**
     * @brief Перевести миллиметры в пикселы и обратно
     * @param _x указывает направление (горизонтальное - true или вертикальное - false), в котором
     * необходимо произвести рассчёт
     */
    static qreal mmToPx(qreal _mm, bool _x = true);
    static qreal pxToMm(qreal _px, bool _x = true);

    /**
     * @brief Перевести пункты в пикселы и обратно
     * @param _x указывает направление (горизонтальное - true или вертикальное - false), в котором
     * необходимо произвести рассчёт
     */
    static qreal ptToPx(qreal _pt, bool _x = true);
    static int pxToPt(qreal _px, bool _x = true);

    /**
     * @brief Перевести миллиметры в дюймы и обратно
     */
    static qreal mmToInch(qreal _mm);
    static qreal inchToMm(qreal _inch);
};
