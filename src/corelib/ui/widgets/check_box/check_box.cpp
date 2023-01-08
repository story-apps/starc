#include "check_box.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>


class CheckBox::Implementation
{
public:
    /**
     * @brief Установлена ли галочка
     */
    bool isChecked = false;
    bool isIndeterminate = false;

    /**
     * @brief Текст
     */
    QString text;

    /**
     * @brief Цвет галочки
     */
    QColor checkMarkColor;

    /**
     * @brief  Декорации переключателя при клике
     */
    ClickAnimation decorationAnimation;
};


// ****


CheckBox::CheckBox(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&CheckBox::update));
}

CheckBox::~CheckBox() = default;

bool CheckBox::isChecked() const
{
    return d->isChecked;
}

void CheckBox::setChecked(bool _checked)
{
    if ((_checked && d->isChecked) || (!_checked && !d->isChecked && !d->isIndeterminate)) {
        return;
    }

    d->isChecked = _checked;
    d->isIndeterminate = false;
    emit checkedChanged(d->isChecked, d->isIndeterminate);
    update();
}

bool CheckBox::isIndeterminate() const
{
    return d->isIndeterminate;
}

void CheckBox::setIndeterminate()
{
    if (d->isIndeterminate) {
        return;
    }

    d->isIndeterminate = true;
    d->isChecked = false;
    emit checkedChanged(d->isChecked, d->isIndeterminate);
    update();
}

QString CheckBox::text() const
{
    return d->text;
}

void CheckBox::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    updateGeometry();
    update();
}

void CheckBox::setCheckMarkColor(const QColor& _color)
{
    if (d->checkMarkColor == _color) {
        return;
    }

    d->checkMarkColor = _color;
    update();
}

QSize CheckBox::minimumSizeHint() const
{
    return sizeHint();
}

QSize CheckBox::sizeHint() const
{
    return QSize(
        static_cast<int>(Ui::DesignSystem::checkBox().margins().left()
                         + Ui::DesignSystem::checkBox().iconSize().width()
                         + Ui::DesignSystem::checkBox().spacing()
                         + TextHelper::fineTextWidthF(d->text, Ui::DesignSystem::font().subtitle1())
                         + Ui::DesignSystem::checkBox().margins().right()),
        static_cast<int>(Ui::DesignSystem::checkBox().height()));
}

void CheckBox::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Позиционируем рисовальщика
    //
    painter.translate(contentsRect().topLeft());

    //
    // Настраиваем размещение текста для разных языков
    //
    qreal textRectX = 0;
    QRectF iconRect;
    QRectF textRect;

    if (isLeftToRight()) {
        iconRect.setRect(Ui::DesignSystem::checkBox().margins().left(),
                         Ui::DesignSystem::checkBox().margins().top(),
                         Ui::DesignSystem::checkBox().iconSize().width(),
                         Ui::DesignSystem::checkBox().iconSize().height());

        textRectX = iconRect.right() + Ui::DesignSystem::checkBox().spacing();
        textRect.setRect(textRectX, 0, width() - textRectX, sizeHint().height());
    } else {
        const auto textWidth = width() - Ui::DesignSystem::checkBox().margins().left()
            - Ui::DesignSystem::checkBox().spacing()
            - Ui::DesignSystem::checkBox().iconSize().width()
            - Ui::DesignSystem::checkBox().margins().right();

        textRectX = Ui::DesignSystem::checkBox().margins().left();
        textRect.setRect(textRectX, 0, textWidth, sizeHint().height());
        iconRect.setRect(textRectX + textWidth + Ui::DesignSystem::checkBox().spacing(),
                         Ui::DesignSystem::checkBox().margins().top(),
                         Ui::DesignSystem::checkBox().iconSize().width(),
                         Ui::DesignSystem::checkBox().iconSize().height());
    }

    //
    // Рисуем декорацию переключателя
    //
    const auto checkMarkColor
        = d->checkMarkColor.isValid() ? d->checkMarkColor : Ui::DesignSystem::color().accent();
    if (underMouse() || hasFocus()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(isChecked() || isIndeterminate() ? checkMarkColor : textColor());
        painter.setOpacity(hasFocus() ? Ui::DesignSystem::focusBackgroundOpacity()
                                      : Ui::DesignSystem::hoverBackgroundOpacity());
        const auto radius = d->decorationAnimation.maximumRadius();
        painter.drawEllipse(iconRect.center(), radius, radius);
        painter.setOpacity(1.0);
    }
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(isChecked() ? checkMarkColor : textColor());
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
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.setPen(isEnabled() && (isChecked() || isIndeterminate()) ? checkMarkColor : penColor);
    painter.drawText(iconRect, Qt::AlignCenter,
                     d->isChecked ? u8"\U000F0132"
                                  : (d->isIndeterminate ? u8"\U000F0375" : u8"\U000F0131"));


    //
    // Рисуем текст
    //
    painter.setFont(Ui::DesignSystem::font().subtitle1());
    painter.setPen(penColor);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(d->text, Qt::ElideRight, textRect.width()));
}

void CheckBox::mousePressEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)
    d->decorationAnimation.start();
}

void CheckBox::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!rect().contains(_event->pos())) {
        return;
    }

    if (d->isChecked || d->isIndeterminate) {
        setChecked(false);
    } else {
        setChecked(true);
    }
}

void CheckBox::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Space) {
        _event->accept();
        d->decorationAnimation.start();
        setChecked(!isChecked());
        return;
    }

    Widget::keyPressEvent(_event);
}

void CheckBox::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->decorationAnimation.setRadiusInterval(Ui::DesignSystem::radioButton().iconSize().height()
                                                 / 2.0,
                                             Ui::DesignSystem::radioButton().height() / 2.5);

    updateGeometry();
    update();
}
