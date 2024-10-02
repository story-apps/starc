#include "comments_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <utils/helpers/color_helper.h>
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
const QString kCommentsTypeKey = QLatin1String("widgets/screenplay-text-comments-toolbar/type");
} // namespace

class CommentsToolbar::Implementation
{
public:
    Implementation(CommentsToolbar* _q);

    /**
     * @brief Обновить список доступных действий в зависмотси от типа рецензирования и режима
     */
    void updateActions();

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


    CommentsToolbar* q = nullptr;

    CommentsType type = CommentsType::Review;
    Mode mode = Mode::AddReview;

    QAction* typeAction = nullptr;
    FloatingToolBar* typeToolbar = nullptr;
    QAction* reviewType = nullptr;
    QAction* changesType = nullptr;
    QAction* revisionType = nullptr;

    QAction* reviewTextColorAction = nullptr;
    QAction* reviewTextBackgroundColorAction = nullptr;
    QAction* reviewCommentAction = nullptr;
    QAction* changesTextBackgroundColorAction = nullptr;
    QAction* revisionMarkAction = nullptr;
    QAction* colorSeparatorAction = nullptr;
    QAction* colorAction = nullptr;
    QAction* markAsDoneSeparatorAction = nullptr;
    QAction* markAsDoneAction = nullptr;
    QAction* removeAction = nullptr;

    QVariantAnimation opacityAnimation;
    QTimer hideTimer;
    QPixmap contentPixmap;
    QVariantAnimation moveAnimation;

    ColorPickerPopup* colorPickerPopup = nullptr;
};

