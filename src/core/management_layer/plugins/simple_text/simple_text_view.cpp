#include "simple_text_view.h"

#include "text/simple_text_edit.h"
#include "text/simple_text_edit_shortcuts_manager.h"
#include "text/simple_text_edit_toolbar.h"
#include "text/simple_text_fast_format_widget.h"
#include "text/simple_text_search_manager.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/bookmarks/bookmarks_model.h>
#include <ui/modules/bookmarks/bookmarks_view.h>
#include <ui/modules/comments/comments_model.h>
#include <ui/modules/comments/comments_toolbar.h>
#include <ui/modules/comments/comments_view.h>
#include <ui/widgets/floating_tool_bar/floating_toolbar_animator.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QPointer>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

namespace {
const int kTypeDataRole = Qt::UserRole + 100;

const int kFastFormatTabIndex = 0;
const int kCommentsTabIndex = 1;
const int kBookmarksTabIndex = 2;

const QString kSettingsKey = "simple-text";
const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
const QString kIsBookmarksListVisibleKey = kSettingsKey + "/is-bookmarks-list-visible";
const QString kSidebarPanelIndexKey = kSettingsKey + "/sidebar-panel-index";
} // namespace

class SimpleTextView::Implementation
{
public:
    explicit Implementation(SimpleTextView* _q);

    /**
     * @brief Переконфигурировать представление
     */
    void reconfigureTemplate(bool _withModelReinitialization = true);

    /**
     * @brief Обновить переводы дополнительных действий
     */
    void updateOptionsTranslations();

    /**
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolbarUi();
    void updateToolbarPositon();

    /**
     * @brief Обновить текущий отображаемый тип абзаца в панели инструментов
     */
    void updateToolbarCurrentParagraphTypeName();

    /**
     * @brief Обновить компоновку страницы
     */
    void updateTextEditPageMargins();

    /**
     * @brief Обновить видимость и положение панели инструментов рецензирования
     */
    void updateCommentsToolbar();

    /**
     * @brief Обновить видимость боковой панели (показана, если показана хотя бы одна из вложенных
     * панелей)
     */
    void updateSideBarVisibility(QWidget* _container);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment);


    SimpleTextView* q = nullptr;

    //
    //  Модели
    //
    QPointer<BusinessLayer::SimpleTextModel> model;
    BusinessLayer::CommentsModel* commentsModel = nullptr;
    BusinessLayer::BookmarksModel* bookmarksModel = nullptr;

    //
    // Редактор текста
    //
    SimpleTextEdit* textEdit = nullptr;
    SimpleTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;

    //
    // Панели инструментов
    //
    SimpleTextEditToolbar* toolbar = nullptr;
    BusinessLayer::SimpleTextSearchManager* searchManager = nullptr;
    FloatingToolbarAnimator* toolbarAnimation = nullptr;
    BusinessLayer::TextParagraphType currentParagraphType
        = BusinessLayer::TextParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;
    //
    CommentsToolbar* commentsToolbar = nullptr;

    //
    // Сайдбар
    //
    Shadow* sidebarShadow = nullptr;
    //
    Widget* sidebarWidget = nullptr;
    TabBar* sidebarTabs = nullptr;
    StackWidget* sidebarContent = nullptr;
    SimpleTextFastFormatWidget* fastFormatWidget = nullptr;
    CommentsView* commentsView = nullptr;
    BookmarksView* bookmarksView = nullptr;
    //
    Splitter* splitter = nullptr;

    //
    // Действия опций редактора
    //
    QAction* showBookmarksAction = nullptr;

    /**
     * @brief Группируем события об изменении положения курсора, чтобы сильно не спамить сервер
     */
    Debouncer cursorChangeNotificationsDebounser;
};

