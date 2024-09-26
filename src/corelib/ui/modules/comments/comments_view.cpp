#include "comments_view.h"

#include "add_comment_view.h"
#include "comment_delegate.h"
#include "comment_replies_view.h"
#include "comments_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/tree/tree.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QAction>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

class CommentsView::Implementation
{
public:
    explicit Implementation(CommentsView* _q);

    /**
     * @brief Действия контекстного меню
     */
    enum class ContextMenuAction {
        MarkAsDone,
        MarkAsUndone,
        Remove,
    };

    /**
     * @brief Обновить контекстное меню для заданного списка элементов
     */
    void updateCommentsViewContextMenu(const QModelIndexList& _indexes);

    /**
     * @brief Редактировать заданный комментарий
     */
    void editComment(const QModelIndex& _index);

    /**
     * @brief Обновить действия контекстного меню для заданного ответа
     */
    void updateReplyContextMenu(int _commentIndex);


    CommentsView* q = nullptr;

    bool isReadOnly = false;

    Tree* commentsView = nullptr;
    ContextMenu* contextMenu = nullptr;

    AddCommentView* addCommentView = nullptr;
    QModelIndex commentIndex;
    QColor commentColor;

    CommentRepliesView* repliesView = nullptr;
};

CommentsView::Implementation::Implementation(CommentsView* _q)
    : q(_q)
    , commentsView(new Tree(_q))
    , contextMenu(new ContextMenu(commentsView))
    , addCommentView(new AddCommentView(_q))
    , repliesView(new CommentRepliesView(_q))
{
    commentsView->setAutoAdjustSize(true);
    commentsView->setContextMenuPolicy(Qt::CustomContextMenu);
    commentsView->setItemDelegate(new CommentDelegate(commentsView));
    commentsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void CommentsView::Implementation::updateCommentsViewContextMenu(const QModelIndexList& _indexes)
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
        connect(edit, &QAction::triggered, q,
                [this] { editComment(commentsView->selectedIndexes().constFirst()); });
        menuActions.append(edit);
        auto discuss = new QAction(tr("Discuss"));
        discuss->setIconText(u8"\U000F0860");
        connect(discuss, &QAction::triggered, q, [this] {
            q->showCommentRepliesView(commentsView->selectedIndexes().constFirst());
        });
        menuActions.append(discuss);
        //
        // ... отметить как выполненное можно только редакторскую заметку, но не ривизию
        //
        if (_indexes.constFirst()
                .data(BusinessLayer::CommentsModel::ReviewMarkIsRevisionRole)
                .toBool()
            == false) {
            if (_indexes.constFirst()
                    .data(BusinessLayer::CommentsModel::ReviewMarkIsDoneRole)
                    .toBool()) {
                auto markAsUndone = new QAction(tr("Mark as undone"));
                markAsUndone->setIconText(u8"\U000F0131");
                connect(markAsUndone, &QAction::triggered, q,
                        [this] { emit q->markAsUndoneRequested(commentsView->selectedIndexes()); });
                menuActions.append(markAsUndone);
            } else {
                auto markAsDone = new QAction(tr("Mark as done"));
                markAsDone->setIconText(u8"\U000F0135");
                connect(markAsDone, &QAction::triggered, q,
                        [this] { emit q->markAsDoneRequested(commentsView->selectedIndexes()); });
                menuActions.append(markAsDone);
            }
        }
        auto remove = new QAction(tr("Remove"));
        remove->setIconText(u8"\U000F01B4");
        connect(remove, &QAction::triggered, q,
                [this] { emit q->removeRequested(commentsView->selectedIndexes()); });
        menuActions.append(remove);
    }
    //
    // Настраиваем контекстное меню для нескольких выделенных элементов
    //
    else {
        auto markAsDone = new QAction(tr("Mark selected notes as done"));
        markAsDone->setIconText(u8"\U000F0139");
        connect(markAsDone, &QAction::triggered, q,
                [this] { emit q->markAsDoneRequested(commentsView->selectedIndexes()); });
        menuActions.append(markAsDone);
        //
        auto markAsUndone = new QAction(tr("Mark selected notes as undone"));
        markAsUndone->setIconText(u8"\U000F0137");
        connect(markAsUndone, &QAction::triggered, q,
                [this] { emit q->markAsUndoneRequested(commentsView->selectedIndexes()); });
        menuActions.append(markAsUndone);
        //
        auto remove = new QAction(tr("Remove selected notes"));
        remove->setIconText(u8"\U000F01B4");
        connect(remove, &QAction::triggered, q,
                [this] { emit q->removeRequested(commentsView->selectedIndexes()); });
        menuActions.append(remove);
    }

    contextMenu->setActions(menuActions);
}

