#include "screenplay_text_comments_view.h"

#include "screenplay_text_add_comment_widget.h"
#include "screenplay_text_comment_delegate.h"
#include "screenplay_text_comments_model.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui
{

class ScreenplayTextCommentsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Действия контекстного меню
     */
    enum class ContextMenuAction {
        MarkAsDone,
        MarkAsUndone,
        Remove
    };

    /**
     * @brief Обновить контекстное меню для заданного списка элементов
     */
    void updateCommentsViewContextMenu(const QModelIndexList& _indexes);


    Tree* commentsView = nullptr;
    QStandardItemModel* commentsViewContextMenuModel = nullptr;
    ContextMenu* commentsViewContextMenu = nullptr;

    ScreenplayTextAddCommentWidget* addCommentWidget = nullptr;
    QColor addCommentColor;
};

ScreenplayTextCommentsView::Implementation::Implementation(QWidget* _parent)
    : commentsView(new Tree(_parent)),
      commentsViewContextMenuModel(new QStandardItemModel(commentsView)),
      commentsViewContextMenu(new ContextMenu(commentsView)),
      addCommentWidget(new ScreenplayTextAddCommentWidget(_parent))
{
    commentsView->setAutoAdjustSize(true);
    commentsView->setContextMenuPolicy(Qt::CustomContextMenu);
    commentsView->setItemDelegate(new ScreenplayTextCommentDelegate(commentsView));
    commentsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    commentsViewContextMenu->setModel(commentsViewContextMenuModel);
}

void ScreenplayTextCommentsView::Implementation::updateCommentsViewContextMenu(const QModelIndexList& _indexes)
{
    if (_indexes.isEmpty()) {
        return;
    }

    commentsViewContextMenuModel->clear();

    //
    // Настраиваем контекстное меню для одного элемента
    //
    if (_indexes.size() == 1) {
        if (_indexes.constFirst().data(BusinessLayer::ScreenplayTextCommentsModel::ReviewMarkIsDone).toBool()) {
            auto markAsUndone = new QStandardItem(tr("Mark as undone"));
            markAsUndone->setData(u8"\U000F0131", Qt::DecorationRole);
            markAsUndone->setData(static_cast<int>(ContextMenuAction::MarkAsUndone), Qt::UserRole);
            commentsViewContextMenuModel->appendRow(markAsUndone);
        } else {
            auto markAsDone = new QStandardItem(tr("Mark as done"));
            markAsDone->setData(u8"\U000F0135", Qt::DecorationRole);
            markAsDone->setData(static_cast<int>(ContextMenuAction::MarkAsDone), Qt::UserRole);
            commentsViewContextMenuModel->appendRow(markAsDone);
        }
        auto remove = new QStandardItem(tr("Remove"));
        remove->setData(u8"\U000F01B4", Qt::DecorationRole);
        remove->setData(static_cast<int>(ContextMenuAction::Remove), Qt::UserRole);
        commentsViewContextMenuModel->appendRow(remove);

    }
    //
    // Настраиваем контекстное меню для нескольких выделенных элементов
    //
    else {
        auto markAsDone = new QStandardItem(tr("Mark selected notes as done"));
        markAsDone->setData(u8"\U000F0139", Qt::DecorationRole);
        markAsDone->setData(static_cast<int>(ContextMenuAction::MarkAsDone), Qt::UserRole);
        commentsViewContextMenuModel->appendRow(markAsDone);
        //
        auto markAsUndone = new QStandardItem(tr("Mark selected notes as undone"));
        markAsUndone->setData(u8"\U000F0137", Qt::DecorationRole);
        markAsUndone->setData(static_cast<int>(ContextMenuAction::MarkAsUndone), Qt::UserRole);
        commentsViewContextMenuModel->appendRow(markAsUndone);
        //
        auto remove = new QStandardItem(tr("Remove selected notes"));
        remove->setData(u8"\U000F01B4", Qt::DecorationRole);
        remove->setData(static_cast<int>(ContextMenuAction::Remove), Qt::UserRole);
        commentsViewContextMenuModel->appendRow(remove);
    }
}


// ****


ScreenplayTextCommentsView::ScreenplayTextCommentsView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->commentsView);
    addWidget(d->addCommentWidget);


    connect(d->commentsView, &Tree::currentIndexChanged, this, &ScreenplayTextCommentsView::commentSelected);
    connect(d->commentsView, &Tree::customContextMenuRequested, this, [this] (const QPoint& _pos) {
        if (d->commentsView->selectedIndexes().isEmpty()) {
            return;
        }

        d->updateCommentsViewContextMenu(d->commentsView->selectedIndexes());
        d->commentsViewContextMenu->showContextMenu(d->commentsView->mapToGlobal(_pos));
    });
    connect(d->commentsViewContextMenu, &ContextMenu::clicked, d->commentsViewContextMenu, &ContextMenu::hide);
    connect(d->commentsViewContextMenu, &ContextMenu::clicked, this, [this] (const QModelIndex& _contextMenuIndex) {
        const auto action = _contextMenuIndex.data(Qt::UserRole).toInt();
        switch (static_cast<Implementation::ContextMenuAction>(action)) {
            case Implementation::ContextMenuAction::MarkAsDone: {
                emit markAsDoneRequested(d->commentsView->selectedIndexes());
                break;
            }

            case Implementation::ContextMenuAction::MarkAsUndone: {
                emit markAsUndoneRequested(d->commentsView->selectedIndexes());
                break;
            }

            case Implementation::ContextMenuAction::Remove: {
                emit removeRequested(d->commentsView->selectedIndexes());
                break;
            }
        }
    });
    connect(d->addCommentWidget, &ScreenplayTextAddCommentWidget::savePressed, this, [this] {
        emit addCommentRequested(d->addCommentColor, d->addCommentWidget->comment());
        setCurrentWidget(d->commentsView);
    });
    connect(d->addCommentWidget, &ScreenplayTextAddCommentWidget::cancelPressed, this, [this] {
        setCurrentWidget(d->commentsView);
    });


    designSystemChangeEvent(nullptr);
}

ScreenplayTextCommentsView::~ScreenplayTextCommentsView() = default;

void ScreenplayTextCommentsView::setModel(QAbstractItemModel* _model)
{
    d->commentsView->setModel(_model);
}

void ScreenplayTextCommentsView::setCurrentIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);
    d->commentsView->setCurrentIndex(_index);
}

void ScreenplayTextCommentsView::showAddCommentView(const QColor& _withColor)
{
    d->addCommentColor = _withColor;
    d->addCommentWidget->setComment({});
    setCurrentWidget(d->addCommentWidget);
    QTimer::singleShot(animationDuration(), d->addCommentWidget, qOverload<>(&ScreenplayTextAddCommentWidget::setFocus));
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
