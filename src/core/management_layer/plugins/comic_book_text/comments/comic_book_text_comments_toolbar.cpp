#include "comic_book_text_comments_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/color_picker/color_picker.h>

#include <QAction>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QSettings>
#include <QTimer>
#include <QVariantAnimation>


namespace Ui {

namespace {
const QString kColorKey = QLatin1String("widgets/screenplay-text-comments-toolbar/color");
}

class ComicBookTextCommentsToolbar::Implementation
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
    QAction* textBackgroundColorAction = nullptr;
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

ComicBookTextCommentsToolbar::Implementation::Implementation(QWidget* _parent)
    : textColorAction(new QAction)
    , textBackgroundColorAction(new QAction)
    , commentAction(new QAction)
    , colorAction(new QAction)
    , popup(new Card(_parent))
    , popupContent(new ColorPicker(_parent))
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

void ComicBookTextCommentsToolbar::Implementation::animateShow()
{
    hideTimer.stop();

    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.start();
}

void ComicBookTextCommentsToolbar::Implementation::animateHide()
{
    opacityAnimation.setStartValue(1.0);
    opacityAnimation.setEndValue(0.0);
    opacityAnimation.start();

    hideTimer.start();
}

void ComicBookTextCommentsToolbar::Implementation::animateMove(const QPoint& _from,
                                                               const QPoint& _to)
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

void ComicBookTextCommentsToolbar::Implementation::showPopup(QWidget* _parent)
{
    isPopupShown = true;

    popup->resize(static_cast<int>(popup->sizeHint().width()), 0);

    const auto left
        = QPoint(_parent->rect().center().x() - popup->width() / 2,
                 _parent->rect().bottom() - Ui::DesignSystem::textField().margins().bottom());
    const auto pos = _parent->mapToGlobal(left);
    popup->move(pos);
    popup->show();
    popup->setFocus();

    popupHeightAnimation.setDirection(QVariantAnimation::Forward);
    popupHeightAnimation.setEndValue(popup->sizeHint().height());
    popupHeightAnimation.start();
}

void ComicBookTextCommentsToolbar::Implementation::hidePopup()
{
    isPopupShown = false;

    popupHeightAnimation.setDirection(QVariantAnimation::Backward);
    popupHeightAnimation.start();
}


// ****


ComicBookTextCommentsToolbar::ComicBookTextCommentsToolbar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    setOrientation(Qt::Vertical);

    d->textColorAction->setIconText(u8"\U000f069e");
    addAction(d->textColorAction);

    d->textBackgroundColorAction->setIconText(u8"\U000f0266");
    addAction(d->textBackgroundColorAction);

    d->commentAction->setIconText(u8"\U000f0188");
    addAction(d->commentAction);

    d->colorAction->setIconText(u8"\U000f0765");
    addAction(d->colorAction);
    if (QSettings settings; settings.value(kColorKey).isValid()) {
        setActionColor(d->colorAction, settings.value(kColorKey).value<QColor>());
    } else {
        setActionColor(d->colorAction, "#FE0000");
    }


    connect(d->textColorAction, &QAction::triggered, this,
            [this] { emit textColorChangeRequested(actionColor(d->colorAction)); });
    connect(d->textBackgroundColorAction, &QAction::triggered, this,
            [this] { emit textBackgoundColorChangeRequested(actionColor(d->colorAction)); });
    connect(d->commentAction, &QAction::triggered, this,
            [this] { emit commentAddRequested(actionColor(d->colorAction)); });
    connect(d->colorAction, &QAction::triggered, this, [this] {
        if (d->isPopupShown) {
            d->hidePopup();
        } else {
            d->showPopup(this);
        }
    });
    connect(d->popupContent, &ColorPicker::colorSelected, this, [this](const QColor& _color) {
        setActionColor(d->colorAction, _color);
        d->hidePopup();

        QSettings settings;
        settings.setValue(kColorKey, _color);
    });

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->hideTimer, &QTimer::timeout, this, &Widget::hide);
    connect(&d->moveAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { move(_value.toPoint()); });

    connect(&d->popupHeightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
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

ComicBookTextCommentsToolbar::~ComicBookTextCommentsToolbar() = default;

void ComicBookTextCommentsToolbar::showToolbar()
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

void ComicBookTextCommentsToolbar::hideToolbar()
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

void ComicBookTextCommentsToolbar::moveToolbar(const QPoint& _position)
{
    if (isHidden()) {
        move(_position);
        return;
    }

    d->animateMove(pos(), _position);
}

void ComicBookTextCommentsToolbar::paintEvent(QPaintEvent* _event)
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

void ComicBookTextCommentsToolbar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    if (!d->popup->hasFocus()) {
        d->hidePopup();
    }
}

void ComicBookTextCommentsToolbar::updateTranslations()
{
    d->textColorAction->setToolTip(tr("Change text color"));
    d->textBackgroundColorAction->setToolTip(tr("Change text highlight color"));
    d->commentAction->setToolTip(tr("Add comment"));
    //: This allow user to choose color for the review mode actions like text higlight or comments
    d->colorAction->setToolTip(tr("Choose color for the action"));
}

void ComicBookTextCommentsToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    resize(sizeHint());

    d->popup->setBackgroundColor(Ui::DesignSystem::color().background());
}

} // namespace Ui
