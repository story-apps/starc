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
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <ui/widgets/tree/tree.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QAction>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

namespace {

/**
 * @brief Действия меню фильтров
 */
enum FilterMenuAction {
    ShowResolved,
};

/**
 * @brief Модель для фильтрации списка комментариев
 */
class CommentsFilterModel : public QSortFilterProxyModel
{
public:
    explicit CommentsFilterModel(QObject* _parent)
        : QSortFilterProxyModel(_parent)
    {
        setFilterRole(BusinessLayer::CommentsModel::ReviewMarkIsDoneRole);
    }

    void setShowResolved(bool _show)
    {
        if (m_showResolved == _show) {
            return;
        }

        m_showResolved = _show;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int _row, const QModelIndex& _parent) const override
    {
        //
        // Показвыаем все комменты, либо только те, которые не решены
        //
        return m_showResolved
            || !sourceModel()->index(_row, 0, _parent).data(filterRole()).toBool();
    }

private:
    bool m_showResolved = false;
};

} // namespace


class CommentsView::Implementation
{
public:
    explicit Implementation(CommentsView* _q);

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

    /**
     * @brief Индексы элементов исходной модели комментариев
     */
    QModelIndexList selectedSourceIndexes() const;

    /**
     * @brief Находится ли пользователь в процессе добавления нового комментария
     */
    bool isNewCommentAdding() const;


    CommentsView* q = nullptr;

    /**
     * @brief Редактор текста, к выделению в котором добавляются комментарии
     */
    QPointer<PageTextEdit> textEdit;

    bool isReadOnly = false;

    CommentsFilterModel* commentsModelWrapper = nullptr;
    ContextMenu* filterMenu = nullptr;

    Tree* commentsView = nullptr;
    ContextMenu* contextMenu = nullptr;

    AddCommentView* addCommentView = nullptr;
    QModelIndex commentIndex;
    QColor commentColor;

    CommentRepliesView* repliesView = nullptr;
};

