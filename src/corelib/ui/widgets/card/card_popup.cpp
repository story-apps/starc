#include "card_popup.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QApplication>
#include <QBoxLayout>
#include <QScreen>
#include <QVariantAnimation>


class CardPopup::Implementation
{
public:
    explicit Implementation();

    QVariantAnimation positionAnimation;
    QVariantAnimation heightAnimation;
};

CardPopup::Implementation::Implementation()
{
    positionAnimation.setEasingCurve(QEasingCurve::OutQuint);
    positionAnimation.setDuration(240);
    heightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    heightAnimation.setDuration(240);
    heightAnimation.setStartValue(1);
}


// ****


CardPopup::CardPopup(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    hide();


    connect(&d->positionAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { move(x(), _value.toInt()); });
    connect(&d->heightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { resize(width(), _value.toInt()); });
    connect(&d->heightAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->heightAnimation.direction() == QVariantAnimation::Backward
            && d->heightAnimation.currentValue().toInt() == d->heightAnimation.startValue()) {
            hide();
        }
    });
}

CardPopup::~CardPopup() = default;

void CardPopup::showPopup(const QPoint& _position, int _parentHeight)
{
    showPopup(_position, _parentHeight,
              sizeHint()
                  - QSize(0,
                          Ui::DesignSystem::card().shadowMargins().top()
                              + Ui::DesignSystem::card().shadowMargins().bottom()));
}

void CardPopup::showPopup(const QPoint& _position, int _parentHeight, const QSize& _size)
{
    //
    // Определим высоту попапа
    //
    const auto finalHeight = _size.height() + Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::card().shadowMargins().bottom();
    d->heightAnimation.setEndValue(static_cast<int>(finalHeight));

    //
    // Прикидываем размещение попапа на экране
    //
    const auto screen = QApplication::screenAt(_position);
    Q_ASSERT(screen);
    const auto screenGeometry = screen->geometry();
    auto position = _position;
    //
    // ... если попап не вмещается в нижнюю часть экрана и при этом сверху больше места,
    //     то показываем попап вверх
    //
    if (position.y() + finalHeight > screenGeometry.bottom()
        && screenGeometry.bottom() - position.y() < position.y() - screenGeometry.top()) {
        position.setY(position.y() - _parentHeight);
        d->positionAnimation.setStartValue(position.y());
        d->positionAnimation.setEndValue(position.y() - static_cast<int>(finalHeight));
    }
    //
    // ... в остальных случаях показываем попап вниз
    //
    else {
        d->positionAnimation.setStartValue(position.y());
        d->positionAnimation.setEndValue(position.y());
    }

    resize(_size.width(), 1);
    move(position);
    show();

    d->positionAnimation.setDirection(QVariantAnimation::Forward);
    d->positionAnimation.start();
    d->heightAnimation.setDirection(QVariantAnimation::Forward);
    d->heightAnimation.start();
}

void CardPopup::hidePopup()
{
    if (d->heightAnimation.endValue().toInt() != sizeHint().height()) {
        d->heightAnimation.setEndValue(sizeHint().height());
    }

    d->positionAnimation.setDirection(QVariantAnimation::Backward);
    d->positionAnimation.start();
    d->heightAnimation.setDirection(QVariantAnimation::Backward);
    d->heightAnimation.start();
}
