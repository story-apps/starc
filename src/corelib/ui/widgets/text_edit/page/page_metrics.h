#pragma once

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
    ~PageMetrics();

    /**
     * @brief Обновить метрики
     */
    void update(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins = {});

    /**
     * @brief Методы доступа к параметрам страницы
     */
    QPageSize::PageSizeId pageFormat() const;
    const QSizeF& mmPageSize() const;
    const QMarginsF& mmPageMargins() const;
    const QSizeF& pxPageSize() const;
    const QMarginsF& pxPageMargins() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
