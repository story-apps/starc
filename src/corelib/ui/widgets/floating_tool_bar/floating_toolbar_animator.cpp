#include "floating_toolbar_animator.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QVariantAnimation>


class FloatingToolbarAnimator::Implementation
{
public:
    QString targetIcon;
    QPointF targetIconStartPosition;
    Widget* sourceWidget = nullptr;
    QPixmap sourceWidgetImage;
    Widget* targetWidget = nullptr;
    QPixmap targetWidgetImage;

    QVariantAnimation geometryAnimation;
    QVariantAnimation iconPositionAnimation;
    QVariantAnimation opacityAnimation;
};


// ****


FloatingToolbarAnimator::FloatingToolbarAnimator(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation)
{
    hide();

    d->geometryAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->geometryAnimation.setDuration(240);
    connect(&d->geometryAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { setGeometry(_value.toRect()); });
    connect(&d->geometryAnimation, &QVariantAnimation::finished, this, [this] {
        hide();
        if (d->geometryAnimation.direction() == QVariantAnimation::Forward) {
            d->targetWidget->show();
            d->targetWidget->setFocus();
            d->targetWidget->setOpacity(opacity());
        } else {
            d->sourceWidget->show();
            d->sourceWidget->setOpacity(opacity());
        }
    });

    d->iconPositionAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->iconPositionAnimation.setDuration(240);
    connect(&d->iconPositionAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });

    d->opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->opacityAnimation.setDuration(240);
    d->opacityAnimation.setStartValue(1.0);
    d->opacityAnimation.setEndValue(0.0);
    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
}

FloatingToolbarAnimator::~FloatingToolbarAnimator() = default;

void FloatingToolbarAnimator::switchToolbars(const QString& _targetIcon,
                                             const QPointF _targetIconStartPosition,
                                             Widget* _sourceWidget, Widget* _targetWidget)
{
    if (_sourceWidget == nullptr || _targetWidget == nullptr) {
        return;
    }

    d->targetIcon = _targetIcon;
    d->targetIconStartPosition = _targetIconStartPosition;
    d->sourceWidget = _sourceWidget;
    d->targetWidget = _targetWidget;
    d->targetWidget->setOpacity(d->sourceWidget->opacity());

    d->sourceWidgetImage = d->sourceWidget->grab();
    d->targetWidget->show();
    d->targetWidgetImage = d->targetWidget->grab();
    d->targetWidget->hide();

    d->geometryAnimation.setStartValue(d->sourceWidget->geometry());
    d->geometryAnimation.setEndValue(d->targetWidget->geometry());
    d->iconPositionAnimation.setStartValue(_targetIconStartPosition);
    d->iconPositionAnimation.setEndValue(
        QPointF(Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                    + Ui::DesignSystem::floatingToolBar().margins().left(),
                Ui::DesignSystem::floatingToolBar().shadowMargins().top()
                    + Ui::DesignSystem::floatingToolBar().margins().top()));

    setOpacity(d->sourceWidget->opacity());
    setGeometry(d->sourceWidget->geometry());
    show();
    raise();
    d->sourceWidget->hide();

    d->geometryAnimation.setDirection(QVariantAnimation::Forward);
    d->geometryAnimation.start();
    d->iconPositionAnimation.setDirection(QVariantAnimation::Forward);
    d->iconPositionAnimation.start();
    d->opacityAnimation.setDirection(QVariantAnimation::Forward);
    d->opacityAnimation.start();
}

void FloatingToolbarAnimator::switchToolbarsBack()
{
    if (d->sourceWidget == nullptr || d->targetWidget == nullptr) {
        return;
    }

    d->sourceWidget->show();
    d->sourceWidget->setOpacity(d->targetWidget->opacity());
    d->sourceWidgetImage = d->sourceWidget->grab();
    d->sourceWidget->hide();
    d->targetWidgetImage = d->targetWidget->grab();

    d->geometryAnimation.setStartValue(d->sourceWidget->geometry());
    d->geometryAnimation.setEndValue(d->targetWidget->geometry());

    setOpacity(d->targetWidget->opacity());
    setGeometry(d->targetWidget->geometry());
    show();
    raise();
    d->targetWidget->hide();

    d->geometryAnimation.setDirection(QVariantAnimation::Backward);
    d->geometryAnimation.start();
    d->iconPositionAnimation.setDirection(QVariantAnimation::Backward);
    d->iconPositionAnimation.start();
    d->opacityAnimation.setDirection(QVariantAnimation::Backward);
    d->opacityAnimation.start();
}

void FloatingToolbarAnimator::paintEventPostprocess(QPainter& _painter)
{
    const auto leftMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().left()
        + Ui::DesignSystem::floatingToolBar().margins().left();
    const auto rightMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().right()
        + Ui::DesignSystem::floatingToolBar().margins().right();
    const auto topMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().top();
    const auto bottomMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().bottom();

    const auto iconRect = QRectF(d->iconPositionAnimation.currentValue().toPointF(),
                                 Ui::DesignSystem::floatingToolBar().iconSize());
    _painter.drawPixmap(iconRect.left(), topMargin, d->targetWidgetImage, leftMargin, topMargin,
                        width() - iconRect.left() - rightMargin,
                        height() - topMargin - bottomMargin);

    _painter.setOpacity(d->opacityAnimation.currentValue().toReal());
    _painter.drawPixmap(leftMargin, topMargin, d->sourceWidgetImage, leftMargin, topMargin,
                        d->sourceWidgetImage.width() - leftMargin - rightMargin,
                        d->sourceWidgetImage.height() - topMargin - bottomMargin);

    _painter.setOpacity(d->opacityAnimation.currentValue().toReal());
    _painter.fillRect(iconRect, backgroundColor());
    _painter.drawText(iconRect, Qt::AlignCenter, d->targetIcon);
}
