#include "shadow.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QPainter>


Shadow::Shadow(QWidget* _parent)
    : Widget(_parent)
{
}

void Shadow::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    const qreal sideMargin = width() / 3.0;
    const QRectF backgroundRect = QRectF(rect()).marginsRemoved({sideMargin, 0.0, sideMargin, 0.0});
    QPixmap backgroundImage(backgroundRect.size().toSize());
    backgroundImage.fill(backgroundColor());
    const QPixmap shadow
            = ImageHelper::dropShadow(backgroundImage,
                                      {sideMargin, 0, sideMargin, 0},
                                      Ui::DesignSystem::button().minimumShadowBlurRadius() + sideMargin,
                                      Ui::DesignSystem::color().shadow());

    QPainter painter(this);
    painter.drawPixmap(0, 0, shadow);
}
