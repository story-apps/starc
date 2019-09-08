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
}

AbstractLabel::~AbstractLabel() = default;

void AbstractLabel::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    updateGeometry();
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


// ****


Body1Label::Body1Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont&Body1Label::textFont() const
{
    return Ui::DesignSystem::font().body1();
}


// ****


H6Label::H6Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H6Label::textFont() const
{
    return Ui::DesignSystem::font().h6();
}


// ****


Body2Label::Body2Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Body2Label::textFont() const
{
    return Ui::DesignSystem::font().body2();
}
