#include "account_bar.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QAction>
#include <QPainter>
#include <QVariantAnimation>


namespace Ui
{

class AccountBar::Implementation
{
public:
    Implementation();

    QAction* accountAction = nullptr;

    QColor notificationColor;
    bool keepNotificationVisible = false;
    QVariantAnimation notificationAngleAnimation;
    QVariantAnimation notificationOpacityAnimation;
};

AccountBar::Implementation::Implementation()
    : accountAction(new QAction)
{
    notificationAngleAnimation.setEasingCurve(QEasingCurve::OutQuad);
    notificationAngleAnimation.setStartValue(0.0);
    notificationAngleAnimation.setEndValue(1.0);
    notificationAngleAnimation.setDuration(340);
    notificationOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    notificationOpacityAnimation.setStartValue(1.0);
    notificationOpacityAnimation.setEndValue(0.0);
    notificationOpacityAnimation.setDuration(200);
}


// ****


AccountBar::AccountBar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation)
{
    d->accountAction->setIconText(u8"\U000f0004");
    addAction(d->accountAction);
    hide();

    connect(&d->notificationAngleAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->notificationAngleAnimation, &QVariantAnimation::finished, this, [this] {
        d->notificationOpacityAnimation.start();
    });
    connect(&d->notificationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(d->accountAction, &QAction::triggered, this, &AccountBar::accountPressed);
}

AccountBar::~AccountBar() = default;

void AccountBar::setAvatar(const QPixmap& _avatar)
{
    if (_avatar.isNull()) {
        d->accountAction->setIcon({});
        return;
    }

    const int maxAvatarSideSize = std::max(_avatar.width(), _avatar.height());
    const QSize maxAvatarSize(maxAvatarSideSize, maxAvatarSideSize);
    QPixmap avatar = ImageHelper::makeAvatar(_avatar, maxAvatarSize);
    d->accountAction->setIcon(QIcon(avatar));
}

void AccountBar::notify(const QColor& _color)
{
    if (d->notificationColor == _color) {
        return;
    }

    d->notificationColor = _color;
    d->notificationOpacityAnimation.setCurrentTime(0);
    d->notificationAngleAnimation.start();
}

void AccountBar::paintEvent(QPaintEvent* _event)
{
    FloatingToolBar::paintEvent(_event);

    if (d->keepNotificationVisible
        || d->notificationAngleAnimation.state() == QVariantAnimation::Running
        || d->notificationOpacityAnimation.state() == QVariantAnimation::Running) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (d->notificationOpacityAnimation.currentValue().isValid()
            && !d->keepNotificationVisible) {
            painter.setOpacity(d->notificationOpacityAnimation.currentValue().toReal());
        }

        //
        // Заполнение
        //
        QPen pen;
        pen.setWidthF(Ui::DesignSystem::slider().trackHeight());
        pen.setColor(d->notificationColor);
        painter.setPen(pen);
        qreal startAngle = 90 * 16;
        qreal valueCorrected = -1 * d->notificationAngleAnimation.currentValue().toReal();
        qreal spanAngle = valueCorrected * 360 * 16;
        const QRectF rectangle = QRectF(rect()).marginsRemoved(Ui::DesignSystem::floatingToolBar().shadowMargins());
        painter.drawArc(rectangle, static_cast<int>(startAngle), static_cast<int>(spanAngle));
    }
}

} // namespace Ui