CommentsToolbar::Implementation::Implementation(CommentsToolbar* _q)
    : q(_q)
    , typeAction(new QAction)
    , typeToolbar(new FloatingToolBar(_q->parentWidget()))
    , reviewType(new QAction)
    , changesType(new QAction)
    , revisionType(new QAction)
    //
    , reviewTextColorAction(new QAction)
    , reviewTextBackgroundColorAction(new QAction)
    , reviewCommentAction(new QAction)
    , changesTextBackgroundColorAction(new QAction)
    , revisionMarkAction(new QAction)
    , colorSeparatorAction(new QAction)
    , colorAction(new QAction)
    , markAsDoneSeparatorAction(new QAction)
    , markAsDoneAction(new QAction)
    , removeAction(new QAction)
    , colorPickerPopup(new ColorPickerPopup(_q))
{
    reviewType->setIconText(u8"\U000F0E31");
    typeToolbar->addAction(reviewType);
    changesType->setIconText(u8"\U000F0900");
    typeToolbar->addAction(changesType);
    revisionType->setIconText(u8"\U000F0DF2");
    typeToolbar->addAction(revisionType);
    typeToolbar->hide();

    opacityAnimation.setDuration(220);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    hideTimer.setSingleShot(true);
    hideTimer.setInterval(opacityAnimation.duration());
    moveAnimation.setDuration(420);
    moveAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void CommentsToolbar::Implementation::updateActions()
{
    const auto isEditCommentVisible = mode == Mode::EditReview;

    switch (type) {
    case CommentsType::Review: {
        reviewTextColorAction->setVisible(true);
        reviewTextBackgroundColorAction->setVisible(true);
        reviewCommentAction->setVisible(true);
        changesTextBackgroundColorAction->setVisible(false);
        revisionMarkAction->setVisible(false);
        markAsDoneAction->setVisible(isEditCommentVisible);
        removeAction->setVisible(isEditCommentVisible);
        break;
    }

    case CommentsType::Changes: {
        reviewTextColorAction->setVisible(false);
        reviewTextBackgroundColorAction->setVisible(false);
        reviewCommentAction->setVisible(false);
        changesTextBackgroundColorAction->setVisible(true);
        revisionMarkAction->setVisible(false);
        markAsDoneAction->setVisible(isEditCommentVisible);
        removeAction->setVisible(isEditCommentVisible);
        break;
    }

    case CommentsType::Revision: {
        reviewTextColorAction->setVisible(false);
        reviewTextBackgroundColorAction->setVisible(false);
        reviewCommentAction->setVisible(false);
        changesTextBackgroundColorAction->setVisible(false);
        revisionMarkAction->setVisible(true);
        markAsDoneAction->setVisible(false);
        removeAction->setVisible(isEditCommentVisible);
        break;
    }
    }

    markAsDoneSeparatorAction->setVisible(isEditCommentVisible);

    if (type == CommentsType::Revision) {
        QVector<QColor> palette;
        for (int level = 0; level < 9; ++level) {
            palette.append(ColorHelper::revisionColor(level));
        }
        colorPickerPopup->setCustomPalette(palette);
    } else {
        colorPickerPopup->setCustomPalette({});
    }

    q->resize(q->sizeHint());
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

    //
    // Сначала сразу сдвигаем по горизонтали
    //
    const QPoint fromCorrected(_to.x(), _from.y());
    q->move(fromCorrected);

    //
    // А потом анимируем смещение по вертикали
    //
    moveAnimation.setStartValue(fromCorrected);
    moveAnimation.setEndValue(_to);
    moveAnimation.start();
}


// ****


CommentsToolbar::CommentsToolbar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    setOrientation(Qt::Vertical);
    setCurtain(true, Qt::RightEdge);

    d->typeToolbar->installEventFilter(this);

    if (const auto value = QSettings().value(kCommentsTypeKey); value.isValid()) {
        d->type = static_cast<CommentsType>(value.toInt());
        switch (d->type) {
        case CommentsType::Review: {
            d->typeAction->setIconText(d->reviewType->iconText());
            break;
        }
        case CommentsType::Changes: {
            d->typeAction->setIconText(d->changesType->iconText());
            break;
        }
        case CommentsType::Revision: {
            d->typeAction->setIconText(d->revisionType->iconText());
            break;
        }
        }
        d->updateActions();
    } else {
        d->typeAction->setIconText(u8"\U000F0E31");
    }
    addAction(d->typeAction);
    d->updateActions();

    d->reviewTextColorAction->setIconText(u8"\U000f069e");
    addAction(d->reviewTextColorAction);

    d->reviewTextBackgroundColorAction->setIconText(u8"\U000f0266");
    addAction(d->reviewTextBackgroundColorAction);

    d->reviewCommentAction->setIconText(u8"\U000f0188");
    addAction(d->reviewCommentAction);

    d->changesTextBackgroundColorAction->setIconText(u8"\U000f0266");
    addAction(d->changesTextBackgroundColorAction);

    d->revisionMarkAction->setIconText(u8"\U000F0382");
    addAction(d->revisionMarkAction);

    d->markAsDoneSeparatorAction->setSeparator(true);
    addAction(d->markAsDoneSeparatorAction);

    d->markAsDoneAction->setIconText(u8"\U000F012C");
    d->markAsDoneAction->setCheckable(true);
    addAction(d->markAsDoneAction);

    d->removeAction->setIconText(u8"\U000F01B4");
    addAction(d->removeAction);

    d->colorSeparatorAction->setSeparator(true);
    addAction(d->colorSeparatorAction);

    d->colorAction->setIconText(u8"\U000f0765");
    addAction(d->colorAction);
    if (const auto value = QSettings().value(kColorKey); value.isValid()) {
        setActionColor(d->colorAction, value.value<QColor>());
    } else {
        setActionColor(d->colorAction, "#FE0000");
    }


    connect(d->typeAction, &QAction::triggered, this, [this] {
        d->typeToolbar->resize(d->typeToolbar->sizeHint());
        d->typeToolbar->raise();
        d->typeToolbar->move(pos() - QPoint(DesignSystem::layout().px48(), 0));
        d->typeToolbar->show();
        d->typeToolbar->setFocus();
    });
    auto setType = [this](CommentsType _type) {
        if (d->type == _type) {
            return;
        }

        d->type = _type;
        d->updateActions();

        QSettings().setValue(kCommentsTypeKey, static_cast<int>(d->type));

        emit commentsTypeChanged(d->type);
    };
    connect(d->reviewType, &QAction::triggered, this, [this, setType] {
        d->typeToolbar->hide();
        d->typeAction->setIconText(d->reviewType->iconText());
        setType(CommentsType::Review);
    });
    connect(d->changesType, &QAction::triggered, this, [this, setType] {
        d->typeToolbar->hide();
        d->typeAction->setIconText(d->changesType->iconText());
        setType(CommentsType::Changes);
    });
    connect(d->revisionType, &QAction::triggered, this, [this, setType] {
        d->typeToolbar->hide();
        d->typeAction->setIconText(d->revisionType->iconText());
        setType(CommentsType::Revision);
    });
    //
    connect(d->reviewTextColorAction, &QAction::triggered, this,
            [this] { emit textColorChangeRequested(color()); });
    connect(d->reviewTextBackgroundColorAction, &QAction::triggered, this,
            [this] { emit textBackgoundColorChangeRequested(color()); });
    connect(d->reviewCommentAction, &QAction::triggered, this,
            [this] { emit commentAddRequested(color()); });
    connect(d->changesTextBackgroundColorAction, &QAction::triggered, this,
            [this] { emit textBackgoundColorChangeRequested(color()); });
    connect(d->revisionMarkAction, &QAction::triggered, this,
            [this] { emit revisionMarkAddRequested(color()); });
    connect(d->markAsDoneAction, &QAction::toggled, this, &CommentsToolbar::markAsDoneRequested);
    connect(d->removeAction, &QAction::triggered, this, &CommentsToolbar::removeRequested);
    connect(d->colorAction, &QAction::triggered, this, [this] {
        if (d->colorPickerPopup->isPopupShown()) {
            d->colorPickerPopup->hidePopup();
        } else {
            d->colorPickerPopup->setSelectedColor(color());
            d->colorPickerPopup->showPopup(this);
        }
    });
    connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) {
                setActionColor(d->colorAction, _color);

                QSettings().setValue(kColorKey, _color);

                emit colorChanged(_color);
            });

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->hideTimer, &QTimer::timeout, this, &Widget::hide);
    connect(&d->moveAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { move(_value.toPoint()); });
}

CommentsToolbar::~CommentsToolbar() = default;

CommentsToolbar::CommentsType CommentsToolbar::commentsType() const
{
    return d->type;
}

QColor CommentsToolbar::color() const
{
    return actionColor(d->colorAction);
}

void CommentsToolbar::setMode(Mode _mode)
{
    if (d->mode == _mode) {
        return;
    }

    d->mode = _mode;
    d->updateActions();
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

void CommentsToolbar::moveToolbar(const QPoint& _position, bool _force)
{
    if (isHidden() || _force) {
        move(_position);
        return;
    }

    d->animateMove(pos(), _position);
}

bool CommentsToolbar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->typeToolbar && d->typeToolbar->isVisible()) {
        if (_event->type() == QEvent::FocusOut) {
            d->typeToolbar->hide();
        } else if (_event->type() == QEvent::KeyPress) {
            const auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Escape) {
                d->typeToolbar->hide();
            }
        }
    }

    return FloatingToolBar::eventFilter(_watched, _event);
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

void CommentsToolbar::moveEvent(QMoveEvent* _event)
{
    FloatingToolBar::moveEvent(_event);

    if (d->typeToolbar->isVisible()) {
        d->typeToolbar->move(pos() - QPoint(DesignSystem::layout().px48(), 0));
    }
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
    setCurtain(true, isLeftToRight() ? Qt::RightEdge : Qt::LeftEdge);
    d->typeAction->setToolTip(tr("Choose review mode"));
    d->reviewType->setToolTip(tr("Review mode"));
    d->changesType->setToolTip(tr("Track additions mode"));
    d->revisionType->setToolTip(tr("Revision mode"));
    d->reviewTextColorAction->setToolTip(tr("Change text color"));
    d->reviewTextBackgroundColorAction->setToolTip(tr("Change text highlight color"));
    d->reviewCommentAction->setToolTip(tr("Add comment"));
    d->changesTextBackgroundColorAction->setToolTip(tr("Change text highlight color"));
    d->revisionMarkAction->setToolTip(tr("Mark revisited"));
    //: This allow user to choose color for the review mode actions like text higlight or comments
    d->colorAction->setToolTip(tr("Choose color for the action"));
    d->markAsDoneAction->setToolTip(tr("Mark as done"));
    d->removeAction->setToolTip(tr("Remove selected mark"));
}

void CommentsToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    resize(sizeHint());

    setActionColor(d->typeAction, DesignSystem::color().accent());

    d->typeToolbar->setStartOpacity(1.0);
    d->typeToolbar->setBackgroundColor(DesignSystem::color().background());
    d->typeToolbar->setTextColor(DesignSystem::color().onBackground());

    d->colorPickerPopup->setBackgroundColor(DesignSystem::color().background());
    d->colorPickerPopup->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui
