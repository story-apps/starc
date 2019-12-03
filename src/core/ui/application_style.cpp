#include "application_style.h"

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
    if (_element == QStyle::PE_IndicatorItemViewItemDrop) {
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
    // Рисуем индикатор элемента, если у него есть дети
    //
    else if (_element == QStyle::PE_IndicatorBranch
               && _option->state & QStyle::State_Children) {
        _painter->setRenderHint(QPainter::Antialiasing, true);

        QColor indicatorColor(_widget->palette().text().color());

        //
        // Кисть для рисования линий
        //
        QPen pen(indicatorColor);
        pen.setWidth(1);
        pen.setJoinStyle(Qt::MiterJoin);
        QBrush brush(indicatorColor);

        //
        // Настраиваем рисовальщика
        //
        _painter->setPen(pen);
        _painter->setBrush(brush);
        //
        // FIXME:
        //
        const int arrowHeight = 5;
        const int arrowHalfWidth = 4;
        //
        // ... открытый
        //
        if (_option->state & QStyle::State_Open) {
            int x = _option->rect.center().x();
            int y = _option->rect.center().y() + arrowHeight/2;
            QPolygon treangle;
            treangle << QPoint(x, y)
                     << QPoint(x - arrowHalfWidth,  y - arrowHeight)
                     << QPoint(x + arrowHalfWidth,  y - arrowHeight);
            _painter->drawPolygon(treangle);
        }
        //
        // ... закрытый
        //
        else {
            int x = _option->rect.center().x() + arrowHeight/2;
            int y = _option->rect.center().y();
            QPolygon treangle;
            if (QLocale().textDirection() == Qt::LeftToRight) {
                treangle << QPoint(x, y)
                         << QPoint(x - arrowHeight,  y - arrowHalfWidth)
                         << QPoint(x - arrowHeight,  y + arrowHalfWidth);
            } else {
                treangle << QPoint(x,  y - arrowHalfWidth)
                         << QPoint(x, y + arrowHalfWidth)
                         << QPoint(x - arrowHeight,  y);
            }
            _painter->drawPolygon(treangle);
        }
    }
    //
    // Всё остальное рисуем стандартным образом
    //
    else {
        QProxyStyle::drawPrimitive(_element, _option, _painter, _widget);
    }
}
