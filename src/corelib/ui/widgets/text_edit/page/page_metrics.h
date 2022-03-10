#pragma once

#include <QMarginsF>
#include <QPageSize>

#include <corelib_global.h>

class QPaintDevice;

/**
 * @brief Класс метрик страницы
 */
class CORE_LIBRARY_EXPORT PageMetrics
{
public:
    PageMetrics(QPageSize::PageSizeId _pageFormat = QPageSize::A4,
                const QMarginsF& _mmMargins = {});
    ~PageMetrics();

    /**
     * @brief Обновить метрики
     */
    void update(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins = {},
                const QMarginsF& _pxPageMargins = {});

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