SimpleTextView::Implementation::Implementation(SimpleTextView* _q)
    : q(_q)
    , commentsModel(new BusinessLayer::CommentsModel(_q))
    , bookmarksModel(new BusinessLayer::BookmarksModel(_q))
    , textEdit(new SimpleTextEdit(_q))
    , shortcutsManager(textEdit)
    , scalableWrapper(new ScalableWrapper(textEdit, _q))
    , toolbar(new SimpleTextEditToolbar(scalableWrapper))
    , searchManager(new BusinessLayer::SimpleTextSearchManager(scalableWrapper, textEdit))
    , toolbarAnimation(new FloatingToolbarAnimator(_q))
    , paragraphTypesModel(new QStandardItemModel(toolbar))
    , commentsToolbar(new CommentsToolbar(_q))
    , sidebarShadow(new Shadow(Qt::RightEdge, scalableWrapper))
    , sidebarWidget(new Widget(_q))
    , sidebarTabs(new TabBar(_q))
    , sidebarContent(new StackWidget(_q))
    , fastFormatWidget(new SimpleTextFastFormatWidget(_q))
    , commentsView(new CommentsView(_q))
    , bookmarksView(new BookmarksView(_q))
    , splitter(new Splitter(_q))
    , showBookmarksAction(new QAction(_q))
    , cursorChangeNotificationsDebounser(500)
{
    toolbar->setParagraphTypesModel(paragraphTypesModel);

    commentsToolbar->hide();

    textEdit->setVerticalScrollBar(new ScrollBar);
    textEdit->setHorizontalScrollBar(new ScrollBar);
    shortcutsManager.setShortcutsContext(scalableWrapper);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

    textEdit->setUsePageMode(true);

    sidebarWidget->hide();
    sidebarTabs->setFixed(false);
    sidebarTabs->addTab({}); // fastformat
    sidebarTabs->setTabVisible(kFastFormatTabIndex, false);
    sidebarTabs->addTab({}); // comments
    sidebarTabs->setTabVisible(kCommentsTabIndex, false);
    sidebarTabs->addTab({}); // bookmarks
    sidebarTabs->setTabVisible(kBookmarksTabIndex, false);
    sidebarContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidebarContent->setAnimationType(StackWidget::AnimationType::Slide);
    sidebarContent->addWidget(fastFormatWidget);
    sidebarContent->addWidget(commentsView);
    sidebarContent->addWidget(bookmarksView);
    fastFormatWidget->hide();
    fastFormatWidget->setParagraphTypesModel(paragraphTypesModel);
    commentsView->setModel(commentsModel);
    commentsView->hide();
    bookmarksView->setModel(bookmarksModel);
    bookmarksView->hide();

    showBookmarksAction->setCheckable(true);
    showBookmarksAction->setIconText(u8"\U000F0E16");
}

void SimpleTextView::Implementation::reconfigureTemplate(bool _withModelReinitialization)
{
    paragraphTypesModel->clear();

    using namespace BusinessLayer;
    const auto usedTemplate = BusinessLayer::TemplatesFacade::textTemplate(model);
    const QVector<TextParagraphType> types = { TextParagraphType::ChapterHeading1,
                                               TextParagraphType::ChapterHeading2,
                                               TextParagraphType::ChapterHeading3,
                                               TextParagraphType::ChapterHeading4,
                                               TextParagraphType::ChapterHeading5,
                                               TextParagraphType::ChapterHeading6,
                                               TextParagraphType::Text,
                                               TextParagraphType::InlineNote };
    for (const auto type : types) {
        if (!usedTemplate.paragraphStyle(type).isActive()) {
            continue;
        }

        auto typeItem = new QStandardItem(toDisplayString(type));
        typeItem->setData(shortcutsManager.shortcut(type), Qt::WhatsThisRole);
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        paragraphTypesModel->appendRow(typeItem);
    }

    shortcutsManager.reconfigure();

    if (_withModelReinitialization) {
        textEdit->reinit();
    }
}

void SimpleTextView::Implementation::updateOptionsTranslations()
{
    showBookmarksAction->setText(showBookmarksAction->isChecked() ? tr("Hide bookmarks list")
                                                                  : tr("Show bookmarks list"));
}

void SimpleTextView::Implementation::updateToolbarUi()
{
    updateToolbarPositon();
    toolbar->setBackgroundColor(ColorHelper::nearby(Ui::DesignSystem::color().background()));
    toolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    toolbar->raise();

    searchManager->toolbar()->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    searchManager->toolbar()->setTextColor(Ui::DesignSystem::color().onBackground());
    searchManager->toolbar()->raise();

    toolbarAnimation->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    toolbarAnimation->setTextColor(Ui::DesignSystem::color().onBackground());

    commentsToolbar->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    commentsToolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    commentsToolbar->raise();
    updateCommentsToolbar();
}