CommentsView::Implementation::Implementation(CommentsView* _q)
    : q(_q)
    , commentsModelWrapper(new CommentsFilterModel(_q))
    , filterMenu(new ContextMenu(_q))
    , commentsView(new Tree(_q))
    , contextMenu(new ContextMenu(commentsView))
    , addCommentView(new AddCommentView(_q))
    , repliesView(new CommentRepliesView(_q))
{
    commentsView->setModel(commentsModelWrapper);
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
        const auto isRevision = _indexes.constFirst()
                                    .data(BusinessLayer::CommentsModel::ReviewMarkIsRevisionRole)
                                    .toBool();
        const auto isChange = _indexes.constFirst()
                                  .data(BusinessLayer::CommentsModel::ReviewMarkIsAdditionRole)
                                  .toBool()
            || _indexes.constFirst()
                   .data(BusinessLayer::CommentsModel::ReviewMarkIsRemovalRole)
                   .toBool();

        //
        // Редактировать можно только редакторские заметки
        //
        if (!isRevision && !isChange) {
            auto edit = new QAction(tr("Edit"));
            edit->setIconText(u8"\U000F03EB");
            connect(edit, &QAction::triggered, q,
                    [this] { editComment(selectedSourceIndexes().constFirst()); });
            menuActions.append(edit);
        }

        //
        // Обсудить можно всё
        //
        auto discuss = new QAction(tr("Discuss"));
        discuss->setIconText(u8"\U000F0860");
        connect(discuss, &QAction::triggered, q,
                [this] { q->showCommentRepliesView(selectedSourceIndexes().constFirst()); });
        menuActions.append(discuss);

        //
        // Отметить как выполненное можно только редакторскую заметку
        //
        if (!isRevision && !isChange) {
            if (_indexes.constFirst()
                    .data(BusinessLayer::CommentsModel::ReviewMarkIsDoneRole)
                    .toBool()) {
                auto markAsUndone = new QAction(tr("Mark as undone"));
                markAsUndone->setIconText(u8"\U000F0131");
                connect(markAsUndone, &QAction::triggered, q,
                        [this] { emit q->markAsUndoneRequested(selectedSourceIndexes()); });
                menuActions.append(markAsUndone);
            } else {
                auto markAsDone = new QAction(tr("Mark as done"));
                markAsDone->setIconText(u8"\U000F0135");
                connect(markAsDone, &QAction::triggered, q,
                        [this] { emit q->markAsDoneRequested(selectedSourceIndexes()); });
                menuActions.append(markAsDone);
            }
        }

        //
        // Принять можно только изменение
        //
        if (isChange) {
            auto applyChange = new QAction(tr("Apply change"));
            applyChange->setIconText(u8"\U000F012C");
            connect(applyChange, &QAction::triggered, q,
                    [this] { emit q->applyChangeRequested(selectedSourceIndexes()); });
            menuActions.append(applyChange);

            auto cancelChange = new QAction(tr("Cancel change"));
            cancelChange->setIconText(u8"\U000F0156");
            connect(cancelChange, &QAction::triggered, q,
                    [this] { emit q->cancelChangeRequested(selectedSourceIndexes()); });
            menuActions.append(cancelChange);
        }

        //
        // Удалить можно редакторскую заметку и ревизию
        //
        if (!isChange) {
            auto remove = new QAction(tr("Remove"));
            remove->setIconText(u8"\U000F01B4");
            connect(remove, &QAction::triggered, q,
                    [this] { emit q->removeRequested(selectedSourceIndexes()); });
            menuActions.append(remove);
        }
    }
    //
    // Настраиваем контекстное меню для нескольких выделенных элементов
    //
    else {
        bool hasReviewMarks = false;
        bool hasRevisions = false;
        bool hasChanges = false;
        for (const auto& index : _indexes) {
            if (index.data(BusinessLayer::CommentsModel::ReviewMarkIsRevisionRole).toBool()) {
                hasRevisions = true;
            } else if (index.data(BusinessLayer::CommentsModel::ReviewMarkIsAdditionRole).toBool()
                       || index.data(BusinessLayer::CommentsModel::ReviewMarkIsRemovalRole)
                              .toBool()) {
                hasChanges = true;
            } else {
                hasReviewMarks = true;
            }
        }

        //
        // Если есть только редакторские заметки, то их можно пометить выполненными
        //
        if (hasReviewMarks && !hasRevisions && !hasChanges) {
            auto markAsDone = new QAction(tr("Mark selected notes as done"));
            markAsDone->setIconText(u8"\U000F0139");
            connect(markAsDone, &QAction::triggered, q,
                    [this] { emit q->markAsDoneRequested(selectedSourceIndexes()); });
            menuActions.append(markAsDone);
            //
            auto markAsUndone = new QAction(tr("Mark selected notes as undone"));
            markAsUndone->setIconText(u8"\U000F0137");
            connect(markAsUndone, &QAction::triggered, q,
                    [this] { emit q->markAsUndoneRequested(selectedSourceIndexes()); });
            menuActions.append(markAsUndone);
        }
        //
        // Если есть только изменения, то их можно применить
        //
        if (!hasReviewMarks && !hasRevisions && hasChanges) {
            auto applyChanges = new QAction(tr("Apply changes"));
            applyChanges->setIconText(u8"\U000F012D");
            connect(applyChanges, &QAction::triggered, q,
                    [this] { emit q->applyChangeRequested(selectedSourceIndexes()); });
            menuActions.append(applyChanges);
            //
            auto cancelChanges = new QAction(tr("Cancel changes"));
            cancelChanges->setIconText(u8"\U000F0156");
            connect(cancelChanges, &QAction::triggered, q,
                    [this] { emit q->cancelChangeRequested(selectedSourceIndexes()); });
            menuActions.append(cancelChanges);
        }
        //
        // Если есть редакторские заметки или ревизии, то их можно удалить
        //
        if ((hasReviewMarks || hasRevisions) && !hasChanges) {
            auto remove = new QAction(tr("Remove selected notes"));
            remove->setIconText(u8"\U000F01B4");
            connect(remove, &QAction::triggered, q,
                    [this] { emit q->removeRequested(selectedSourceIndexes()); });
            menuActions.append(remove);
        }
    }

    contextMenu->setActions(menuActions);
}

void CommentsView::Implementation::editComment(const QModelIndex& _index)
{
    commentIndex = _index;
    q->showAddCommentView(
        commentIndex.data(BusinessLayer::CommentsModel::ReviewMarkColorRole).value<QColor>(),
        commentIndex.data(BusinessLayer::CommentsModel::ReviewMarkCommentRole).toString(),
        commentsView->visualRect(commentsModelWrapper->mapFromSource(commentIndex)).top());
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
        const auto currentCommentIndex = repliesView->commentIndex();
        emit q->removeReviewMarkReplyRequested(currentCommentIndex, _commentIndex + 1);
        q->setCurrentIndex(currentCommentIndex);
        repliesView->setCommentIndex(currentCommentIndex);
    });
    menuActions.append(remove);

    contextMenu->setActions(menuActions);
}