void CommentsView::Implementation::editComment(const QModelIndex& _index)
{
    commentIndex = _index;
    q->showAddCommentView(
        commentIndex.data(BusinessLayer::CommentsModel::ReviewMarkColorRole).value<QColor>(),
        commentIndex.data(BusinessLayer::CommentsModel::ReviewMarkCommentRole).toString(),
        commentsView->visualRect(commentIndex).top());
}

void CommentsView::Implementation::updateReplyContextMenu(int _commentIndex)
{
    const auto replyIndex = _commentIndex + 1;
    const auto comments = repliesView->commentIndex()
                              .data(BusinessLayer::CommentsModel::ReviewMarkRepliesRole)
                              .value<QVector<BusinessLayer::TextModelTextItem::ReviewComment>>();
    const auto comment = comments.at(replyIndex);
    if (!comment.authorEmail.isEmpty()
        && comment.authorEmail
            != DataStorageLayer::StorageFacade::settingsStorage()->accountEmail()) {
        return;
    }

    QVector<QAction*> menuActions;

    //
    // Настраиваем контекстное меню для одного элемента
    //
    auto edit = new QAction(tr("Edit"));
    edit->setIconText(u8"\U000F03EB");
    connect(edit, &QAction::triggered, q,
            [this, replyIndex, comment] { repliesView->changeMessage(replyIndex, comment.text); });
    menuActions.append(edit);
    //
    auto remove = new QAction(tr("Remove"));
    remove->setIconText(u8"\U000F01B4");
    connect(remove, &QAction::triggered, q, [this, _commentIndex] {
        const auto currentCommentIndex = commentsView->currentIndex();
        emit q->removeReviewMarkReplyRequested(currentCommentIndex, _commentIndex + 1);
        commentsView->setCurrentIndex(currentCommentIndex);
        repliesView->setCommentIndex(currentCommentIndex);
    });
    menuActions.append(remove);

    contextMenu->setActions(menuActions);
}


// ****


