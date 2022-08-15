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
     * @brief Перевести дюймы в пиксели и обратно
     * @param _x указывает направление (горизонтальное - true или вертикальное - false), в котором
     * необходимо произвести рассчёт
     */
    static qreal inchToPx(qreal _inch, bool _x = true);
    static qreal pxToInch(qreal _px, bool _x = true);

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

    /**
     * @brief Перевести миллиметры/пиксели в твипсы (мера длины в формате RTF)
     */
    static int mmToTwips(qreal _mm);
    static int pxToTwips(qreal _px, bool _x = true);
};
