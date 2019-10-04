#pragma once

#include <QProxyStyle>


class ApplicationStyle: public QProxyStyle
{
public:
    explicit ApplicationStyle(QStyle* _style = nullptr);

    /**
     * @brief Переопределяем, чтобы задать собственный размер вспомогательных элементов
     */
    QRect subControlRect(ComplexControl _complexControl, const QStyleOptionComplex* _option,
                         SubControl _subControl, const QWidget* _widget) const override;

    /**
     * @brief Переопределяем, чтобы при корректно отрабатывать клик на полосе прокрутки
     */
    QStyle::SubControl hitTestComplexControl(QStyle::ComplexControl _control,
        const QStyleOptionComplex* _option, const QPoint& _pos, const QWidget* _widget) const override;

    int pixelMetric(PixelMetric _metric, const QStyleOption* _option = nullptr,
        const QWidget* _widget = nullptr) const override;
};
