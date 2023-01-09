#include "radio_button.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>

class RadioButton::Implementation
{
public:
    bool isChecked = false;
    QString text;

    /**
     * @brief  Декорации переключателя при клике
     */
    ClickAnimation decorationAnimation;
};


// ****


RadioButton::RadioButton(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&RadioButton::update));

    designSystemChangeEvent(nullptr);
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
    updateGeometry();
    update();
}

QSize RadioButton::minimumSizeHint() const
{
    return sizeHint();
}

QSize RadioButton::sizeHint() const
{
    return QSize(
        static_cast<int>(Ui::DesignSystem::radioButton().margins().left()
                         + Ui::DesignSystem::radioButton().iconSize().width()
                         + Ui::DesignSystem::radioButton().spacing()
                         + TextHelper::fineTextWidthF(d->text, Ui::DesignSystem::font().subtitle1())
                         + Ui::DesignSystem::radioButton().margins().right()),
        static_cast<int>(Ui::DesignSystem::radioButton().height()));
}

void RadioButton::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Настраиваем размещение текста для разных языков
    //
    qreal textRectX = 0;
    qreal textWidth = 0;
    QRectF textRect;
    QRectF iconRect;

    if (isLeftToRight()) {
        iconRect.setRect(Ui::DesignSystem::radioButton().margins().left(),
                         Ui::DesignSystem::radioButton().margins().top(),
                         Ui::DesignSystem::radioButton().iconSize().width(),
                         Ui::DesignSystem::radioButton().iconSize().height());

        textRectX = iconRect.right() + Ui::DesignSystem::radioButton().spacing();
        textWidth = width() - textRectX - Ui::DesignSystem::radioButton().margins().right();
        textRect.setRect(textRectX, 0, width() - textRectX, sizeHint().height());
    } else {
        textWidth = width() - Ui::DesignSystem::radioButton().margins().left()
            - Ui::DesignSystem::radioButton().spacing()
            - Ui::DesignSystem::radioButton().iconSize().width()
            - Ui::DesignSystem::radioButton().margins().right();

        textRectX = Ui::DesignSystem::radioButton().margins().left();
        textRect.setRect(textRectX, 0, textWidth, sizeHint().height());
        iconRect.setRect(textRectX + textWidth + Ui::DesignSystem::radioButton().spacing(),
                         Ui::DesignSystem::radioButton().margins().top(),
                         Ui::DesignSystem::radioButton().iconSize().width(),
                         Ui::DesignSystem::radioButton().iconSize().height());
    }
    //
    // Рисуем декорацию переключателя
    //
    if (underMouse() || hasFocus()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(isChecked() ? Ui::DesignSystem::color().accent() : textColor());
        painter.setOpacity(hasFocus() ? Ui::DesignSystem::focusBackgroundOpacity()
                                      : Ui::DesignSystem::hoverBackgroundOpacity());
        const auto radius = d->decorationAnimation.maximumRadius();
        painter.drawEllipse(iconRect.center(), radius, radius);
        painter.setOpacity(1.0);
    }
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(isChecked() ? Ui::DesignSystem::color().accent() : textColor());
        painter.setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        painter.drawEllipse(iconRect.center(), radius, radius);
        painter.setOpacity(1.0);
    }

    const auto penColor = isEnabled()
        ? textColor()
        : ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity());

    //
    // Рисуем сам переключатель
    //
    paintBox(painter, iconRect, penColor);
    //
    // Текст
    //
    painter.setFont(Ui::DesignSystem::font().subtitle1());
    painter.setPen(penColor);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(d->text, Qt::ElideRight, textRect.width()));
}

void RadioButton::paintBox(QPainter& _painter, const QRectF& _rect, const QColor& _penColor)
{
    _painter.setFont(Ui::DesignSystem::font().iconsMid());
    _painter.setPen(isEnabled() && d->isChecked ? Ui::DesignSystem::color().accent() : _penColor);
    _painter.drawText(_rect, Qt::AlignCenter, d->isChecked ? u8"\U000f043e" : u8"\U000f043d");
}

void RadioButton::mousePressEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);
    d->decorationAnimation.start();
}

void RadioButton::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!rect().contains(_event->pos())) {
        return;
    }

    setChecked(true);
}

void RadioButton::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Space) {
        _event->accept();
        d->decorationAnimation.start();
        setChecked(true);
        return;
    }

    Widget::keyPressEvent(_event);
}

void RadioButton::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->decorationAnimation.setRadiusInterval(Ui::DesignSystem::radioButton().iconSize().height()
                                                 / 2.0,
                                             Ui::DesignSystem::radioButton().height() / 2.5);

    updateGeometry();
    update();
}
