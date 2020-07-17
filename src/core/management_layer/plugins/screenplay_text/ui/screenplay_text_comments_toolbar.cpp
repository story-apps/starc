#include "screenplay_text_comments_toolbar.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/card/card.h>
#include <ui/widgets/color_picker/color_picker.h>

#include <QAction>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVariantAnimation>


namespace Ui
{

class ScreenplayTextCommentsToolbar::Implementation
{
public:
    Implementation(QWidget* _parent);

    /**
     * @brief Анимировать отображение
     */
    void animateShow();

    /**
     * @brief Анимировать скрытие
     */
    void animateHide();

    /**
     * @brief Анимировать смещение
     */
    void animateMove(const QPoint& _from, const QPoint& _to);

    /**
     * @brief Показать попап
     */
    void showPopup(QWidget* _parent);

    /**
     * @brief Скрыть попап
     */
    void hidePopup();


    QAction* textColorAction = nullptr;
    QAction* backgroundColorAction = nullptr;
    QAction* commentAction = nullptr;
    QAction* colorAction = nullptr;

    QVariantAnimation opacityAnimation;
    QTimer hideTimer;
    QPixmap contentPixmap;
    QVariantAnimation moveAnimation;

    bool isPopupShown = false;
    Card* popup = nullptr;
    ColorPicker* popupContent = nullptr;
    QVariantAnimation popupHeightAnimation;
};

ScreenplayTextCommentsToolbar::Implementation::Implementation(QWidget* _parent)
    : textColorAction(new QAction),
      backgroundColorAction(new QAction),
      commentAction(new QAction),
      colorAction(new QAction),
      popup(new Card(_parent)),
      popupContent(new ColorPicker(_parent))
{
    opacityAnimation.setDuration(220);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    hideTimer.setSingleShot(true);
    hideTimer.setInterval(opacityAnimation.duration());
    moveAnimation.setDuration(420);
    moveAnimation.setEasingCurve(QEasingCurve::OutQuad);

    popup->setFocusPolicy(Qt::StrongFocus);
    popup->setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    popup->setAttribute(Qt::WA_Hover, false);
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);
    popup->hide();

    QHBoxLayout* popupLayout = new QHBoxLayout;
    popupLayout->setMargin({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(popupContent);
    popup->setLayoutReimpl(popupLayout);

    popupHeightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    popupHeightAnimation.setDuration(240);
    popupHeightAnimation.setStartValue(0);
    popupHeightAnimation.setEndValue(0);
}

void ScreenplayTextCommentsToolbar::Implementation::animateShow()
{
    hideTimer.stop();

    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.start();
}

void ScreenplayTextCommentsToolbar::Implementation::animateHide()
{
    opacityAnimation.setStartValue(1.0);
    opacityAnimation.setEndValue(0.0);
    opacityAnimation.start();

    hideTimer.start();
}

void ScreenplayTextCommentsToolbar::Implementation::animateMove(const QPoint& _from, const QPoint& _to)
{
    if (moveAnimation.state() == QVariantAnimation::Running) {
        if (moveAnimation.endValue().toPoint() == _to) {
            return;
        } else {
            moveAnimation.stop();
        }
    }

    moveAnimation.setStartValue(_from);
    moveAnimation.setEndValue(_to);
    moveAnimation.start();
}

void ScreenplayTextCommentsToolbar::Implementation::showPopup(QWidget* _parent)
{
    isPopupShown = true;

    popup->resize(static_cast<int>(popup->sizeHint().width()), 0);

    const auto left = QPoint(_parent->rect().center().x() - popup->width() / 2,
                             _parent->rect().bottom()
                             - Ui::DesignSystem::textField().margins().bottom());
    const auto pos = _parent->mapToGlobal(left);
    popup->move(pos);
    popup->show();

    popupHeightAnimation.setDirection(QVariantAnimation::Forward);
    popupHeightAnimation.setEndValue(popup->sizeHint().height());
    popupHeightAnimation.start();
}

void ScreenplayTextCommentsToolbar::Implementation::hidePopup()
{
    isPopupShown = false;

    popupHeightAnimation.setDirection(QVariantAnimation::Backward);
    popupHeightAnimation.start();
}


// ****


ScreenplayTextCommentsToolbar::ScreenplayTextCommentsToolbar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation(this))
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

    connect(d->colorAction, &QAction::triggered, this, [this] {
        if (d->isPopupShown) {
            d->hidePopup();
        } else {
            d->showPopup(this);
        }
    });
    connect(d->popupContent, &ColorPicker::colorSelected, this, [this] (const QColor& _color) {
        setActionColor(d->colorAction, _color);
        d->hidePopup();
    });

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->hideTimer, &QTimer::timeout, this, &Widget::hide);
    connect(&d->moveAnimation, &QVariantAnimation::valueChanged, this,
            [this] (const QVariant& _value) { move(_value.toPoint()); });

    connect(&d->popupHeightAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        const auto height = _value.toInt();
        d->popup->resize(d->popup->width(), height);
    });
    connect(&d->popupHeightAnimation, &QVariantAnimation::finished, this, [this] {
        if (!d->isPopupShown) {
            d->popup->hide();
        }
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextCommentsToolbar::~ScreenplayTextCommentsToolbar() = default;

void ScreenplayTextCommentsToolbar::showToolbar()
{
    if (parentWidget() == nullptr) {
        return;
    }

    if (isVisible() && d->opacityAnimation.endValue().toReal() > 0.0) {
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

    if (d->opacityAnimation.endValue().toReal() < 1.0) {
        return;
    }

    d->contentPixmap = grab();
    d->animateHide();
}

void ScreenplayTextCommentsToolbar::moveToolbar(const QPoint& _position)
{
    if (isHidden()) {
        move(_position);
        return;
    }

    d->animateMove(pos(), _position);
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

void ScreenplayTextCommentsToolbar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    if (!d->popup->hasFocus()) {
        d->hidePopup();
    }
}

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

    d->popup->setBackgroundColor(Ui::DesignSystem::color().background());
}

} // namespace Ui
