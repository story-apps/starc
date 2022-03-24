#include "bookmarks_view.h"

#include "add_bookmark_view.h"
#include "bookmark_delegate.h"
#include "bookmarks_model.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/tree/tree.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QAction>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

class BookmarksView::Implementation
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
    void updateBookmarksViewContextMenu(const QModelIndexList& _indexes, BookmarksView* _view);


    Tree* commentsView = nullptr;
    ContextMenu* commentsViewContextMenu = nullptr;

    AddBookmarkView* addCommentView = nullptr;
    QModelIndex commentIndex;
    QColor commentColor;
};

BookmarksView::Implementation::Implementation(QWidget* _parent)
    : commentsView(new Tree(_parent))
    , commentsViewContextMenu(new ContextMenu(commentsView))
    , addCommentView(new AddBookmarkView(_parent))
{
    commentsView->setAutoAdjustSize(true);
    commentsView->setContextMenuPolicy(Qt::CustomContextMenu);
    commentsView->setItemDelegate(new BookmarkDelegate(commentsView));
    commentsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void BookmarksView::Implementation::updateBookmarksViewContextMenu(const QModelIndexList& _indexes,
                                                                   BookmarksView* _view)
{
    if (_indexes.isEmpty()) {
        return;
    }

    QVector<QAction*> menuActions;

    //
    // Настраиваем контекстное меню для одного элемента
    //
    //    if (_indexes.size() == 1) {
    //        auto edit = new QAction(tr("Edit"));
    //        edit->setIconText(u8"\U000F03EB");
    //        connect(edit, &QAction::triggered, _view, [this, _view] {
    //            commentIndex = commentsView->selectedIndexes().constFirst();
    //            _view->showAddBookmarkView(
    //                commentIndex.data(BusinessLayer::CommentsModel::ReviewMarkColorRole)
    //                    .value<QColor>(),
    //                commentIndex.data(BusinessLayer::CommentsModel::ReviewMarkCommentRole).toString());
    //        });
    //        menuActions.append(edit);
    //        auto discuss = new QAction(tr("Discuss"));
    //        discuss->setIconText(u8"\U000F0860");
    //        connect(discuss, &QAction::triggered, _view, [this, _view] {
    //            _view->showCommentRepliesView(commentsView->selectedIndexes().constFirst());
    //        });
    //        menuActions.append(discuss);
    //        if (_indexes.constFirst()
    //                .data(BusinessLayer::CommentsModel::ReviewMarkIsDoneRole)
    //                .toBool()) {
    //            auto markAsUndone = new QAction(tr("Mark as undone"));
    //            markAsUndone->setIconText(u8"\U000F0131");
    //            connect(markAsUndone, &QAction::triggered, _view, [this, _view] {
    //                emit _view->markAsUndoneRequested(commentsView->selectedIndexes());
    //            });
    //            menuActions.append(markAsUndone);
    //        } else {
    //            auto markAsDone = new QAction(tr("Mark as done"));
    //            markAsDone->setIconText(u8"\U000F0135");
    //            connect(markAsDone, &QAction::triggered, _view, [this, _view] {
    //                emit _view->markAsDoneRequested(commentsView->selectedIndexes());
    //            });
    //            menuActions.append(markAsDone);
    //        }
    //        auto remove = new QAction(tr("Remove"));
    //        remove->setIconText(u8"\U000F01B4");
    //        connect(remove, &QAction::triggered, _view,
    //                [this, _view] { emit _view->removeRequested(commentsView->selectedIndexes());
    //                });
    //        menuActions.append(remove);
    //    }
    //    //
    //    // Настраиваем контекстное меню для нескольких выделенных элементов
    //    //
    //    else {
    //        auto markAsDone = new QAction(tr("Mark selected notes as done"));
    //        markAsDone->setIconText(u8"\U000F0139");
    //        connect(markAsDone, &QAction::triggered, _view, [this, _view] {
    //            emit _view->markAsDoneRequested(commentsView->selectedIndexes());
    //        });
    //        menuActions.append(markAsDone);
    //        //
    //        auto markAsUndone = new QAction(tr("Mark selected notes as undone"));
    //        markAsUndone->setIconText(u8"\U000F0137");
    //        connect(markAsUndone, &QAction::triggered, _view, [this, _view] {
    //            emit _view->markAsUndoneRequested(commentsView->selectedIndexes());
    //        });
    //        menuActions.append(markAsUndone);
    //        //
    //        auto remove = new QAction(tr("Remove selected notes"));
    //        remove->setIconText(u8"\U000F01B4");
    //        connect(remove, &QAction::triggered, _view,
    //                [this, _view] { emit _view->removeRequested(commentsView->selectedIndexes());
    //                });
    //        menuActions.append(remove);
    //    }

    commentsViewContextMenu->setActions(menuActions);
}


// ****


BookmarksView::BookmarksView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->commentsView);
    addWidget(d->addCommentView);


    connect(d->commentsView, &Tree::clicked, this, &BookmarksView::bookmarkSelected);
    //    connect(d->commentsView, &Tree::doubleClicked, this,
    //    &BookmarksView::showCommentRepliesView);
    connect(d->commentsView, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        if (d->commentsView->selectedIndexes().isEmpty()) {
            return;
        }

        d->updateBookmarksViewContextMenu(d->commentsView->selectedIndexes(), this);
        d->commentsViewContextMenu->showContextMenu(d->commentsView->mapToGlobal(_pos));
    });
    connect(d->addCommentView, &AddBookmarkView::savePressed, this, [this] {
        if (d->commentIndex.isValid()) {
            emit changeReviewMarkRequested(d->commentIndex, d->addCommentView->text());
            d->commentIndex = {};
        } else {
            emit addReviewMarkRequested(d->commentColor, d->addCommentView->text());
        }
        setCurrentWidget(d->commentsView);
    });
    connect(d->addCommentView, &AddBookmarkView::cancelPressed, this, [this] {
        d->commentIndex = {};
        setCurrentWidget(d->commentsView);
    });


    designSystemChangeEvent(nullptr);
}

BookmarksView::~BookmarksView() = default;

void BookmarksView::setModel(QAbstractItemModel* _model)
{
    d->commentsView->setModel(_model);
}

void BookmarksView::setCurrentIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);
    d->commentsView->setCurrentIndex(_index);
}

void BookmarksView::showAddBookmarkView(const QColor& _withColor, const QString& _withText)
{
    d->commentColor = _withColor;
    d->addCommentView->setText(_withText);
    setCurrentWidget(d->addCommentView);
    QTimer::singleShot(animationDuration(), d->addCommentView,
                       qOverload<>(&AddBookmarkView::setFocus));
}

void BookmarksView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->commentsView->setBackgroundColor(DesignSystem::color().primary());
    d->commentsView->setTextColor(DesignSystem::color().onPrimary());
    d->commentsViewContextMenu->setBackgroundColor(DesignSystem::color().background());
    d->commentsViewContextMenu->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui
