#include "button.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QPaintEvent>


class Button::Implementation
{
public:
    QString text;
};

Button::Button(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{

}

void Button::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    update();
}

void Button::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем текст
    //
    painter.setFont(Ui::DesignSystem::font().button());
    painter.setPen(textColor());
    painter.drawText(rect(), Qt::AlignCenter, d->text);
}

void Button::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);
    emit clicked();
}

Button::~Button() = default;
