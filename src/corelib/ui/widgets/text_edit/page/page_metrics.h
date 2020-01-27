#pragma once

#include <QString>
#include <QSizeF>
#include <QMarginsF>
#include <QPageSize>

class QPaintDevice;


/**
 * @brief Класс метрик страницы
 */
class PageMetrics
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
    static qreal pxToPt(qreal _px, bool _x = true);

    /**
     * @brief Получить размер страницы из строки
     */
    static QPageSize::PageSizeId pageSizeIdFromString(const QString& _from);

    /**
     * @brief Получить строку из размера страницы
     */
    static QString stringFromPageSizeId(QPageSize::PageSizeId _pageSize);

public:
    PageMetrics(QPageSize::PageSizeId _pageFormat = QPageSize::A4, const QMarginsF& _mmMargins = {});

    /**
     * @brief Обновить метрики
     */
    void update(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins = {});

    /**
     * @brief Методы доступа к параметрам страницы
     */
    QPageSize::PageSizeId pageFormat() const;
    QSizeF mmPageSize() const;
    QMarginsF mmPageMargins() const;
    QSizeF pxPageSize() const;
    QMarginsF pxPageMargins() const;

private:
    /**
     * @brief Формат страницы
     */
    QPageSize::PageSizeId m_pageFormat;

    /**
     * @brief Размеры в миллиметрах
     */
    QSizeF m_mmPageSize;
    QMarginsF m_mmPageMargins;

    /**
     * @brief Размеры в пикселах
     */
    QSizeF m_pxPageSize;
    QMarginsF m_pxPageMargins;
};