void SimpleTextView::Implementation::updateToolbarPositon()
{
    toolbar->move(QPointF((scalableWrapper->width() - toolbar->width()) / 2.0,
                          -Ui::DesignSystem::card().shadowMargins().top())
                      .toPoint());
    searchManager->toolbar()->move(
        QPointF((scalableWrapper->width() - searchManager->toolbar()->width()) / 2.0,
                -Ui::DesignSystem::card().shadowMargins().top())
            .toPoint());
}

void SimpleTextView::Implementation::updateToolbarCurrentParagraphTypeName()
{
    auto paragraphType = textEdit->currentParagraphType();
    if (currentParagraphType == paragraphType) {
        return;
    }

    currentParagraphType = paragraphType;

    for (int itemRow = 0; itemRow < paragraphTypesModel->rowCount(); ++itemRow) {
        const auto item = paragraphTypesModel->item(itemRow);
        const auto itemType
            = static_cast<BusinessLayer::TextParagraphType>(item->data(kTypeDataRole).toInt());
        if (itemType == paragraphType) {
            toolbar->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            fastFormatWidget->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            return;
        }
    }
}

void SimpleTextView::Implementation::updateTextEditPageMargins()
{
    if (textEdit->usePageMode()) {
        return;
    }

    const QMarginsF pageMargins
        = QMarginsF{ 15, 20 / scalableWrapper->zoomRange(), 12 / scalableWrapper->zoomRange(), 5 };
    textEdit->setPageMarginsMm(pageMargins);
}

void SimpleTextView::Implementation::updateCommentsToolbar()
{
    if (textEdit->isReadOnly() || !toolbar->isCommentsModeEnabled()
        || !textEdit->textCursor().hasSelection()) {
        commentsToolbar->hideToolbar();
        return;
    }

    //
    // Определяем точку на границе страницы, либо если страница не влезает в экран, то с боку экрана
    //
    const int x = (q->isLeftToRight() ? ((textEdit->width() - textEdit->viewport()->width()) / 2
                                         + textEdit->viewport()->width())
                                      : ((textEdit->width() - textEdit->viewport()->width()) / 2))
        - commentsToolbar->width();
    const qreal textRight = scalableWrapper->mapFromEditor(QPoint(x, 0)).x();
    const auto cursorRect = textEdit->cursorRect();
    const auto globalCursorCenter = textEdit->mapToGlobal(cursorRect.center());
    const auto localCursorCenter
        = commentsToolbar->parentWidget()->mapFromGlobal(globalCursorCenter);
    //
    // И смещаем панель рецензирования к этой точке
    //
    commentsToolbar->moveToolbar(QPoint(
        q->isLeftToRight()
            ? std::min(scalableWrapper->width() - commentsToolbar->width()
                           - Ui::DesignSystem::layout().px24(),
                       textRight)
            : sidebarWidget->width() + std::max(Ui::DesignSystem::layout().px24(), textRight),
        localCursorCenter.y() - (commentsToolbar->height() / 3)));

    //
    // Если панель ещё не была показана, отобразим её
    //
    commentsToolbar->showToolbar();
}

void SimpleTextView::Implementation::updateSideBarVisibility(QWidget* _container)
{
    const bool isSidebarShouldBeVisible = toolbar->isFastFormatPanelVisible()
        || toolbar->isCommentsModeEnabled() || showBookmarksAction->isChecked();
    if (sidebarWidget->isVisible() == isSidebarShouldBeVisible) {
        return;
    }

    sidebarShadow->setVisible(isSidebarShouldBeVisible);
    sidebarWidget->setVisible(isSidebarShouldBeVisible);

    if (isSidebarShouldBeVisible && splitter->sizes().constLast() == 0) {
        const auto sideBarWidth = sidebarContent->sizeHint().width();
        splitter->setSizes({ _container->width() - sideBarWidth, sideBarWidth });
    }
}

void SimpleTextView::Implementation::addReviewMark(const QColor& _textColor,
                                                   const QColor& _backgroundColor,
                                                   const QString& _comment)
{
    //
    // Добавим заметку
    //
    const auto textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    textEdit->addReviewMark(textColor, _backgroundColor, _comment);

    //
    // Снимем выделение, чтобы пользователь получил обратную связь от приложения, что выделение
    // добавлено
    //
    BusinessLayer::TextCursor cursor(textEdit->textCursor());
    const auto selectionInterval = cursor.selectionInterval();
    //
    // ... делаем танец с бубном, чтобы получить сигнал об обновлении позиции курсора
    //     и выделить новую заметку в общем списке
    //
    cursor.setPosition(selectionInterval.to);
    textEdit->setTextCursorAndKeepScrollBars(cursor);
    cursor.setPosition(selectionInterval.from);
    textEdit->setTextCursorAndKeepScrollBars(cursor);

    //
    // Фокусируем редактор сценария, чтобы пользователь мог продолжать работать с ним
    //
    scalableWrapper->setFocus();
}


