#include "tree_view.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVariantAnimation>


class TreeView::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();


    /**
     * @brief  Декорации кнопки при клике
     */
    QPointF decorationCenterPosition;
    QRectF decorationRect;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

TreeView::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationRadiusAnimation.setStartValue(1.0);
    decorationRadiusAnimation.setDuration(240);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(420);
}

void TreeView::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


TreeView::TreeView(QWidget* _parent)
    : QTreeView(_parent),
      d(new Implementation)
{
    viewport()->installEventFilter(this);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); viewport()->update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); viewport()->update(); });
}

TreeView::~TreeView() = default;

bool TreeView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == viewport() && _event->type() == QEvent::MouseButtonPress) {
        auto event = static_cast<QMouseEvent*>(_event);
        d->decorationCenterPosition = event->pos();
        d->decorationRect = visualRect(indexAt(event->pos()));
        d->decorationRect.setLeft(0.0);
        d->decorationRadiusAnimation.setEndValue(d->decorationRect.width());
        d->animateClick();
    }

    return QTreeView::eventFilter(_watched, _event);
}

void TreeView::paintEvent(QPaintEvent* _event)
{
    QTreeView::paintEvent(_event);

    //
    // Если необходимо, рисуем декорацию
    //
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setClipRect(d->decorationRect);
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().highlightedText());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(d->decorationCenterPosition, d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
        painter.setClipRect(QRect(), Qt::NoClip);
    }
}