QModelIndexList CommentsView::Implementation::selectedSourceIndexes() const
{
    QModelIndexList indexes;
    for (const auto& index : commentsView->selectedIndexes()) {
        indexes.append(commentsModelWrapper->mapToSource(index));
    }
    return indexes;
}

bool CommentsView::Implementation::isNewCommentAdding() const
{
    //
    // Новый комментарий привязывается к текущему выделению в редакторе, а при редактировании
    // существующего комментария выделение не используется
    //
    return q->currentWidget() == addCommentView && addCommentView->isVisible()
        && !commentIndex.isValid();
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

    const auto showResolved
        = settingsValue(DataStorageLayer::kApplicationShowResolvedCommentsKey, false).toBool();
    d->commentsModelWrapper->setShowResolved(showResolved);

    auto showResolvedAction = new QAction(d->filterMenu);
    showResolvedAction->setCheckable(true);
    showResolvedAction->setChecked(showResolved);
    showResolvedAction->setIconText(showResolved ? u8"\U000F0132" : u8"\U000F0131");
    connect(showResolvedAction, &QAction::toggled, showResolvedAction,
            [this, showResolvedAction](bool _checked) {
                showResolvedAction->setIconText(_checked ? u8"\U000F0132" : u8"\U000F0131");
                d->commentsModelWrapper->setShowResolved(_checked);
                setSettingsValue(DataStorageLayer::kApplicationShowResolvedCommentsKey, _checked);
            });
    d->filterMenu->setActions({ showResolvedAction });

    connect(d->commentsView, &Tree::clicked, this, [this](const QModelIndex& _proxyIndex) {
        emit commentSelected(d->commentsModelWrapper->mapToSource(_proxyIndex));
    });
    connect(d->commentsView, &Tree::doubleClicked, this, [this](const QModelIndex& _proxyIndex) {
        const auto index = d->commentsModelWrapper->mapToSource(_proxyIndex);

        using namespace BusinessLayer;
        const auto authorName = index.data(CommentsModel::ReviewMarkAuthorNameRole).toString();
        const auto authorEmail = index.data(CommentsModel::ReviewMarkAuthorEmailRole).toString();
        const auto replies = index.data(CommentsModel::ReviewMarkRepliesRole)
                                 .value<QVector<TextModelTextItem::ReviewComment>>();
        //
        // Если есть ответы, то показываем обсуждение
        //
        if (replies.size() > 1) {
            showCommentRepliesView(index);
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
                d->editComment(index);
            }
            //
            // ... в противном случае - добавить ответ
            //
            else {
                showCommentRepliesView(index);
            }
            return;
        }

        //
        // Если имя автора комментария совпадает с именем текущего пользователя - изменить коммент
        //
        if (authorName == DataStorageLayer::StorageFacade::settingsStorage()->accountName()) {
            d->editComment(index);
        }
        //
        // В противном случае - добавить ответ
        //
        else {
            showCommentRepliesView(index);
        }
    });
    connect(d->commentsView, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        if (d->isReadOnly || d->selectedSourceIndexes().isEmpty()) {
            return;
        }

        d->updateCommentsViewContextMenu(d->selectedSourceIndexes());
        d->contextMenu->showContextMenu(d->commentsView->mapToGlobal(_pos));
    });
    connect(d->addCommentView, &AddCommentView::savePressed, this, [this] {
        //
        // Прерываем добавление коммента, если текст не задан
        //
        if (d->addCommentView->comment().isEmpty()) {
            d->addCommentView->setCommentError(tr("Can't save empty comment"));
            d->addCommentView->shake();
            return;
        }

        //
        // Прерываем добавление коммента, если по какой-то причине не выделен текст в редакторе
        //
        if (!d->commentIndex.isValid() && !d->addCommentView->comment().isEmpty()
            && !d->textEdit->textCursor().hasSelection()) {
            d->addCommentView->setCommentError(tr("There are no text selected for adding comment"));
            d->addCommentView->shake();
            return;
        }

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
                const auto currentCommentIndex = d->repliesView->commentIndex();
                emit addReviewMarkReplyRequested(currentCommentIndex, _reply);
                setCurrentIndex(currentCommentIndex);
            });
    connect(d->repliesView, &CommentRepliesView::editReplyPressed, this,
            [this](int _replyIndex, const QString& _reply) {
                const auto currentCommentIndex = d->repliesView->commentIndex();
                emit editReviewMarkReplyRequested(currentCommentIndex, _replyIndex, _reply);
                setCurrentIndex(currentCommentIndex);
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
    if (d->commentsModelWrapper->sourceModel() == _model) {
        return;
    }

    if (d->commentsModelWrapper->sourceModel() != nullptr) {
        d->commentsModelWrapper->sourceModel()->disconnect(this);
    }

    d->commentsModelWrapper->setSourceModel(_model);

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
    return d->commentsModelWrapper->mapToSource(d->commentsView->currentIndex());
}

void CommentsView::setCurrentIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);
    d->commentsView->setCurrentIndex(d->commentsModelWrapper->mapFromSource(_index));
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
        auto animationRect
            = d->commentsView->visualRect(d->commentsModelWrapper->mapFromSource(_commentIndex));
        animationRect.setLeft(0);
        setAnimationRect(d->commentsView, animationRect);
        setCurrentWidget(d->repliesView);
        QTimer::singleShot(animationDuration(), d->repliesView,
                           qOverload<>(&AddCommentView::setFocus));
    });
}

