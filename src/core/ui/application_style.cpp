#include "application_style.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QStyleOption>


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
    if (_metric == PM_ScrollBarExtent) {
        return 0;
    }

    return QProxyStyle::pixelMetric(_metric, _option, _widget);
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
        pen.setWidth(2);
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
            int x = _widget->width() / 2;
            int y = _widget->height() - 16;
            QPolygonF treangle;
            treangle <<  QPointF(x, y)
                     << QPointF(x + 7,  y + 10)
                     << QPointF(x - 7,  y + 10);
            _painter->drawPolygon(treangle);
        }
        //
        // Элемент вставляется между двух соседних
        //
        else if (!_option->rect.topLeft().isNull() && _option->rect.size().isEmpty()) {
            //
            // Рисуем линию между элементов
            //
            _painter->drawLine(QPoint(_option->rect.topLeft().x() - 10,
                                     _option->rect.topLeft().y()),
                              _option->rect.topRight());
            //
            // Вспомогательный треугольник
            //
            QPolygonF treangle;
            treangle <<  QPointF(_option->rect.topLeft().x() - 10, _option->rect.topLeft().y() - 4)
                     << QPointF(_option->rect.topLeft().x() - 5,  _option->rect.topLeft().y())
                     << QPointF(_option->rect.topLeft().x() - 10, _option->rect.topLeft().y() + 4);
            _painter->drawPolygon(treangle);
        }
        //
        // Элемент вставляется в группирующий
        //
        else {
            //
            // Заливку делаем полупрозрачной
            //
            indicatorColor.setAlpha(50);
            QBrush brush(indicatorColor);
            _painter->setBrush(brush);

            //
            // Расширим немного область, чтобы не перекрывать иконку
            //
            QRect rect = _option->rect;
            rect.setX(rect.topLeft().x() - 4);
            _painter->drawRoundedRect(rect, 2, 2);
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