// ****


SimpleTextView::SimpleTextView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->scalableWrapper);
    d->scalableWrapper->installEventFilter(this);

    QVBoxLayout* sidebarLayout = new QVBoxLayout(d->sidebarWidget);
    sidebarLayout->setContentsMargins({});
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(d->sidebarTabs);
    sidebarLayout->addWidget(d->sidebarContent);

    d->splitter->setWidgets(d->scalableWrapper, d->sidebarWidget);
    d->splitter->setSizes({ 1, 0 });

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);

    connect(d->toolbar, &SimpleTextEditToolbar::undoPressed, d->textEdit, &SimpleTextEdit::undo);
    connect(d->toolbar, &SimpleTextEditToolbar::redoPressed, d->textEdit, &SimpleTextEdit::redo);
    connect(d->toolbar, &SimpleTextEditToolbar::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    connect(d->toolbar, &SimpleTextEditToolbar::fastFormatPanelVisibleChanged, this,
            [this](bool _visible) {
                d->updateToolbarPositon();
                d->sidebarTabs->setTabVisible(kFastFormatTabIndex, _visible);
                d->fastFormatWidget->setVisible(_visible);
                if (_visible) {
                    d->sidebarTabs->setCurrentTab(kFastFormatTabIndex);
                    d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
                }
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &SimpleTextEditToolbar::commentsModeEnabledChanged, this,
            [this](bool _enabled) {
                d->sidebarTabs->setTabVisible(kCommentsTabIndex, _enabled);
                d->commentsView->setVisible(_enabled);
                if (_enabled) {
                    d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                    d->sidebarContent->setCurrentWidget(d->commentsView);
                    d->updateCommentsToolbar();
                }
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &SimpleTextEditToolbar::searchPressed, this, [this] {
        d->toolbarAnimation->switchToolbars(d->toolbar->searchIcon(),
                                            d->toolbar->searchIconPosition(), d->toolbar,
                                            d->searchManager->toolbar());
    });
    //
    connect(d->searchManager, &BusinessLayer::SimpleTextSearchManager::hideToolbarRequested, this,
            [this] { d->toolbarAnimation->switchToolbarsBack(); });
    //
    connect(d->commentsToolbar, &CommentsToolbar::textColorChangeRequested, this,
            [this](const QColor& _color) { d->addReviewMark(_color, {}, {}); });
    connect(d->commentsToolbar, &CommentsToolbar::textBackgoundColorChangeRequested, this,
            [this](const QColor& _color) { d->addReviewMark({}, _color, {}); });
    connect(d->commentsToolbar, &CommentsToolbar::commentAddRequested, this,
            [this](const QColor& _color) {
                d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                d->commentsView->showAddCommentView(
                    _color, {},
                    d->commentsView
                        ->mapFromGlobal(d->textEdit->viewport()->mapToGlobal(
                            d->textEdit->cursorRect().topLeft()))
                        .y());
            });
    connect(d->commentsView, &CommentsView::addReviewMarkRequested, this,
            [this](const QColor& _color, const QString& _comment) {
                d->addReviewMark({}, _color, _comment);
            });
    connect(d->commentsView, &CommentsView::changeReviewMarkRequested, this,
            [this](const QModelIndex& _index, const QString& _comment) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->setComment(_index, _comment);
            });
    connect(d->commentsView, &CommentsView::addReviewMarkReplyRequested, this,
            [this](const QModelIndex& _index, const QString& _reply) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->addReply(_index, _reply);
            });
    connect(d->commentsView, &CommentsView::editReviewMarkReplyRequested, this,
            [this](const QModelIndex& _index, int _replyIndex, const QString& _reply) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->editReply(_index, _replyIndex, _reply);
            });
    connect(d->commentsView, &CommentsView::removeReviewMarkReplyRequested, this,
            [this](const QModelIndex& _index, int _replyIndex) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->removeReply(_index, _replyIndex);
            });
    connect(d->commentsView, &CommentsView::commentSelected, this,
            [this](const QModelIndex& _index) {
                const auto positionHint = d->commentsModel->mapToModel(_index);
                const auto position = d->textEdit->positionForModelIndex(positionHint.index)
                    + positionHint.blockPosition;
                auto cursor = d->textEdit->textCursor();
                cursor.setPosition(position);
                d->textEdit->ensureCursorVisible(cursor);
                d->scalableWrapper->setFocus();
            });
    connect(d->commentsView, &CommentsView::markAsDoneRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->markAsDone(_indexes);
            });
    connect(d->commentsView, &CommentsView::markAsUndoneRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->markAsUndone(_indexes);
            });
    connect(d->commentsView, &CommentsView::removeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->remove(_indexes);
            });
    //
    connect(d->bookmarksView, &BookmarksView::addBookmarkRequested, this,
            &SimpleTextView::createBookmarkRequested);
    connect(d->bookmarksView, &BookmarksView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                emit changeBookmarkRequested(d->bookmarksModel->mapToModel(_index), _text, _color);
            });
    connect(d->bookmarksView, &BookmarksView::bookmarkSelected, this,
            [this](const QModelIndex& _index) {
                const auto index = d->bookmarksModel->mapToModel(_index);
                const auto position = d->textEdit->positionForModelIndex(index);
                auto cursor = d->textEdit->textCursor();
                cursor.setPosition(position);
                d->textEdit->ensureCursorVisible(cursor);
                d->scalableWrapper->setFocus();
            });
    connect(d->bookmarksView, &BookmarksView::removeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->bookmarksModel->remove(_indexes);
            });
    //
    connect(d->sidebarTabs, &TabBar::currentIndexChanged, this, [this](int _currentIndex) {
        switch (_currentIndex) {
        case kFastFormatTabIndex: {
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
            break;
        }
        case kCommentsTabIndex: {
            d->sidebarContent->setCurrentWidget(d->commentsView);
            break;
        }
        case kBookmarksTabIndex: {
            d->sidebarContent->setCurrentWidget(d->bookmarksView);
            break;
        }
        }
    });
    //
    connect(d->fastFormatWidget, &SimpleTextFastFormatWidget::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    //
    connect(d->scalableWrapper->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolbar(); });
    connect(d->scalableWrapper->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolbar(); });
    connect(
        d->scalableWrapper, &ScalableWrapper::zoomRangeChanged, this,
        [this] {
            d->updateTextEditPageMargins();
            d->updateCommentsToolbar();
        },
        Qt::QueuedConnection);
    //
    auto handleCursorPositionChanged = [this] {
        //
        // Обновим состояние панелей форматов
        //
        d->updateToolbarCurrentParagraphTypeName();
        //
        // Уведомим навигатор клиентов, о смене текущего элемента
        //
        const auto screenplayModelIndex = d->textEdit->currentModelIndex();
        emit currentModelIndexChanged(screenplayModelIndex);
        //
        // Если необходимо выберем соответствующий комментарий
        //
        const auto positionInBlock = d->textEdit->textCursor().positionInBlock();
        const auto commentModelIndex
            = d->commentsModel->mapFromModel(screenplayModelIndex, positionInBlock);
        d->commentsView->setCurrentIndex(commentModelIndex);
        //
        // Выберем закладку, если курсор в блоке с закладкой
        //
        const auto bookmarkModelIndex = d->bookmarksModel->mapFromModel(screenplayModelIndex);
        d->bookmarksView->setCurrentIndex(bookmarkModelIndex);
        //
        // Запланируем уведомление внешних клиентов о смене позиции курсора
        //
        d->cursorChangeNotificationsDebounser.orderWork();
    };
    connect(d->textEdit, &SimpleTextEdit::paragraphTypeChanged, this, handleCursorPositionChanged);
    connect(d->textEdit, &SimpleTextEdit::cursorPositionChanged, this, handleCursorPositionChanged);
    connect(d->textEdit, &SimpleTextEdit::selectionChanged, this,
            [this] { d->updateCommentsToolbar(); });
    connect(d->textEdit, &SimpleTextEdit::addBookmarkRequested, this, [this] {
        //
        // Если список закладок показан, добавляем новую через него
        //
        if (d->showBookmarksAction->isChecked()) {
            d->bookmarksView->showAddBookmarkView(
                {},
                d->bookmarksView
                    ->mapFromGlobal(
                        d->textEdit->viewport()->mapToGlobal(d->textEdit->cursorRect().topLeft()))
                    .y());
        }
        //
        // В противном случае, через диалог
        //
        else {
            emit addBookmarkRequested();
        }
    });
    connect(d->textEdit, &SimpleTextEdit::editBookmarkRequested, this, [this] {
        //
        // Если список закладок показан, редактируем через него
        //
        if (d->showBookmarksAction->isChecked()) {
            d->bookmarksView->showAddBookmarkView(
                d->bookmarksModel->mapFromModel(currentModelIndex()),
                d->bookmarksView
                    ->mapFromGlobal(
                        d->textEdit->viewport()->mapToGlobal(d->textEdit->cursorRect().topLeft()))
                    .y());
        }
        //
        // В противном случае, через диалог
        //
        else {
            emit addBookmarkRequested();
        }
    });
    connect(d->textEdit, &SimpleTextEdit::removeBookmarkRequested, this,
            &SimpleTextView::removeBookmarkRequested);
    connect(d->textEdit, &SimpleTextEdit::showBookmarksRequested, d->showBookmarksAction,
            &QAction::toggle);
    //
    connect(d->showBookmarksAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kBookmarksTabIndex, _checked);
        d->bookmarksView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kBookmarksTabIndex);
            d->sidebarContent->setCurrentWidget(d->bookmarksView);
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(&d->cursorChangeNotificationsDebounser, &Debouncer::gotWork, this, [this] {
        emit cursorChanged(QString::number(d->textEdit->textCursor().position()).toUtf8());
    });

    reconfigure({});
}

