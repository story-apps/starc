#include "toggle.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>

#include <QMouseEvent>
#include <QPainter>
#include <QVariantAnimation>


class Toggle::Implementation
{
public:
    explicit Implementation(Toggle* _q);

    /**
     * @brief Размер контента
     */
    QSizeF contentSize() const;

    /**
     * @brief Анимировать переключение
     */
    void animateToggle();

    /**
     * @brief Прекратить аниацию, если виджет невидим
     */
    void finishAnimationIfInvisible();


    Toggle* q = nullptr;
    bool isChecked = true;
    QVariantAnimation tumblerAnimation;
};

Toggle::Implementation::Implementation(Toggle* _q)
    : q(_q)
{
    tumblerAnimation.setDuration(160);
    tumblerAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

QSizeF Toggle::Implementation::contentSize() const
{
    return QSizeF(Ui::DesignSystem::layout().px24() + Ui::DesignSystem::toggle().trackSize().width()
                      + Ui::DesignSystem::toggle().tumblerOverflow() * 2,
                  Ui::DesignSystem::layout().px24()
                      + Ui::DesignSystem::toggle().tumblerSize().height());
}

void Toggle::Implementation::animateToggle()
{
    //
    // Определим крайние положения переключателя
    //

    const QRectF toggleLeftRect(
        { Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12() },
        Ui::DesignSystem::toggle().tumblerSize());
    const QRectF toggleRightRect({ Ui::DesignSystem::layout().px12()
                                       + Ui::DesignSystem::toggle().tumblerOverflow() * 2
                                       + Ui::DesignSystem::toggle().trackSize().width()
                                       - Ui::DesignSystem::toggle().tumblerSize().width(),
                                   Ui::DesignSystem::layout().px12() },
                                 Ui::DesignSystem::toggle().tumblerSize());
    if (isChecked) {
        tumblerAnimation.setStartValue(toggleLeftRect);
        tumblerAnimation.setEndValue(toggleRightRect);
    } else {
        tumblerAnimation.setStartValue(toggleRightRect);
        tumblerAnimation.setEndValue(toggleLeftRect);
    }
    tumblerAnimation.start();

    //
    // Если переключатель скрыт, то выполнять всю анимацию ни к чему
    //
    if (!q->isVisible()) {
        tumblerAnimation.setCurrentTime(tumblerAnimation.duration());
    }
}


// ****


Toggle::Toggle(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    d->animateToggle();

    connect(&d->tumblerAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&Toggle::update));
}

Toggle::~Toggle() = default;

void Toggle::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    d->animateToggle();
}

QSize Toggle::minimumSizeHint() const
{
    return d->contentSize().toSize();
}

void Toggle::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(_event->rect(), backgroundColor());

    const auto tumblerColor = d->isChecked
        ? Ui::DesignSystem::color().secondary()
        : (ColorHelper::isColorLight(backgroundColor()) ? backgroundColor() : textColor());
    const auto trackColor
        = d->isChecked ? tumblerColor.lighter(140) : ColorHelper::nearby(tumblerColor, 160);
    //
    // Трэк
    //
    painter.setBrush(trackColor);
    const QRectF trackRect(
        { Ui::DesignSystem::layout().px12() + Ui::DesignSystem::toggle().tumblerOverflow(),
          Ui::DesignSystem::layout().px12() + Ui::DesignSystem::toggle().tumblerOverflow() },
        Ui::DesignSystem::toggle().trackSize());

    painter.drawRoundedRect(trackRect, trackRect.height() / 2.0, trackRect.height() / 2.0);

    //
    // Переключатель
    //
    painter.setBrush(tumblerColor);
    const QRectF toggleRect = d->tumblerAnimation.currentValue().toRectF();
    const qreal borderRadius = toggleRect.height() / 2.0;
    //
    // ... подготовим тень
    //
    static QPixmap backgroundImage;
    if (backgroundImage.size() != toggleRect.size().toSize()) {
        backgroundImage = QPixmap(toggleRect.size().toSize());
        backgroundImage.fill(Qt::transparent);
        QPainter backgroundImagePainter(&backgroundImage);
        backgroundImagePainter.setPen(Qt::NoPen);
        backgroundImagePainter.setBrush(Ui::DesignSystem::color().textEditor());
        backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundImage.size()),
                                               borderRadius, borderRadius);
    }
    //
    // ... рисуем тень
    //
    const qreal shadowHeight = Ui::DesignSystem::card().minimumShadowBlurRadius();
    const bool useCache = true;
    const QPixmap shadow
        = ImageHelper::dropShadow(backgroundImage, Ui::DesignSystem::card().shadowMargins(),
                                  shadowHeight, Ui::DesignSystem::color().shadow(), useCache);
    painter.drawPixmap(toggleRect.topLeft()
                           - QPointF{ Ui::DesignSystem::card().shadowMargins().left(),
                                      Ui::DesignSystem::card().shadowMargins().top() },
                       shadow);
    //
    // ... рисуем сам переключатель
    //
    painter.drawRoundedRect(toggleRect, toggleRect.width() / 2.0, toggleRect.width() / 2.0);
}

void Toggle::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);
}

void Toggle::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    d->isChecked = !d->isChecked;
    d->animateToggle();

    emit checkedChanged(d->isChecked);
}
