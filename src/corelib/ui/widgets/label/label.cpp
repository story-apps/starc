#include "label.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/text_helper.h>

#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>


class AbstractLabel::Implementation
{
public:
    QString text;
    Qt::Alignment alignment = Qt::AlignTop | Qt::AlignLeft;
    bool clickable = false;
};


// ****


AbstractLabel::AbstractLabel(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
}

QString AbstractLabel::text() const
{
    return d->text;
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

void AbstractLabel::setAlignment(Qt::Alignment _alignment)
{
    if (d->alignment == _alignment) {
        return;
    }

    d->alignment = _alignment;
    update();
}

void AbstractLabel::setClickable(bool _clickable)
{
    if (d->clickable == _clickable) {
        return;
    }

    setAttribute(Qt::WA_Hover, _clickable);
    d->clickable = _clickable;
    update();
}

QSize AbstractLabel::sizeHint() const
{
    const int width = TextHelper::fineTextWidth(d->text, textFont());
    const int height = QFontMetrics(textFont()).lineSpacing();
    return QRect(QPoint(0, 0), QSize(width, height)).marginsAdded(contentsMargins()).size();
}

int AbstractLabel::heightForWidth(int _width) const
{
    const int textWidth = _width - contentsMargins().left() - contentsMargins().right();
    const int textHeight
        = static_cast<int>(TextHelper::heightForWidth(d->text, textFont(), textWidth));
    return contentsMargins().top() + textHeight + contentsMargins().bottom();
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
    auto font = textFont();
    if (d->clickable && underMouse()) {
        font.setUnderline(true);
    }
    painter.setFont(font);
    painter.setPen(textColor());
    painter.setOpacity(isEnabled() ? 1.0 : Ui::DesignSystem::disabledTextOpacity());

    painter.drawText(
        contentsRect(),
        d->alignment
            | (d->text.isRightToLeft() ? Qt::TextForceRightToLeft : Qt::TextForceLeftToRight)
            | Qt::TextWordWrap,
        d->text);
}

void AbstractLabel::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    if (!rect().contains(_event->pos())) {
        return;
    }

    emit clicked();
}


// ****


H4Label::H4Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H4Label::textFont() const
{
    return Ui::DesignSystem::font().h4();
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


H6Label::H6Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H6Label::textFont() const
{
    return Ui::DesignSystem::font().h6();
}


// ****


Subtitle1Label::Subtitle1Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Subtitle1Label::textFont() const
{
    return Ui::DesignSystem::font().subtitle1();
}


// ****


Subtitle2Label::Subtitle2Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Subtitle2Label::textFont() const
{
    return Ui::DesignSystem::font().subtitle2();
}


// ****


Body1Label::Body1Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Body1Label::textFont() const
{
    return Ui::DesignSystem::font().body1();
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


// ****


CaptionLabel::CaptionLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& CaptionLabel::textFont() const
{
    return Ui::DesignSystem::font().caption();
}


// ****


OverlineLabel::OverlineLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& OverlineLabel::textFont() const
{
    return Ui::DesignSystem::font().overline();
}


// ****


IconsMidLabel::IconsMidLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

void IconsMidLabel::setIcon(const QString& _icon)
{
    setText(_icon);
}

const QFont& IconsMidLabel::textFont() const
{
    return Ui::DesignSystem::font().iconsMid();
}


// ****


IconsBigLabel::IconsBigLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

void IconsBigLabel::setIcon(const QString& _icon)
{
    setText(_icon);
}

const QFont& IconsBigLabel::textFont() const
{
    return Ui::DesignSystem::font().iconsBig();
}
