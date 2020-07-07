#include "screenplay_text_comments_toolbar.h"

#include <QAction>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVariantAnimation>


namespace Ui
{

class ScreenplayTextCommentsToolbar::Implementation
{
public:
    Implementation();

    /**
     * @brief Анимировать отображение
     */
    void animateShow();

    /**
     * @brief Анимировать скрытие
     */
    void animateHide();


    QAction* textColorAction = nullptr;
    QAction* backgroundColorAction = nullptr;
    QAction* commentAction = nullptr;
    QAction* colorAction = nullptr;

    QVariantAnimation opacityAnimation;
    QPixmap contentPixmap;
};

ScreenplayTextCommentsToolbar::Implementation::Implementation()
    : textColorAction(new QAction),
      backgroundColorAction(new QAction),
      commentAction(new QAction),
      colorAction(new QAction)
{
    opacityAnimation.setDuration(220);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void ScreenplayTextCommentsToolbar::Implementation::animateShow()
{
    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.start();
}

void ScreenplayTextCommentsToolbar::Implementation::animateHide()
{
    opacityAnimation.setStartValue(1.0);
    opacityAnimation.setEndValue(0.0);
    opacityAnimation.start();
}


// ****


ScreenplayTextCommentsToolbar::ScreenplayTextCommentsToolbar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation)
{
    setOrientation(Qt::Vertical);

    d->textColorAction->setIconText(u8"\U000f069e");
    addAction(d->textColorAction);

    d->backgroundColorAction->setIconText(u8"\U000f0266");
    addAction(d->backgroundColorAction);

    d->commentAction->setIconText(u8"\U000f0188");
    addAction(d->commentAction);

    d->colorAction->setIconText(u8"\U000f0765");
    addAction(d->colorAction);

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void ScreenplayTextCommentsToolbar::showToolbar()
{
    if (parentWidget() == nullptr) {
        return;
    }

    if (qFuzzyCompare(d->opacityAnimation.endValue().toReal(), 1.0)) {
        return;
    }

    //
    // Сохраняем изображение контента и прячем сам виджет
    //
    d->contentPixmap = grab();

    //
    // Запускаем отображение
    //
    d->animateShow();
    show();
}

void ScreenplayTextCommentsToolbar::hideToolbar()
{
    if (isHidden()) {
        return;
    }

    if (qFuzzyCompare(d->opacityAnimation.endValue().toReal(), 0.0)) {
        return;
    }

    d->contentPixmap = grab();
    d->animateHide();
    QTimer::singleShot(d->opacityAnimation.duration(), this, &Widget::hide);
}

void ScreenplayTextCommentsToolbar::paintEvent(QPaintEvent* _event)
{
    //
    // Если надо, анимируем появление
    //
    if (d->opacityAnimation.state() == QVariantAnimation::Running) {
        QPainter painter(this);
        painter.setOpacity(d->opacityAnimation.currentValue().toReal());
        painter.drawPixmap(QPointF{}, d->contentPixmap);
        return;
    }

    FloatingToolBar::paintEvent(_event);
}

ScreenplayTextCommentsToolbar::~ScreenplayTextCommentsToolbar() = default;

void ScreenplayTextCommentsToolbar::updateTranslations()
{
    d->textColorAction->setToolTip(tr("Change text color"));
    d->backgroundColorAction->setToolTip(tr("Change text highlight color"));
    d->commentAction->setToolTip(tr("Add comment"));
    d->colorAction->setToolTip(tr("Choose color for the action below"));
}

void ScreenplayTextCommentsToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    resize(sizeHint());
}

} // namespace Ui