CommentsView::CommentsView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->commentsView);
    addWidget(d->addCommentView);
    addWidget(d->repliesView);


    connect(d->commentsView, &Tree::clicked, this, &CommentsView::commentSelected);
    connect(d->commentsView, &Tree::doubleClicked, this, [this](const QModelIndex& _index) {
        using namespace BusinessLayer;
        const auto authorName = _index.data(CommentsModel::ReviewMarkAuthorNameRole).toString();
        const auto authorEmail = _index.data(CommentsModel::ReviewMarkAuthorEmailRole).toString();
        const auto replies = _index.data(CommentsModel::ReviewMarkRepliesRole)
                                 .value<QVector<TextModelTextItem::ReviewComment>>();
        //
        // Если есть ответы, то показываем обсуждение
        //
        if (replies.size() > 1) {
            showCommentRepliesView(_index);
            return;
        }

        //
        // Если у коммента есть имейл автора
        //
        if (!authorEmail.isEmpty()) {
            //
            // ... и он совпадает с комментом текущего пользователя - изменить комментарий
            //
            if (authorEmail == DataStorageLayer::StorageFacade::settingsStorage()->accountEmail()) {
                d->editComment(_index);
            }
            //
            // ... в противном случае - добавить ответ
            //
            else {
                showCommentRepliesView(_index);
            }
            return;
        }

        //
        // Если имя автора комментария совпадает с именем текущего пользователя - изменить коммент
        //
        if (authorName == DataStorageLayer::StorageFacade::settingsStorage()->accountName()) {
            d->editComment(_index);
        }
        //
        // В противном случае - добавить ответ
        //
        else {
            showCommentRepliesView(_index);
        }
    });
    connect(d->commentsView, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        if (d->isReadOnly || d->commentsView->selectedIndexes().isEmpty()) {
            return;
        }

        d->updateCommentsViewContextMenu(d->commentsView->selectedIndexes());
        d->contextMenu->showContextMenu(d->commentsView->mapToGlobal(_pos));
    });
    connect(d->addCommentView, &AddCommentView::savePressed, this, [this] {
        if (d->commentIndex.isValid()) {
            emit changeReviewMarkRequested(d->commentIndex, d->addCommentView->comment());
            d->commentIndex = {};
        } else {
            emit addReviewMarkRequested(d->commentColor, d->addCommentView->comment());
        }
        setCurrentWidget(d->commentsView);
    });
    connect(d->addCommentView, &AddCommentView::cancelPressed, this, [this] {
        d->commentIndex = {};
        setCurrentWidget(d->commentsView);
    });
    connect(d->repliesView, &CommentRepliesView::addReplyPressed, this,
            [this](const QString& _reply) {
                const auto currentCommentIndex = d->commentsView->currentIndex();
                emit addReviewMarkReplyRequested(currentCommentIndex, _reply);
                d->commentsView->setCurrentIndex(currentCommentIndex);
            });
    connect(d->repliesView, &CommentRepliesView::editReplyPressed, this,
            [this](int _replyIndex, const QString& _reply) {
                const auto currentCommentIndex = d->commentsView->currentIndex();
                emit editReviewMarkReplyRequested(currentCommentIndex, _replyIndex, _reply);
                d->commentsView->setCurrentIndex(currentCommentIndex);
            });
    connect(d->repliesView, &CommentRepliesView::replyContextMenuRequested, this,
            [this](int _index) {
                d->updateReplyContextMenu(_index);
                d->contextMenu->showContextMenu(QCursor::pos());
            });
    connect(d->repliesView, &CommentRepliesView::closePressed, this, [this] {
        auto animationRect = d->commentsView->visualRect(d->commentsView->currentIndex());
        animationRect.setLeft(0);
        setAnimationRect(d->commentsView, animationRect);
        setCurrentWidget(d->commentsView);
        QTimer::singleShot(animationDuration(), this,
                           [this] { setAnimationType(StackWidget::AnimationType::Slide); });
    });
}

CommentsView::~CommentsView() = default;

bool CommentsView::isReadOnly() const
{
    return d->isReadOnly;
}

void CommentsView::setReadOnly(bool _readOnly)
{
    d->isReadOnly = _readOnly;
    d->repliesView->setReadOnly(_readOnly);
}

void CommentsView::setModel(QAbstractItemModel* _model)
{
    if (d->commentsView->model() != nullptr) {
        d->commentsView->model()->disconnect(this);
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

QModelIndex CommentsView::currentIndex() const
{
    return d->commentsView->currentIndex();
}

void CommentsView::setCurrentIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);
    d->commentsView->setCurrentIndex(_index);
}

void CommentsView::showAddCommentView(const QColor& _withColor, const QString& _withText,
                                      int _topMargin)
{
    d->commentColor = _withColor;
    d->addCommentView->setComment(_withText);
    d->addCommentView->setTopMargin(_topMargin);
    setCurrentWidget(d->addCommentView);
    QTimer::singleShot(animationDuration(), d->addCommentView,
                       qOverload<>(&AddCommentView::setFocus));
}

void CommentsView::showCommentRepliesView(const QModelIndex& _commentIndex)
{
    d->repliesView->setCurrentUser(
        DataStorageLayer::StorageFacade::settingsStorage()->accountName(),
        DataStorageLayer::StorageFacade::settingsStorage()->accountEmail());
    d->repliesView->setCommentIndex(_commentIndex);

    //
    // Начинаем переход после того, как закончится анимация выбора элемента
    //
    QTimer::singleShot(100, this, [this, _commentIndex] {
        setAnimationType(StackWidget::AnimationType::Expand);
        auto animationRect = d->commentsView->visualRect(_commentIndex);
        animationRect.setLeft(0);
        setAnimationRect(d->commentsView, animationRect);
        setCurrentWidget(d->repliesView);
        QTimer::singleShot(animationDuration(), d->repliesView,
                           qOverload<>(&AddCommentView::setFocus));
    });
}

void CommentsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->commentsView->setBackgroundColor(DesignSystem::color().primary());
    d->commentsView->setTextColor(DesignSystem::color().onPrimary());
    d->contextMenu->setBackgroundColor(DesignSystem::color().background());
    d->contextMenu->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui
