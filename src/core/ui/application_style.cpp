#include "application_style.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QStyleOption>

namespace {
    qreal scaledValue(qreal _value) {
        return _value * Ui::DesignSystem::scaleFactor();
    }
}


ApplicationStyle::ApplicationStyle(QStyle* _style)
    : QProxyStyle(_style)
{
}

QRect ApplicationStyle::subControlRect(QStyle::ComplexControl _complexControl,
    const QStyleOptionComplex* _option, QStyle::SubControl _subControl, const QWidget* _widget) const
{
    if (_complexControl == CC_ScrollBar
        && (_subControl == SC_ScrollBarAddLine || _subControl == SC_ScrollBarSubLine)) {
        return {};
    }

    return QProxyStyle::subControlRect(_complexControl, _option, _subControl, _widget);
}

QStyle::SubControl ApplicationStyle::hitTestComplexControl(QStyle::ComplexControl _control,
    const QStyleOptionComplex* _option, const QPoint& _pos, const QWidget* _widget) const
{
    const SubControl _subcontrol = QProxyStyle::hitTestComplexControl(_control, _option, _pos, _widget);
    if (_control == CC_ScrollBar
        && (_subcontrol == SC_ScrollBarAddLine || _subcontrol == SC_ScrollBarSubLine)) {
            return SC_ScrollBarSlider;
    }
    return _subcontrol;
}

int ApplicationStyle::pixelMetric(QStyle::PixelMetric _metric, const QStyleOption* _option,
    const QWidget* _widget) const
{
    switch (_metric) {
        case PM_ScrollBarExtent: {
            return 0;
        }
        case PM_ToolTipLabelFrameWidth: {
            return static_cast<int>(Ui::DesignSystem::layout().px8());
        }
        default: {
            return QProxyStyle::pixelMetric(_metric, _option, _widget);
        }
    }
}

void ApplicationStyle::drawPrimitive(QStyle::PrimitiveElement _element, const QStyleOption* _option,
    QPainter* _painter, const QWidget* _widget) const
{
    if (_element == PE_IndicatorItemViewItemDrop) {
        _painter->setRenderHint(QPainter::Antialiasing, true);

        QColor indicatorColor(_widget->palette().text().color());

        //
        // Кисть для рисования линий
        //
        QPen pen(indicatorColor);
        pen.setWidthF(scaledValue(2.0));
        QBrush brush(indicatorColor);

        //
        // Настраиваем рисовальщика
        //
        _painter->setPen(pen);
        _painter->setBrush(brush);

        //
        // Элемент вставляется в конец списка
        //
        if (_option->rect.topLeft().isNull() && _option->rect.size().isEmpty()) {
            //
            // Рисуем вспомогательный треугольник внизу виджета
            //
            const qreal x = _widget->width() / 2.0;
            const qreal y = _widget->height() - scaledValue(16.0);
            QPolygonF treangle;
            treangle << QPointF(x, y)
                     << QPointF(x + scaledValue(7.0),  y + scaledValue(10.0))
                     << QPointF(x - scaledValue(7.0),  y + scaledValue(10.0));
            _painter->drawPolygon(treangle);
        }
        //
        // Элемент вставляется между двух соседних
        //
        else if (!_option->rect.topLeft().isNull() && _option->rect.size().isEmpty()) {
            //
            // Рисуем линию между элементов
            //
            _painter->drawLine(QPointF(_option->rect.topLeft().x() - scaledValue(10.0),
                                       _option->rect.topLeft().y()),
                               _option->rect.topRight());
            //
            // Вспомогательный треугольник
            //
            QPolygonF treangle;
            treangle <<  QPointF(_option->rect.topLeft().x() - scaledValue(10.0), _option->rect.topLeft().y() - scaledValue(4.0))
                     << QPointF(_option->rect.topLeft().x() - scaledValue(5.0),  _option->rect.topLeft().y())
                     << QPointF(_option->rect.topLeft().x() - scaledValue(10.0), _option->rect.topLeft().y() + scaledValue(4.0));
            _painter->drawPolygon(treangle);
        }
        //
        // Элемент вставляется в группирующий
        //
        else {
            //
            // Заливку делаем полупрозрачной
            //
            indicatorColor.setAlphaF(0.5);
            QBrush brush(indicatorColor);
            _painter->setBrush(brush);

            //
            // Расширим немного область, чтобы не перекрывать иконку
            //
            QRectF rect = _option->rect;
            rect.setX(rect.topLeft().x() - scaledValue(4.0));
            _painter->drawRoundedRect(rect, scaledValue(2.0), scaledValue(2.0));
        }
    }
    //
    // Рисуем индикатор элемента
    //
    else if (_element == PE_IndicatorBranch) {
        _painter->setRenderHint(QPainter::Antialiasing, true);

        //
        // Заливаем фон индикатора, если элемент под курсором, но не выделен
        //
        if (_option->state & State_MouseOver && !(_option->state & State_Selected)) {
            _painter->fillRect(_option->rect, _widget->palette().color(QPalette::AlternateBase));
        }

        //
        // Если у элемента есть дети, рисуем сам индикатор
        //
        if (_option->state & State_Children) {
            //
            // Кисть для рисования треугольника
            //
            auto indicatorColor(_option->state & State_Selected
                                ? _widget->palette().highlightedText().color()
                                : _widget->palette().text().color());
            if (!(_option->state & State_Selected) && !(_option->state & State_MouseOver)) {
                indicatorColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
            }
            _painter->setBrush(indicatorColor);

            //
            // Ручка для рисования линий
            //
            QPen pen(indicatorColor, 1.0);
            pen.setJoinStyle(Qt::MiterJoin);
            _painter->setPen(pen);

            //
            // Определим координаты треугольника в зависимости от состояния
            //
            QPolygonF triangle;
            const auto arrowHeight = Ui::DesignSystem::tree().arrowHeight();
            const auto arrowHalfWidth = Ui::DesignSystem::tree().arrowHalfWidth();
            //
            // ... открытый
            //
            if (_option->state & QStyle::State_Open) {
                triangle << QPointF(0.0, 0.0)
                         << QPointF(-arrowHalfWidth, -arrowHeight)
                         << QPointF(arrowHalfWidth, -arrowHeight);
            }
            //
            // ... закрытый
            //
            else {
                if (QLocale().textDirection() == Qt::LeftToRight) {
                    triangle << QPointF(0.0, 0.0)
                             << QPointF(-arrowHeight, -arrowHalfWidth)
                             << QPointF(-arrowHeight, arrowHalfWidth);
                } else {
                    triangle << QPointF(0.0, -arrowHalfWidth)
                             << QPointF(0.0, arrowHalfWidth)
                             << QPointF(-arrowHeight, 0.0);
                }
            }
            const QPointF distance = _option->rect.center() - triangle.boundingRect().center();
            triangle.translate(distance);

            //
            // Рисуем
            //
            _painter->drawPolygon(triangle);
        }
    }
    //
    // Всё остальное рисуем стандартным образом
    //
    else {
        QProxyStyle::drawPrimitive(_element, _option, _painter, _widget);
    }
}

int ApplicationStyle::styleHint(QStyle::StyleHint _hint, const QStyleOption* _option,
    const QWidget* _widget, QStyleHintReturn* _returnData) const
{
    if (_hint == QStyle::SH_ToolTip_Mask) {
        if (auto mask = qstyleoption_cast<QStyleHintReturnMask*>(_returnData)) {
            //
            // Обрезаем регион на единичку, чтобы не рисовалась рамка,
            // и срезаем края, чтобы получить эффект закруглённых углов
            //
            int x, y, w, h;
            _option->rect.adjusted(1, 1, -1, -1).getRect(&x, &y, &w, &h);
            QRegion toolTipRegion(x + 4, y + 0, w - 4*2, h - 0*2);
            toolTipRegion += QRegion(x + 0, y + 4, w - 0*2, h - 4*2);
            toolTipRegion += QRegion(x + 2, y + 1, w - 2*2, h - 1*2);
            toolTipRegion += QRegion(x + 1, y + 2, w - 1*2, h - 2*2);
            mask->region = toolTipRegion;
        }
        return true;
    }

    return QProxyStyle::styleHint(_hint, _option, _widget, _returnData);
}