void CommentsView::attachToTabBar(TabBar* _tabBar, int _tabIndex)
{
    _tabBar->setTabOption(_tabIndex, u8"\U000F01D9");
    connect(_tabBar, &TabBar::tabOptionClicked, this, [this, _tabIndex](int _clickedIndex) {
        if (_clickedIndex == _tabIndex) {
            d->filterMenu->showContextMenu(QCursor::pos());
        }
    });
}

void CommentsView::setTextEdit(PageTextEdit* _textEdit)
{
    if (d->textEdit == _textEdit) {
        return;
    }

    if (d->textEdit != nullptr) {
        d->textEdit->removeEventFilter(this);
        d->textEdit->viewport()->removeEventFilter(this);
    }

    d->textEdit = _textEdit;

    if (d->textEdit != nullptr) {
        d->textEdit->installEventFilter(this);
        d->textEdit->viewport()->installEventFilter(this);
    }
}

bool CommentsView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (d->textEdit != nullptr && (_watched == d->textEdit || _watched == d->textEdit->viewport())
        && d->isNewCommentAdding()) {
        bool needToBlock = false;
        switch (_event->type()) {
        //
        // Любые нажатия мышью и перетаскивание в редакторе сбросят выделение
        //
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::ContextMenu:
        case QEvent::DragEnter:
        case QEvent::Drop: {
            needToBlock = true;
            break;
        }

        case QEvent::MouseMove: {
            needToBlock = static_cast<QMouseEvent*>(_event)->buttons() != Qt::NoButton;
            break;
        }

        case QEvent::MouseButtonRelease: {
            //
            // ... отпускание кнопки просто глотаем, т.к. нажатие уже было заблокировано
            //
            return true;
        }

        //
        // ... как и нажатия клавиш, если фокус каким-то образом оказался в редакторе
        //
        case QEvent::KeyPress: {
            needToBlock = true;
            break;
        }

        default: {
            break;
        }
        }

        if (needToBlock) {
            //
            // Оставляем выделение на месте и сигнализируем пользователю, что сначала нужно
            // завершить ввод комментария
            //
            if (_event->type() != QEvent::MouseMove) {
                d->addCommentView->shake();
            }
            //
            // ... возвращаем фокус в поле ввода комментария (отложенно, т.к. при нажатии мышью
            //     фокус передаётся редактору ещё до того, как событие попадёт в фильтр)
            //
            QTimer::singleShot(0, d->addCommentView, [this] {
                if (d->isNewCommentAdding()) {
                    d->addCommentView->setFocus();
                }
            });
            //
            // ... перетаскивание отклоняем, чтобы вставка в редактор была недоступна
            //
            if (_event->type() == QEvent::DragEnter || _event->type() == QEvent::Drop) {
                _event->ignore();
            } else {
                _event->accept();
            }
            return true;
        }
    }

    return StackWidget::eventFilter(_watched, _event);
}

void CommentsView::updateTranslations()
{
    d->filterMenu->actions().at(FilterMenuAction::ShowResolved)->setText(tr("Show resolved"));
}

void CommentsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    d->filterMenu->setBackgroundColor(DesignSystem::color().background());
    d->filterMenu->setTextColor(DesignSystem::color().onBackground());
    d->commentsView->setBottomScrollMargin(
        DesignSystem::floatingToolBar().height()
        + DesignSystem::floatingToolBar().shadowMargins().top()
        + DesignSystem::floatingToolBar().shadowMargins().bottom() + DesignSystem::layout().px24()
        + DesignSystem::layout().px8());
    d->commentsView->setBackgroundColor(DesignSystem::color().primary());
    d->commentsView->setTextColor(DesignSystem::color().onPrimary());
    d->contextMenu->setBackgroundColor(DesignSystem::color().background());
    d->contextMenu->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui
