#include "radio_button.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QPaintEvent>


class RadioButton::Implementation
{
public:
    bool isChecked = false;
    QString text;
};


// ****


RadioButton::RadioButton(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
}

RadioButton::~RadioButton() = default;

bool RadioButton::isChecked() const
{
    return d->isChecked;
}

void RadioButton::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    emit checkedChanged(d->isChecked);
    update();
}

void RadioButton::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    update();
}

QSize RadioButton::sizeHint() const
{
    return QSize(static_cast<int>(Ui::DesignSystem::radioButton().margins().left()
                                  + Ui::DesignSystem::radioButton().iconSize().width()
                                  + Ui::DesignSystem::radioButton().spacing()
                                  + QFontMetrics(Ui::DesignSystem::font().subtitle1()).horizontalAdvance(d->text)
                                  + Ui::DesignSystem::radioButton().margins().right()),
                 static_cast<int>(Ui::DesignSystem::radioButton().height()));
}

void RadioButton::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем сам переключатель
    //
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.setPen(d->isChecked ? Ui::DesignSystem::color().secondary() : textColor());
    const QRectF iconRect(QPointF(Ui::DesignSystem::radioButton().margins().left(),
                                  Ui::DesignSystem::radioButton().margins().top()),
                          Ui::DesignSystem::radioButton().iconSize());
    painter.drawText(iconRect, Qt::AlignCenter, d->isChecked ? "\uf43e" : "\uf43d");

    //
    // Рисуем текст
    //
    painter.setFont(Ui::DesignSystem::font().subtitle1());
    painter.setPen(textColor());
    const qreal textRectX = iconRect.right() + Ui::DesignSystem::radioButton().spacing();
    const QRectF textRect(textRectX, 0, width() - textRectX, height());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, d->text);
}

void RadioButton::enterEvent(QEvent* _event)
{
    Q_UNUSED(_event);
}

void RadioButton::leaveEvent(QEvent* _event)
{
    Q_UNUSED(_event);

}

void RadioButton::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    setChecked(true);
}
