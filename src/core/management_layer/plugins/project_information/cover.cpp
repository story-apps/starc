#include "cover.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>


namespace Ui {

class Cover::Implementation
{
public:
    Implementation();

    void prepreCoverForPainting(const QSize& _size);

    QPixmap cover;
    QPixmap preparedCover;

    QString overlayText;
    QVariantAnimation overlayOpacityAnimation;
};

Cover::Implementation::Implementation()
{
    overlayOpacityAnimation.setDuration(240);
    overlayOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    overlayOpacityAnimation.setStartValue(0.0);
    overlayOpacityAnimation.setEndValue(1.0);
}

void Cover::Implementation::prepreCoverForPainting(const QSize& _size)
{
    if (cover.isNull()) {
        return;
    }

    //
    // Добавляем небольшую дельту, чтобы постер занимал всё доступное пространство
    //
    const auto sizeCorrected = _size + QSize(2, 2);
    if (preparedCover.size() == sizeCorrected) {
        return;
    }

    preparedCover = cover.scaled(sizeCorrected, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


// ****


Cover::Cover(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    connect(&d->overlayOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

Cover::~Cover() = default;

void Cover::setCover(const QPixmap& _cover)
{
    if ((d->cover.isNull() && _cover.isNull())
        || (!d->cover.isNull() && !_cover.isNull() && d->cover.cacheKey() == _cover.cacheKey())) {
        return;
    }

    d->cover = _cover;
    d->preparedCover = {};
    d->prepreCoverForPainting(size());
    updateGeometry();
    update();
}

void Cover::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    if (d->preparedCover.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, d->preparedCover);

    if (d->overlayOpacityAnimation.currentValue().toReal() > 0) {
        painter.setOpacity(d->overlayOpacityAnimation.currentValue().toReal());

        painter.fillRect(rect(), Ui::DesignSystem::color().shadow());

        painter.setFont(Ui::DesignSystem::font().button());
        painter.setPen(Ui::DesignSystem::color().onShadow());
        painter.drawText(rect(), Qt::AlignCenter, d->overlayText);
    }
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void Cover::enterEvent(QEnterEvent* _event)
#else
void Cover::enterEvent(QEvent* _event)
#endif
{
    Widget::enterEvent(_event);

    d->overlayOpacityAnimation.setDirection(QVariantAnimation::Forward);
    d->overlayOpacityAnimation.start();
}

void Cover::leaveEvent(QEvent* _event)
{
    Widget::leaveEvent(_event);

    d->overlayOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->overlayOpacityAnimation.start();
}

void Cover::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    if (underMouse()) {
        emit clicked();
    }
}

void Cover::updateTranslations()
{
    d->overlayText = tr("Select...");
}

void Cover::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setFixedSize(QSize(400, 533) * Ui::DesignSystem::scaleFactor());
}

} // namespace Ui
