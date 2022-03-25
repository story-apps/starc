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

    AddBookmarkView* addBookmarkView = nullptr;
    QModelIndex itemWithBookmarkIndex;
};

BookmarksView::Implementation::Implementation(QWidget* _parent)
    : commentsView(new Tree(_parent))
    , commentsViewContextMenu(new ContextMenu(commentsView))
    , addBookmarkView(new AddBookmarkView(_parent))
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
    if (_indexes.size() == 1) {
        auto edit = new QAction(tr("Edit"));
        edit->setIconText(u8"\U000F03EB");
        connect(edit, &QAction::triggered, _view, [this, _view] {
            itemWithBookmarkIndex = commentsView->selectedIndexes().constFirst();
            _view->showAddBookmarkView(itemWithBookmarkIndex);
        });
        menuActions.append(edit);
        //
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
        auto remove = new QAction(tr("Remove selected bookmarks"));
        remove->setIconText(u8"\U000F01B4");
        connect(remove, &QAction::triggered, _view,
                [this, _view] { emit _view->removeRequested(commentsView->selectedIndexes()); });
        menuActions.append(remove);
    }

    commentsViewContextMenu->setActions(menuActions);
}


// ****


BookmarksView::BookmarksView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->commentsView);
    addWidget(d->addBookmarkView);


    connect(d->commentsView, &Tree::clicked, this, &BookmarksView::bookmarkSelected);
    connect(d->commentsView, &Tree::doubleClicked, this, &BookmarksView::showAddBookmarkView);
    connect(d->commentsView, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        if (d->commentsView->selectedIndexes().isEmpty()) {
            return;
        }

        d->updateBookmarksViewContextMenu(d->commentsView->selectedIndexes(), this);
        d->commentsViewContextMenu->showContextMenu(d->commentsView->mapToGlobal(_pos));
    });
    connect(d->addBookmarkView, &AddBookmarkView::savePressed, this, [this] {
        if (d->itemWithBookmarkIndex.isValid()) {
            emit changeBookmarkRequested(d->itemWithBookmarkIndex,
                                         d->addBookmarkView->bookmarkName(),
                                         d->addBookmarkView->bookmarkColor());
            d->itemWithBookmarkIndex = {};
        } else {
            emit addBookmarkRequested(d->addBookmarkView->bookmarkName(),
                                      d->addBookmarkView->bookmarkColor());
        }
        setCurrentWidget(d->commentsView);
    });
    connect(d->addBookmarkView, &AddBookmarkView::cancelPressed, this, [this] {
        d->itemWithBookmarkIndex = {};
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

void BookmarksView::showAddBookmarkView(const QModelIndex& _index)
{
    d->itemWithBookmarkIndex = _index;
    d->addBookmarkView->setBookmarkName(
        _index.data(BusinessLayer::BookmarksModel::BookmarkNameRole).toString());
    d->addBookmarkView->setBookmarkColor(
        _index.data(BusinessLayer::BookmarksModel::BookmarkColorRole).value<QColor>());
    setCurrentWidget(d->addBookmarkView);
    QTimer::singleShot(animationDuration(), d->addBookmarkView,
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
