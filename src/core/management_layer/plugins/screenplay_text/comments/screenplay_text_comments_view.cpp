#include "screenplay_text_comments_view.h"

#include "screenplay_text_add_comment_view.h"
#include "screenplay_text_comment_delegate.h"
#include "screenplay_text_comment_replies_view.h"
#include "screenplay_text_comments_model.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/tree/tree.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QAction>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

class ScreenplayTextCommentsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Действия контекстного меню
     */
    enum class ContextMenuAction { MarkAsDone, MarkAsUndone, Remove };

    /**
     * @brief Обновить контекстное меню для заданного списка элементов
     */
    void updateCommentsViewContextMenu(const QModelIndexList& _indexes,
                                       ScreenplayTextCommentsView* _view);


    Tree* commentsView = nullptr;
    ContextMenu* commentsViewContextMenu = nullptr;

    ScreenplayTextAddCommentView* addCommentView = nullptr;
    QColor addCommentColor;

    ScreenplayTextCommentRepliesView* repliesView = nullptr;
};

ScreenplayTextCommentsView::Implementation::Implementation(QWidget* _parent)
    : commentsView(new Tree(_parent))
    , commentsViewContextMenu(new ContextMenu(commentsView))
    , addCommentView(new ScreenplayTextAddCommentView(_parent))
    , repliesView(new ScreenplayTextCommentRepliesView(_parent))
{
    commentsView->setAutoAdjustSize(true);
    commentsView->setContextMenuPolicy(Qt::CustomContextMenu);
    commentsView->setItemDelegate(new ScreenplayTextCommentDelegate(commentsView));
    commentsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void ScreenplayTextCommentsView::Implementation::updateCommentsViewContextMenu(
    const QModelIndexList& _indexes, ScreenplayTextCommentsView* _view)
{
    if (_indexes.isEmpty()) {
        return;
    }

    QVector<QAction*> menuActions;

    //
    // Настраиваем контекстное меню для одного элемента
    //
    if (_indexes.size() == 1) {
        auto discuss = new QAction(tr("Discuss"));
        discuss->setIconText(u8"\U000F0860");
        connect(discuss, &QAction::triggered, _view, [this, _view] {
            _view->showCommentRepliesView(commentsView->selectedIndexes().constFirst());
        });
        menuActions.append(discuss);
        if (_indexes.constFirst()
                .data(BusinessLayer::ScreenplayTextCommentsModel::ReviewMarkIsDoneRole)
                .toBool()) {
            auto markAsUndone = new QAction(tr("Mark as undone"));
            markAsUndone->setIconText(u8"\U000F0131");
            connect(markAsUndone, &QAction::triggered, _view, [this, _view] {
                emit _view->markAsUndoneRequested(commentsView->selectedIndexes());
            });
            menuActions.append(markAsUndone);
        } else {
            auto markAsDone = new QAction(tr("Mark as done"));
            markAsDone->setIconText(u8"\U000F0135");
            connect(markAsDone, &QAction::triggered, _view, [this, _view] {
                emit _view->markAsDoneRequested(commentsView->selectedIndexes());
            });
            menuActions.append(markAsDone);
        }
        auto remove = new QAction(tr("Remove"));
        remove->setIconText(u8"\U000F01B4");
        connect(remove, &QAction::triggered, _view,
                [this, _view] { emit _view->removeRequested(commentsView->selectedIndexes()); });
        menuActions.append(remove);
    }
    //
    // Настраиваем контекстное меню для нескольких выделенных элементов
    //
    else {
        auto markAsDone = new QAction(tr("Mark selected notes as done"));
        markAsDone->setIconText(u8"\U000F0139");
        connect(markAsDone, &QAction::triggered, _view, [this, _view] {
            emit _view->markAsDoneRequested(commentsView->selectedIndexes());
        });
        menuActions.append(markAsDone);
        //
        auto markAsUndone = new QAction(tr("Mark selected notes as undone"));
        markAsUndone->setIconText(u8"\U000F0137");
        connect(markAsUndone, &QAction::triggered, _view, [this, _view] {
            emit _view->markAsUndoneRequested(commentsView->selectedIndexes());
        });
        menuActions.append(markAsUndone);
        //
        auto remove = new QAction(tr("Remove selected notes"));
        remove->setIconText(u8"\U000F01B4");
        connect(remove, &QAction::triggered, _view,
                [this, _view] { emit _view->removeRequested(commentsView->selectedIndexes()); });
        menuActions.append(remove);
    }

    commentsViewContextMenu->setActions(menuActions);
}


// ****


ScreenplayTextCommentsView::ScreenplayTextCommentsView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->commentsView);
    addWidget(d->addCommentView);
    addWidget(d->repliesView);


    connect(d->commentsView, &Tree::clicked, this, &ScreenplayTextCommentsView::commentSelected);
    connect(d->commentsView, &Tree::doubleClicked, this,
            &ScreenplayTextCommentsView::showCommentRepliesView);
    connect(d->commentsView, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        if (d->commentsView->selectedIndexes().isEmpty()) {
            return;
        }

        d->updateCommentsViewContextMenu(d->commentsView->selectedIndexes(), this);
        d->commentsViewContextMenu->showContextMenu(d->commentsView->mapToGlobal(_pos));
    });
    connect(d->addCommentView, &ScreenplayTextAddCommentView::savePressed, this, [this] {
        emit addReviewMarkRequested(d->addCommentColor, d->addCommentView->comment());
        setCurrentWidget(d->commentsView);
    });
    connect(d->addCommentView, &ScreenplayTextAddCommentView::cancelPressed, this,
            [this] { setCurrentWidget(d->commentsView); });
    connect(d->repliesView, &ScreenplayTextCommentRepliesView::addReplyPressed, this,
            [this](const QString& _reply) {
                emit addReviewMarkCommentRequested(d->commentsView->currentIndex(), _reply);
            });
    connect(d->repliesView, &ScreenplayTextCommentRepliesView::closePressed, this, [this] {
        auto animationRect = d->commentsView->visualRect(d->commentsView->currentIndex());
        animationRect.setLeft(0);
        setAnimationRect(d->commentsView, animationRect);
        setCurrentWidget(d->commentsView);
        QTimer::singleShot(animationDuration(),
                           [this] { setAnimationType(StackWidget::AnimationType::Slide); });
    });


    designSystemChangeEvent(nullptr);
}

ScreenplayTextCommentsView::~ScreenplayTextCommentsView() = default;

void ScreenplayTextCommentsView::setModel(QAbstractItemModel* _model)
{
    if (d->commentsView->model() != nullptr) {
        disconnect(d->commentsView->model());
    }

    d->commentsView->setModel(_model);

    if (_model != nullptr) {
        connect(_model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& _index) {
            if (_index == d->repliesView->commentIndex()) {
                d->repliesView->setCommentIndex(_index);
            }
        });
    }
}

void ScreenplayTextCommentsView::setCurrentIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);
    d->commentsView->setCurrentIndex(_index);
}

void ScreenplayTextCommentsView::showAddCommentView(const QColor& _withColor)
{
    d->addCommentColor = _withColor;
    d->addCommentView->setComment({});
    setCurrentWidget(d->addCommentView);
    QTimer::singleShot(animationDuration(), d->addCommentView,
                       qOverload<>(&ScreenplayTextAddCommentView::setFocus));
}

void ScreenplayTextCommentsView::showCommentRepliesView(const QModelIndex& _commentIndex)
{
    d->repliesView->setCommentIndex(_commentIndex);

    //
    // Начинаем переход после того, как закончится анимация выбора элемента
    //
    QTimer::singleShot(100, [this, _commentIndex] {
        setAnimationType(StackWidget::AnimationType::Expand);
        auto animationRect = d->commentsView->visualRect(_commentIndex);
        animationRect.setLeft(0);
        setAnimationRect(d->commentsView, animationRect);
        setCurrentWidget(d->repliesView);
        QTimer::singleShot(animationDuration(), d->repliesView,
                           qOverload<>(&ScreenplayTextAddCommentView::setFocus));
    });
}

void ScreenplayTextCommentsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->commentsView->setBackgroundColor(DesignSystem::color().primary());
    d->commentsView->setTextColor(DesignSystem::color().onPrimary());
    d->commentsViewContextMenu->setBackgroundColor(DesignSystem::color().background());
    d->commentsViewContextMenu->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui
