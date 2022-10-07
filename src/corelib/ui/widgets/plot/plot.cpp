#include "plot.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>


class Plot::Implementation
{
public:
    /**
     * @brief Позиция мыши
     */
    QPointF mousePos;

    /**
     * @brief Информация о графмке
     */
    QMap<qreal, QStringList> info;
};


// ****


Plot::Plot(QWidget* _parent)
    : QCustomPlot(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
}

Plot::~Plot() = default;

void Plot::setPlotInfo(const QMap<qreal, QStringList>& _info)
{
    if (d->info == _info) {
        return;
    }

    d->info = _info;
    update();
}

void Plot::paintEvent(QPaintEvent* _event)
{
    QCustomPlot::paintEvent(_event);

    //
    // Выводим доступные данные
    //
    if (!axisRect()->rect().contains(d->mousePos.toPoint())) {
        return;
    }

    //
    // Рисуем дополнительную информацию о графике
    //
    if (!underMouse()) {
        return;
    }

    QPainter painter(this);

    //
    // Мониторим положение мыши
    //
    painter.setPen(QPen(Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::layout().px()));
    painter.drawLine(QPoint(d->mousePos.x(), axisRect()->rect().top()),
                     QPoint(d->mousePos.x(), axisRect()->rect().bottom()));

    //
    // Определим ближайшую точку с информацией, которую нужно отобразить
    //
    const qreal mouseX = xAxis->pixelToCoord(d->mousePos.x());
    if (mouseX >= 0) {
        QMap<qreal, QStringList>::Iterator nearMouse = d->info.upperBound(mouseX);
        if (nearMouse != d->info.end()) {
            if (nearMouse != d->info.begin()) {
                --nearMouse;
            }
            const QString infoTitle = nearMouse.value().first();
            const QString infoText = nearMouse.value().last();

            //
            // Выводим информацию о текущем положении
            //
            if (!infoTitle.isEmpty() && !infoText.isEmpty()) {
                //
                // Определим область заголовка
                //
                QRect titleBoundingRect(
                    d->mousePos.x() + 20, d->mousePos.y(),
                    TextHelper::fineTextWidthF(infoTitle, Ui::DesignSystem::font().subtitle2())
                        + Ui::DesignSystem::layout().px24(),
                    TextHelper::fineLineSpacing(Ui::DesignSystem::font().subtitle2())
                        + Ui::DesignSystem::layout().px24());

                //
                // Определим область текста
                //
                auto textRectWidth = 0.0;
                auto textRectHeight = 0.0;
                for (const QString& line : infoText.split("\n")) {
                    const auto lineWidth
                        = TextHelper::fineTextWidthF(line, Ui::DesignSystem::font().body2());
                    if (lineWidth > textRectWidth) {
                        textRectWidth = lineWidth;
                    }
                    textRectHeight += TextHelper::fineLineSpacing(Ui::DesignSystem::font().body2());
                }
                QRect textBoundingRect(0, 0, textRectWidth + Ui::DesignSystem::layout().px24(),
                                       textRectHeight + Ui::DesignSystem::layout().px12());
                textBoundingRect.moveTo(titleBoundingRect.bottomLeft());

                //
                // Скорректируем области
                //
                if (titleBoundingRect.width() > textBoundingRect.width()) {
                    textBoundingRect.setWidth(titleBoundingRect.width());
                } else {
                    titleBoundingRect.setWidth(textBoundingRect.width());
                }
                //
                if (textBoundingRect.right() > width()) {
                    textBoundingRect.moveRight(d->mousePos.x() - 8);
                    titleBoundingRect.moveRight(d->mousePos.x() - 8);
                }

                //
                // Сформируем результирующую область
                //
                QRect fullBoundingRect(titleBoundingRect.topLeft(), textBoundingRect.bottomRight());

                //
                // Заливаем область и рисуем рамку
                //
                painter.setPen(Qt::NoPen);
                painter.setBrush(ColorHelper::transparent(Ui::DesignSystem::color().background(),
                                                          Ui::DesignSystem::inactiveTextOpacity()));
                painter.drawRoundedRect(fullBoundingRect, Ui::DesignSystem::card().borderRadius(),
                                        Ui::DesignSystem::card().borderRadius());

                //
                // Выводим тексты
                //
                painter.setFont(Ui::DesignSystem::font().subtitle2());
                painter.setPen(Ui::DesignSystem::color().onBackground());
                painter.drawText(
                    titleBoundingRect.adjusted(Ui::DesignSystem::layout().px12(), 0, 0, 0),
                    Qt::AlignLeft | Qt::AlignVCenter, infoTitle);
                //
                painter.setFont(Ui::DesignSystem::font().body2());
                painter.drawText(
                    textBoundingRect.adjusted(Ui::DesignSystem::layout().px12(), 0, 0, 0),
                    Qt::AlignLeft | Qt::AlignVCenter, infoText);
            }
        }
    }
}

void Plot::mouseMoveEvent(QMouseEvent* _event)
{
    QCustomPlot::mouseMoveEvent(_event);

    if (d->mousePos != _event->localPos()) {
        d->mousePos = _event->localPos();
        update();
    }
}
