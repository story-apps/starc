#include "avatar.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>


namespace Ui {

class Avatar::Implementation
{
public:
    Implementation();

    void prepreAvatarForPainting(const QSize& _size);

    QPixmap avatar;
    QPixmap preparedAvatar;

    QString overlayText;
    QVariantAnimation overlayOpacityAnimation;
};

Avatar::Implementation::Implementation()
{
    overlayOpacityAnimation.setDuration(240);
    overlayOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    overlayOpacityAnimation.setStartValue(0.0);
    overlayOpacityAnimation.setEndValue(1.0);
}

void Avatar::Implementation::prepreAvatarForPainting(const QSize& _size)
{
    if (avatar.isNull()) {
        return;
    }

    if (preparedAvatar.size() == _size) {
        return;
    }

    preparedAvatar = avatar.scaled(_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


// ****


Avatar::Avatar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    connect(&d->overlayOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
}

Avatar::~Avatar() = default;

void Avatar::setAvatar(const QPixmap& _avatar)
{
    if ((d->avatar.isNull() && _avatar.isNull())
        || (!d->avatar.isNull() && !_avatar.isNull() && d->avatar == _avatar)) {
        return;
    }

    d->avatar = _avatar;
    d->preparedAvatar = {};
    d->prepreAvatarForPainting(size());
    updateGeometry();
    update();
}

void Avatar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    if (d->preparedAvatar.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, d->preparedAvatar);

    if (d->overlayOpacityAnimation.currentValue().toReal() > 0) {
        painter.setOpacity(d->overlayOpacityAnimation.currentValue().toReal());

        painter.fillRect(rect(), Ui::DesignSystem::color().shadow());

        painter.setFont(Ui::DesignSystem::font().button());
        painter.setPen(Ui::DesignSystem::color().onShadow());
        painter.drawText(rect(), Qt::AlignCenter, d->overlayText);
    }
}

void Avatar::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    if (_event->size().height() != _event->size().width()) {
        setFixedWidth(_event->size().height());
        return;
    }

    d->prepreAvatarForPainting(_event->size());
}

void Avatar::enterEvent(QEvent* _event)
{
    Widget::enterEvent(_event);

    d->overlayOpacityAnimation.setDirection(QVariantAnimation::Forward);
    d->overlayOpacityAnimation.start();
}

void Avatar::leaveEvent(QEvent* _event)
{
    Widget::leaveEvent(_event);

    d->overlayOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->overlayOpacityAnimation.start();
}

void Avatar::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    if (underMouse()) {
        emit clicked();
    }
}

void Avatar::updateTranslations()
{
    d->overlayText = tr("Select...");
}

} // namespace Ui
