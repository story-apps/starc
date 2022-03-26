#include "percent_radio_button.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QPainter>
#include <QPainterPath>


namespace {
const int kMinPercents = 0;
const int kMaxPercents = 100;
} // namespace

PercentRadioButton::PercentRadioButton(QWidget* _parent, int _percents)
    : RadioButton(_parent)
    , percents(_percents)
{
    Q_ASSERT(kMinPercents <= percents && percents <= kMaxPercents);
}

void PercentRadioButton::paintBox(QPainter& _painter, const QRectF& _rect, const QColor& _penColor)
{
    //
    // Рисуем фоновую подложку
    //
    _painter.setFont(Ui::DesignSystem::font().iconsMid());
    _painter.setPen(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::focusBackgroundOpacity()));
    _painter.drawText(_rect, Qt::AlignCenter, u8"\U000f043d");

    //
    // В противном случае, ограничиваем область отрисовки, чтобы показать прогресс перевода
    //
    QPainterPath arcPath(_rect.center());
    arcPath.arcTo(_rect, 0, 3.6 * percents);
    QPainterPath centerPath;
    const auto margin = Ui::DesignSystem::layout().px(5);
    const auto centerRect = _rect.adjusted(margin, margin, -margin, -margin);
    const auto radius = centerRect.width() / 2.0;
    centerPath.addRoundedRect(centerRect, radius, radius);

    _painter.setClipPath(arcPath.united(centerPath));
    RadioButton::paintBox(_painter, _rect, _penColor);
    _painter.setClipping(false);
}
