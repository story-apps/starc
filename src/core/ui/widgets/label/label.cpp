#include "label.h"

#include <ui/design_system/design_system.h>

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>


class AbstractLabel::Implementation
{
public:
    QString text;
};


// ****


AbstractLabel::AbstractLabel(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setContentsMargins(24, 24, 24, 24);
}

AbstractLabel::~AbstractLabel() = default;

void AbstractLabel::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    update();
}

QSize AbstractLabel::sizeHint() const
{
    return QFontMetrics(textFont()).boundingRect(d->text).marginsAdded(contentsMargins()).size();
}

void AbstractLabel::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем текст
    //
    painter.setFont(textFont());
    painter.setPen(textColor());
    painter.drawText(contentsRect(), Qt::AlignTop | Qt::AlignLeft, d->text);
}


// ****


H5Label::H5Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H5Label::textFont() const
{
    return Ui::DesignSystem::font().h5();
}