SimpleTextView::~SimpleTextView() = default;

QWidget* SimpleTextView::asQWidget()
{
    return this;
}

void SimpleTextView::toggleFullScreen(bool _isFullScreen)
{
    d->toolbar->setVisible(!_isFullScreen);
}

QVector<QAction*> SimpleTextView::options() const
{
    return {
        d->showBookmarksAction,
    };
}

void SimpleTextView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->textEdit->setReadOnly(readOnly);
    d->toolbar->setReadOnly(readOnly);
    d->searchManager->setReadOnly(readOnly);
    d->commentsView->setReadOnly(readOnly);
    d->bookmarksView->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->shortcutsManager.setEnabled(enabled);
    d->fastFormatWidget->setEnabled(enabled);
}

void SimpleTextView::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->textEdit->setCursors(_cursors);
}

void SimpleTextView::reconfigure(const QStringList& _changedSettingsKeys)
{
    UiHelper::initSpellingFor(d->textEdit);

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey)) {
        d->reconfigureTemplate();
    }

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsSimpleTextEditorShortcutsKey)) {
        d->shortcutsManager.reconfigure();
    }

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationShowDocumentsPagesKey)) {
        const auto usePageMode
            = settingsValue(DataStorageLayer::kApplicationShowDocumentsPagesKey).toBool();
        d->textEdit->setUsePageMode(usePageMode);
        if (usePageMode) {
            d->textEdit->reinit();
        } else {
            d->updateTextEditPageMargins();
        }
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationHighlightCurrentLineKey)) {
        d->textEdit->setHighlightCurrentLine(
            settingsValue(DataStorageLayer::kApplicationHighlightCurrentLineKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationFocusCurrentParagraphKey)) {
        d->textEdit->setFocusCurrentParagraph(
            settingsValue(DataStorageLayer::kApplicationFocusCurrentParagraphKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationUseTypewriterScrollingKey)) {
        d->textEdit->setUseTypewriterScrolling(
            settingsValue(DataStorageLayer::kApplicationUseTypewriterScrollingKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey)) {
        d->textEdit->setReplaceThreeDots(
            settingsValue(DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationSmartQuotesKey)) {
        d->textEdit->setUseSmartQuotes(
            settingsValue(DataStorageLayer::kApplicationSmartQuotesKey).toBool());
    }
}

void SimpleTextView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor = settingsValue(kScaleFactorKey, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);

    const auto isBookmarksListVisible = settingsValue(kIsBookmarksListVisibleKey, false).toBool();
    d->showBookmarksAction->setChecked(isBookmarksListVisible);
    const auto isCommentsModeEnabled = settingsValue(kIsCommentsModeEnabledKey, false).toBool();
    d->toolbar->setCommentsModeEnabled(isCommentsModeEnabled);
    const auto isFastFormatPanelVisible
        = settingsValue(kIsFastFormatPanelVisibleKey, false).toBool();
    d->toolbar->setFastFormatPanelVisible(isFastFormatPanelVisible);
    const auto sidebarPanelIndex = settingsValue(kSidebarPanelIndexKey, 0).toInt();
    d->sidebarTabs->setCurrentTab(sidebarPanelIndex);

    const auto sidebarState = settingsValue(kSidebarStateKey);
    if (sidebarState.isValid()) {
        d->splitter->restoreState(sidebarState.toByteArray());
    }
}

void SimpleTextView::saveViewSettings()
{
    setSettingsValue(kScaleFactorKey, d->scalableWrapper->zoomRange());

    setSettingsValue(kIsFastFormatPanelVisibleKey, d->toolbar->isFastFormatPanelVisible());
    setSettingsValue(kIsCommentsModeEnabledKey, d->toolbar->isCommentsModeEnabled());
    setSettingsValue(kIsBookmarksListVisibleKey, d->showBookmarksAction->isChecked());
    setSettingsValue(kSidebarPanelIndexKey, d->sidebarTabs->currentTab());

    setSettingsValue(kSidebarStateKey, d->splitter->saveState());
}

void SimpleTextView::setModel(BusinessLayer::SimpleTextModel* _model)
{
    d->model = _model;

    //
    // Отслеживаем изменения некоторых параметров
    //
    if (d->model) {
        const bool reinitModel = true;
        d->reconfigureTemplate(!reinitModel);
    }

    d->textEdit->initWithModel(d->model);
    d->commentsModel->setTextModel(d->model);
    d->bookmarksModel->setTextModel(d->model);

    d->updateToolbarCurrentParagraphTypeName();
}

QModelIndex SimpleTextView::currentModelIndex() const
{
    return d->textEdit->currentModelIndex();
}

void SimpleTextView::setCurrentModelIndex(const QModelIndex& _index)
{
    d->textEdit->setCurrentModelIndex(_index);
}

int SimpleTextView::cursorPosition() const
{
    return d->textEdit->textCursor().position();
}

void SimpleTextView::setCursorPosition(int _position)
{
    auto cursor = d->textEdit->textCursor();
    cursor.setPosition(_position);
    d->textEdit->ensureCursorVisible(cursor, false);
}

int SimpleTextView::verticalScroll() const
{
    return d->textEdit->verticalScrollBar()->value();
}

void SimpleTextView::setverticalScroll(int _value)
{
    d->textEdit->verticalScrollBar()->setValue(_value);
}

bool SimpleTextView::eventFilter(QObject* _target, QEvent* _event)
{
    if (_target == d->scalableWrapper) {
        if (_event->type() == QEvent::Resize) {
            QTimer::singleShot(0, this, [this] {
                d->updateToolbarPositon();
                d->updateCommentsToolbar();
            });
        } else if (_event->type() == QEvent::KeyPress && d->searchManager->toolbar()->isVisible()
                   && d->scalableWrapper->hasFocus()) {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Escape) {
                d->toolbarAnimation->switchToolbarsBack();
            }
        }
    }

    return Widget::eventFilter(_target, _event);
}

void SimpleTextView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->updateToolbarPositon();
    d->updateCommentsToolbar();
}

void SimpleTextView::updateTranslations()
{
    d->sidebarTabs->setTabName(kFastFormatTabIndex, tr("Formatting"));
    d->sidebarTabs->setTabName(kCommentsTabIndex, tr("Comments"));
    d->sidebarTabs->setTabName(kBookmarksTabIndex, tr("Bookmarks"));

    d->updateOptionsTranslations();
}

void SimpleTextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolbarUi();

    d->textEdit->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().textEditor());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onTextEditor());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    d->scalableWrapper->setPalette(palette);
    d->textEdit->setPalette(palette);
    palette.setColor(QPalette::Base, Qt::transparent);
    d->textEdit->viewport()->setPalette(palette);
    d->textEdit->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->textEdit->completer()->setBackgroundColor(Ui::DesignSystem::color().background());

    d->splitter->setBackgroundColor(Ui::DesignSystem::color().background());

    d->sidebarTabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->sidebarTabs->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->sidebarContent->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
