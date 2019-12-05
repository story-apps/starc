#pragma once

#include <QProxyStyle>


class ApplicationStyle: public QProxyStyle
{
public:
    explicit ApplicationStyle(QStyle* _style = nullptr);

    QRect subControlRect(ComplexControl _complexControl, const QStyleOptionComplex* _option,
                         SubControl _subControl, const QWidget* _widget) const override;

    QStyle::SubControl hitTestComplexControl(QStyle::ComplexControl _control,
        const QStyleOptionComplex* _option, const QPoint& _pos, const QWidget* _widget) const override;

    int pixelMetric(PixelMetric _metric, const QStyleOption* _option = nullptr,
        const QWidget* _widget = nullptr) const override;

    void drawPrimitive(PrimitiveElement _element, const QStyleOption* _option, QPainter* _painter,
        const QWidget* _widget = nullptr) const override;

    int styleHint(StyleHint _hint, const QStyleOption* _option, const QWidget* _widget,
        QStyleHintReturn* _returnData) const override;
};
