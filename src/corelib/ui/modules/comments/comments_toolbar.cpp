#include "comments_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <utils/tools/run_once.h>

#include <QAction>
#include <QPaintEvent>
#include <QPainter>
#include <QSettings>
#include <QTimer>
#include <QVariantAnimation>


namespace Ui {

namespace {
const QString kColorKey = QLatin1String("widgets/screenplay-text-comments-toolbar/color");
}

class CommentsToolbar::Implementation
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


    QAction* textColorAction = nullptr;
    QAction* textBackgroundColorAction = nullptr;
    QAction* commentAction = nullptr;
    QAction* markAsDoneAction = nullptr;
    QAction* removeAction = nullptr;
    QAction* separatorAction = nullptr;
    QAction* colorAction = nullptr;

    QVariantAnimation opacityAnimation;
    QTimer hideTimer;
    QPixmap contentPixmap;
    QVariantAnimation moveAnimation;

    ColorPickerPopup* colorPickerPopup = nullptr;
};

CommentsToolbar::Implementation::Implementation(QWidget* _parent)
    : textColorAction(new QAction)
    , textBackgroundColorAction(new QAction)
    , commentAction(new QAction)
    , markAsDoneAction(new QAction)
    , removeAction(new QAction)
    , separatorAction(new QAction)
    , colorAction(new QAction)
    , colorPickerPopup(new ColorPickerPopup(_parent))
{
    opacityAnimation.setDuration(220);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    hideTimer.setSingleShot(true);
    hideTimer.setInterval(opacityAnimation.duration());
    moveAnimation.setDuration(420);
    moveAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void CommentsToolbar::Implementation::animateShow()
{
    hideTimer.stop();

    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.start();
}

void CommentsToolbar::Implementation::animateHide()
{
    opacityAnimation.setStartValue(1.0);
    opacityAnimation.setEndValue(0.0);
    opacityAnimation.start();

    hideTimer.start();
}

void CommentsToolbar::Implementation::animateMove(const QPoint& _from, const QPoint& _to)
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


// ****


CommentsToolbar::CommentsToolbar(QWidget* _parent)
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

    d->markAsDoneAction->setIconText(u8"\U000F012C");
    d->markAsDoneAction->setCheckable(true);
    addAction(d->markAsDoneAction);

    d->removeAction->setIconText(u8"\U000F01B4");
    addAction(d->removeAction);

    d->separatorAction->setSeparator(true);
    addAction(d->separatorAction);

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
    connect(d->markAsDoneAction, &QAction::toggled, this, &CommentsToolbar::markAsDoneRequested);
    connect(d->removeAction, &QAction::triggered, this, &CommentsToolbar::removeRequested);
    connect(d->colorAction, &QAction::triggered, this, [this] {
        if (d->colorPickerPopup->isPopupShown()) {
            d->colorPickerPopup->hidePopup();
        } else {
            d->colorPickerPopup->setSelectedColor(actionColor(d->colorAction));
            d->colorPickerPopup->showPopup(this);
        }
    });
    connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) {
                setActionColor(d->colorAction, _color);

                QSettings settings;
                settings.setValue(kColorKey, _color);
            });

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->hideTimer, &QTimer::timeout, this, &Widget::hide);
    connect(&d->moveAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { move(_value.toPoint()); });
}

CommentsToolbar::~CommentsToolbar() = default;

void CommentsToolbar::setMode(Mode _mode)
{
    const auto isAddNewCommentVisible = _mode == Mode::AddNewComment;
    d->textColorAction->setVisible(isAddNewCommentVisible);
    d->textBackgroundColorAction->setVisible(isAddNewCommentVisible);
    d->commentAction->setVisible(isAddNewCommentVisible);
    d->separatorAction->setVisible(isAddNewCommentVisible);
    d->colorAction->setVisible(isAddNewCommentVisible);
    const auto isEditCommentVisible = !isAddNewCommentVisible;
    d->markAsDoneAction->setVisible(isEditCommentVisible);
    d->removeAction->setVisible(isEditCommentVisible);

    resize(sizeHint());
}

void CommentsToolbar::setCurrentCommentIsDone(bool _isDone)
{
    QSignalBlocker signalBlocker(d->markAsDoneAction);
    d->markAsDoneAction->setChecked(_isDone);
}

void CommentsToolbar::showToolbar()
{
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

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

void CommentsToolbar::hideToolbar()
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

void CommentsToolbar::moveToolbar(const QPoint& _position)
{
    if (isHidden()) {
        move(_position);
        return;
    }

    d->animateMove(pos(), _position);
}

void CommentsToolbar::paintEvent(QPaintEvent* _event)
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

void CommentsToolbar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    if (!d->colorPickerPopup->hasFocus()) {
        d->colorPickerPopup->hidePopup();
    }
}

void CommentsToolbar::updateTranslations()
{
    d->textColorAction->setToolTip(tr("Change text color"));
    d->textBackgroundColorAction->setToolTip(tr("Change text highlight color"));
    d->commentAction->setToolTip(tr("Add comment"));
    //: This allow user to choose color for the review mode actions like text higlight or comments
    d->colorAction->setToolTip(tr("Choose color for the action"));
}

void CommentsToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    resize(sizeHint());

    d->colorPickerPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->colorPickerPopup->setTextColor(Ui::DesignSystem::color().onBackground());
}

} // namespace Ui
