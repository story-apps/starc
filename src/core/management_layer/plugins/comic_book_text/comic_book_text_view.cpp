#include "comic_book_text_view.h"

#include "comments/comic_book_text_comments_model.h"
#include "comments/comic_book_text_comments_toolbar.h"
#include "comments/comic_book_text_comments_view.h"
#include "text/comic_book_text_edit.h"
#include "text/comic_book_text_edit_shortcuts_manager.h"
#include "text/comic_book_text_edit_toolbar.h"
#include "text/comic_book_text_fast_format_widget.h"
#include "text/comic_book_text_search_manager.h"

#include <business_layer/document/comic_book/text/comic_book_text_block_data.h>
#include <business_layer/document/comic_book/text/comic_book_text_cursor.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
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

#include <QAction>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

namespace Ui {

namespace {
const int kTypeDataRole = Qt::UserRole + 100;

const int kFastFormatTabIndex = 0;
const int kCommentsTabIndex = 1;

const QString kSettingsKey = "comicBook-text";
const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
const QString kSidebarPanelIndexKey = kSettingsKey + "/sidebar-panel-index";
} // namespace

class ComicBookTextView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolBarUi();

    /**
     * @brief Обновить текущий отображаемый тип абзаца в панели инструментов
     */
    void updateToolBarCurrentParagraphTypeName();

    /**
     * @brief Обновить компоновку страницы
     */
    void updateTextEditPageMargins();

    /**
     * @brief Обновить видимость и положение панели инструментов рецензирования
     */
    void updateCommentsToolBar();

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


    BusinessLayer::ComicBookTextCommentsModel* commentsModel = nullptr;

    ComicBookTextEdit* comicBookText = nullptr;
    ComicBookTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;

    ComicBookTextEditToolbar* toolbar = nullptr;
    BusinessLayer::ComicBookTextSearchManager* searchManager = nullptr;
    FloatingToolbarAnimator* toolbarAnimation = nullptr;
    BusinessLayer::ComicBookParagraphType currentParagraphType
        = BusinessLayer::ComicBookParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;

    ComicBookTextCommentsToolbar* commentsToolbar = nullptr;

    Shadow* sidebarShadow = nullptr;

    bool isSidebarShownFirstTime = true;
    Widget* sidebarWidget = nullptr;
    TabBar* sidebarTabs = nullptr;
    StackWidget* sidebarContent = nullptr;
    ComicBookTextFastFormatWidget* fastFormatWidget = nullptr;
    ComicBookTextCommentsView* commentsView = nullptr;

    Splitter* splitter = nullptr;
};

ComicBookTextView::Implementation::Implementation(QWidget* _parent)
    : commentsModel(new BusinessLayer::ComicBookTextCommentsModel(_parent))
    , comicBookText(new ComicBookTextEdit(_parent))
    , shortcutsManager(comicBookText)
    , scalableWrapper(new ScalableWrapper(comicBookText, _parent))
    , toolbar(new ComicBookTextEditToolbar(scalableWrapper))
    , searchManager(new BusinessLayer::ComicBookTextSearchManager(scalableWrapper, comicBookText))
    , toolbarAnimation(new FloatingToolbarAnimator(_parent))
    , paragraphTypesModel(new QStandardItemModel(toolbar))
    , commentsToolbar(new ComicBookTextCommentsToolbar(_parent))
    , sidebarShadow(new Shadow(Qt::RightEdge, scalableWrapper))
    , sidebarWidget(new Widget(_parent))
    , sidebarTabs(new TabBar(_parent))
    , sidebarContent(new StackWidget(_parent))
    , fastFormatWidget(new ComicBookTextFastFormatWidget(_parent))
    , commentsView(new ComicBookTextCommentsView(_parent))
    , splitter(new Splitter(_parent))

{
    toolbar->setParagraphTypesModel(paragraphTypesModel);

    commentsToolbar->hide();

    comicBookText->setVerticalScrollBar(new ScrollBar);
    comicBookText->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();
    shortcutsManager.setShortcutsContext(scalableWrapper);

    comicBookText->setUsePageMode(true);

    sidebarWidget->hide();
    sidebarTabs->setFixed(true);
    sidebarTabs->addTab({}); // fastformat
    sidebarTabs->setTabVisible(kFastFormatTabIndex, false);
    sidebarTabs->addTab({}); // comments
    sidebarTabs->setTabVisible(kCommentsTabIndex, false);
    sidebarContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidebarContent->setAnimationType(StackWidget::AnimationType::Slide);
    sidebarContent->addWidget(fastFormatWidget);
    sidebarContent->addWidget(commentsView);
    fastFormatWidget->hide();
    fastFormatWidget->setParagraphTypesModel(paragraphTypesModel);
    commentsView->setModel(commentsModel);
    commentsView->hide();
}

void ComicBookTextView::Implementation::updateToolBarUi()
{
    toolbar->move(
        QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toPoint());
    toolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolbar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolbar->raise();

    searchManager->toolbar()->move(
        QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toPoint());
    searchManager->toolbar()->setBackgroundColor(Ui::DesignSystem::color().primary());
    searchManager->toolbar()->setTextColor(Ui::DesignSystem::color().onPrimary());
    searchManager->toolbar()->raise();

    toolbarAnimation->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolbarAnimation->setTextColor(Ui::DesignSystem::color().onPrimary());

    commentsToolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    commentsToolbar->setTextColor(Ui::DesignSystem::color().onPrimary());
    commentsToolbar->raise();
    updateCommentsToolBar();
}

void ComicBookTextView::Implementation::updateToolBarCurrentParagraphTypeName()
{
    auto paragraphType = comicBookText->currentParagraphType();
    if (currentParagraphType == paragraphType) {
        return;
    }

    currentParagraphType = paragraphType;

    if (paragraphType == BusinessLayer::ComicBookParagraphType::FolderFooter) {
        paragraphType = BusinessLayer::ComicBookParagraphType::FolderHeader;
        toolbar->setParagraphTypesEnabled(false);
        fastFormatWidget->setEnabled(false);
    } else {
        toolbar->setParagraphTypesEnabled(true);
        fastFormatWidget->setEnabled(true);
    }

    for (int itemRow = 0; itemRow < paragraphTypesModel->rowCount(); ++itemRow) {
        const auto item = paragraphTypesModel->item(itemRow);
        const auto itemType
            = static_cast<BusinessLayer::ComicBookParagraphType>(item->data(kTypeDataRole).toInt());
        if (itemType == paragraphType) {
            toolbar->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            fastFormatWidget->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            return;
        }
    }
}

void ComicBookTextView::Implementation::updateTextEditPageMargins()
{
    if (comicBookText->usePageMode()) {
        return;
    }

    const QMarginsF pageMargins
        = QMarginsF{ 15, 20 / scalableWrapper->zoomRange(), 12 / scalableWrapper->zoomRange(), 5 };
    comicBookText->setPageMarginsMm(pageMargins);
}

void ComicBookTextView::Implementation::updateCommentsToolBar()
{
    if (!toolbar->isCommentsModeEnabled() || !comicBookText->textCursor().hasSelection()) {
        commentsToolbar->hideToolbar();
        return;
    }

    //
    // Определяем точку на границе страницы, либо если страница не влезает в экран, то с боку экрана
    //
    const int x = (comicBookText->width() - comicBookText->viewport()->width()) / 2
        + comicBookText->viewport()->width() - commentsToolbar->width();
    const qreal textRight = scalableWrapper->mapFromEditor(QPoint(x, 0)).x();
    const auto cursorRect = comicBookText->cursorRect();
    const auto globalCursorCenter = comicBookText->mapToGlobal(cursorRect.center());
    const auto localCursorCenter
        = commentsToolbar->parentWidget()->mapFromGlobal(globalCursorCenter);
    //
    // И смещаем панель рецензирования к этой точке
    //
    commentsToolbar->moveToolbar(QPoint(std::min(scalableWrapper->width() - commentsToolbar->width()
                                                     - Ui::DesignSystem::layout().px24(),
                                                 textRight),
                                        localCursorCenter.y() - (commentsToolbar->height() / 3)));

    //
    // Если панель ещё не была показана, отобразим её
    //
    commentsToolbar->showToolbar();
}

void ComicBookTextView::Implementation::updateSideBarVisibility(QWidget* _container)
{
    const bool isSidebarShouldBeVisible
        = toolbar->isFastFormatPanelVisible() || toolbar->isCommentsModeEnabled();
    if (sidebarWidget->isVisible() == isSidebarShouldBeVisible) {
        return;
    }

    sidebarShadow->setVisible(isSidebarShouldBeVisible);
    sidebarWidget->setVisible(isSidebarShouldBeVisible);

    if (isSidebarShownFirstTime && isSidebarShouldBeVisible) {
        isSidebarShownFirstTime = false;
        const auto sideBarWidth = sidebarContent->sizeHint().width();
        splitter->setSizes({ _container->width() - sideBarWidth, sideBarWidth });
    }
}

void ComicBookTextView::Implementation::addReviewMark(const QColor& _textColor,
                                                      const QColor& _backgroundColor,
                                                      const QString& _comment)
{
    //
    // Добавим заметку
    //
    const auto textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    comicBookText->addReviewMark(textColor, _backgroundColor, _comment);

    //
    // Снимем выделение, чтобы пользователь получил обратную связь от приложения, что выделение
    // добавлено
    //
    BusinessLayer::ComicBookTextCursor cursor(comicBookText->textCursor());
    const auto selectionInterval = cursor.selectionInterval();
    //
    // ... делаем танец с бубном, чтобы получить сигнал об обновлении позиции курсора
    //     и выделить новую заметку в общем списке
    //
    cursor.setPosition(selectionInterval.to);
    comicBookText->setTextCursorReimpl(cursor);
    cursor.setPosition(selectionInterval.from);
    comicBookText->setTextCursorReimpl(cursor);

    //
    // Фокусируем редактор сценария, чтобы пользователь мог продолжать работать с ним
    //
    scalableWrapper->setFocus();
}


// ****


ComicBookTextView::ComicBookTextView(QWidget* _parent)
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

    connect(d->toolbar, &ComicBookTextEditToolbar::undoPressed, d->comicBookText,
            &ComicBookTextEdit::undo);
    connect(d->toolbar, &ComicBookTextEditToolbar::redoPressed, d->comicBookText,
            &ComicBookTextEdit::redo);
    connect(d->toolbar, &ComicBookTextEditToolbar::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::ComicBookParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->comicBookText->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    connect(d->toolbar, &ComicBookTextEditToolbar::fastFormatPanelVisibleChanged, this,
            [this](bool _visible) {
                d->sidebarTabs->setTabVisible(kFastFormatTabIndex, _visible);
                d->fastFormatWidget->setVisible(_visible);
                if (_visible) {
                    d->sidebarTabs->setCurrentTab(kFastFormatTabIndex);
                    d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
                }
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &ComicBookTextEditToolbar::commentsModeEnabledChanged, this,
            [this](bool _enabled) {
                d->sidebarTabs->setTabVisible(kCommentsTabIndex, _enabled);
                d->commentsView->setVisible(_enabled);
                if (_enabled) {
                    d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                    d->sidebarContent->setCurrentWidget(d->commentsView);
                    d->updateCommentsToolBar();
                }
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &ComicBookTextEditToolbar::searchPressed, this, [this] {
        d->toolbarAnimation->switchToolbars(d->toolbar->searchIcon(),
                                            d->toolbar->searchIconPosition(), d->toolbar,
                                            d->searchManager->toolbar());
    });
    //
    connect(d->searchManager, &BusinessLayer::ComicBookTextSearchManager::hideToolbarRequested,
            this, [this] { d->toolbarAnimation->switchToolbarsBack(); });
    //
    connect(d->commentsToolbar, &ComicBookTextCommentsToolbar::textColorChangeRequested, this,
            [this](const QColor& _color) { d->addReviewMark(_color, {}, {}); });
    connect(d->commentsToolbar, &ComicBookTextCommentsToolbar::textBackgoundColorChangeRequested,
            this, [this](const QColor& _color) { d->addReviewMark({}, _color, {}); });
    connect(d->commentsToolbar, &ComicBookTextCommentsToolbar::commentAddRequested, this,
            [this](const QColor& _color) {
                d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                d->commentsView->showAddCommentView(_color);
            });
    connect(d->commentsView, &ComicBookTextCommentsView::addReviewMarkRequested, this,
            [this](const QColor& _color, const QString& _comment) {
                d->addReviewMark({}, _color, _comment);
            });
    connect(d->commentsView, &ComicBookTextCommentsView::addReviewMarkCommentRequested, this,
            [this](const QModelIndex& _index, const QString& _comment) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->addComment(_index, _comment);
            });
    connect(d->commentsView, &ComicBookTextCommentsView::commentSelected, this,
            [this](const QModelIndex& _index) {
                const auto positionHint = d->commentsModel->mapToComicBook(_index);
                const auto position = d->comicBookText->positionForModelIndex(positionHint.index)
                    + positionHint.blockPosition;
                auto cursor = d->comicBookText->textCursor();
                cursor.setPosition(position);
                d->comicBookText->ensureCursorVisible(cursor);
                d->scalableWrapper->setFocus();
            });
    connect(d->commentsView, &ComicBookTextCommentsView::markAsDoneRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->markAsDone(_indexes);
            });
    connect(d->commentsView, &ComicBookTextCommentsView::markAsUndoneRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->markAsUndone(_indexes);
            });
    connect(d->commentsView, &ComicBookTextCommentsView::removeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->remove(_indexes);
            });
    //
    connect(d->sidebarTabs, &TabBar::currentIndexChanged, this, [this](int _currentIndex) {
        if (_currentIndex == kFastFormatTabIndex) {
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
        } else {
            d->sidebarContent->setCurrentWidget(d->commentsView);
        }
    });
    //
    connect(d->fastFormatWidget, &ComicBookTextFastFormatWidget::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::ComicBookParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->comicBookText->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    //
    connect(d->scalableWrapper->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolBar(); });
    connect(d->scalableWrapper->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolBar(); });
    connect(
        d->scalableWrapper, &ScalableWrapper::zoomRangeChanged, this,
        [this] {
            d->updateTextEditPageMargins();
            d->updateCommentsToolBar();
        },
        Qt::QueuedConnection);
    //
    auto handleCursorPositionChanged = [this] {
        //
        // Обновим состояние панелей форматов
        //
        d->updateToolBarCurrentParagraphTypeName();
        //
        // Уведомим навигатор клиентов, о смене текущего элемента
        //
        const auto comicBookModelIndex = d->comicBookText->currentModelIndex();
        emit currentModelIndexChanged(comicBookModelIndex);
        //
        // Если необходимо выберем соответствующий комментарий
        //
        const auto positionInBlock = d->comicBookText->textCursor().positionInBlock();
        const auto commentModelIndex
            = d->commentsModel->mapFromComicBook(comicBookModelIndex, positionInBlock);
        d->commentsView->setCurrentIndex(commentModelIndex);
    };
    connect(d->comicBookText, &ComicBookTextEdit::paragraphTypeChanged, this,
            handleCursorPositionChanged);
    connect(d->comicBookText, &ComicBookTextEdit::cursorPositionChanged, this,
            handleCursorPositionChanged);
    connect(d->comicBookText, &ComicBookTextEdit::selectionChanged, this,
            [this] { d->updateCommentsToolBar(); });

    updateTranslations();
    designSystemChangeEvent(nullptr);

    reconfigure({});
}

ComicBookTextView::~ComicBookTextView() = default;

QWidget* ComicBookTextView::asQWidget()
{
    return this;
}

void ComicBookTextView::toggleFullScreen(bool _isFullScreen)
{
    d->toolbar->setVisible(!_isFullScreen);
}

void ComicBookTextView::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->paragraphTypesModel->clear();

    using namespace BusinessLayer;
    const auto usedTemplate = BusinessLayer::TemplatesFacade::comicBookTemplate();
    const QVector<ComicBookParagraphType> types = {
        ComicBookParagraphType::Page,
        ComicBookParagraphType::Panel,
        ComicBookParagraphType::Description,
        ComicBookParagraphType::Character,
        ComicBookParagraphType::Dialogue,
        ComicBookParagraphType::InlineNote,
        ComicBookParagraphType::UnformattedText,
        ComicBookParagraphType::FolderHeader,
    };
    for (const auto type : types) {
        if (!usedTemplate.paragraphStyle(type).isActive()) {
            continue;
        }

        auto typeItem = new QStandardItem(toDisplayString(type));
        typeItem->setData(d->shortcutsManager.shortcut(type), Qt::WhatsThisRole);
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        d->paragraphTypesModel->appendRow(typeItem);
    }

    UiHelper::initSpellingFor(d->comicBookText);

    d->shortcutsManager.reconfigure();

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey)) {
        d->comicBookText->reinit();
    }

    //    if (_changedSettingsKeys.isEmpty()
    //        || _changedSettingsKeys.contains(
    //            DataStorageLayer::kComponentsComicBookEditorShowSceneNumbersKey)) {
    //        d->comicBookText->setShowSceneNumber(
    //            settingsValue(DataStorageLayer::kComponentsComicBookEditorShowSceneNumbersKey).toBool(),
    //            settingsValue(DataStorageLayer::kComponentsComicBookEditorShowSceneNumbersOnRightKey)
    //                .toBool(),
    //            settingsValue(DataStorageLayer::kComponentsComicBookEditorShowSceneNumberOnLeftKey)
    //                .toBool());
    //    }
    //    if (_changedSettingsKeys.isEmpty()
    //        || _changedSettingsKeys.contains(
    //            DataStorageLayer::kComponentsComicBookEditorShowDialogueNumberKey)) {
    //        d->comicBookText->setShowDialogueNumber(
    //            settingsValue(DataStorageLayer::kComponentsComicBookEditorShowDialogueNumberKey)
    //                .toBool());
    //    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationShowDocumentsPagesKey)) {
        const auto usePageMode
            = settingsValue(DataStorageLayer::kApplicationShowDocumentsPagesKey).toBool();
        d->comicBookText->setUsePageMode(usePageMode);
        if (usePageMode) {
            d->comicBookText->reinit();
        } else {
            d->updateTextEditPageMargins();
        }
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationHighlightCurrentLineKey)) {
        d->comicBookText->setHighlightCurrentLine(
            settingsValue(DataStorageLayer::kApplicationHighlightCurrentLineKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationFocusCurrentParagraphKey)) {
        d->comicBookText->setFocusCurrentParagraph(
            settingsValue(DataStorageLayer::kApplicationFocusCurrentParagraphKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationUseTypewriterScrollingKey)) {
        d->comicBookText->setUseTypewriterScrolling(
            settingsValue(DataStorageLayer::kApplicationUseTypewriterScrollingKey).toBool());
    }
}

void ComicBookTextView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor = settingsValue(kScaleFactorKey, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);

    const auto isCommentsModeEnabled = settingsValue(kIsCommentsModeEnabledKey, false).toBool();
    d->toolbar->setCommentsModeEnabled(isCommentsModeEnabled);
    const auto isFastFormatPanelVisible
        = settingsValue(kIsFastFormatPanelVisibleKey, false).toBool();
    d->toolbar->setFastFormatPanelVisible(isFastFormatPanelVisible);
    const auto sidebarPanelIndex = settingsValue(kSidebarPanelIndexKey, 0).toInt();
    d->sidebarTabs->setCurrentTab(sidebarPanelIndex);

    const auto sidebarState = settingsValue(kSidebarStateKey);
    if (sidebarState.isValid()) {
        d->isSidebarShownFirstTime = false;
        d->splitter->restoreState(sidebarState.toByteArray());
    }
}

void ComicBookTextView::saveViewSettings()
{
    setSettingsValue(kScaleFactorKey, d->scalableWrapper->zoomRange());

    setSettingsValue(kIsFastFormatPanelVisibleKey, d->toolbar->isFastFormatPanelVisible());
    setSettingsValue(kIsCommentsModeEnabledKey, d->toolbar->isCommentsModeEnabled());
    setSettingsValue(kSidebarPanelIndexKey, d->sidebarTabs->currentTab());

    setSettingsValue(kSidebarStateKey, d->splitter->saveState());
}

void ComicBookTextView::setModel(BusinessLayer::ComicBookTextModel* _model)
{
    d->comicBookText->initWithModel(_model);
    d->commentsModel->setModel(_model);

    d->updateToolBarCurrentParagraphTypeName();
}

QModelIndex ComicBookTextView::currentModelIndex() const
{
    return d->comicBookText->currentModelIndex();
}

void ComicBookTextView::setCurrentModelIndex(const QModelIndex& _index)
{
    d->comicBookText->setCurrentModelIndex(_index);
}

int ComicBookTextView::cursorPosition() const
{
    return d->comicBookText->textCursor().position();
}

void ComicBookTextView::setCursorPosition(int _position)
{
    auto cursor = d->comicBookText->textCursor();
    cursor.setPosition(_position);
    d->comicBookText->ensureCursorVisible(cursor, false);
}

bool ComicBookTextView::eventFilter(QObject* _target, QEvent* _event)
{
    if (_target == d->scalableWrapper) {
        if (_event->type() == QEvent::Resize) {
            QTimer::singleShot(0, this, [this] { d->updateCommentsToolBar(); });
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

void ComicBookTextView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    const auto toolbarPosition
        = QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toPoint();
    d->toolbar->move(toolbarPosition);
    d->searchManager->toolbar()->move(toolbarPosition);
    d->updateCommentsToolBar();
}

void ComicBookTextView::updateTranslations()
{
    d->sidebarTabs->setTabName(kFastFormatTabIndex, tr("Formatting"));
    d->sidebarTabs->setTabName(kCommentsTabIndex, tr("Comments"));
}

void ComicBookTextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolBarUi();

    d->comicBookText->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().textEditor());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onTextEditor());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    d->scalableWrapper->setPalette(palette);
    d->comicBookText->setPalette(palette);
    palette.setColor(QPalette::Base, Qt::transparent);
    d->comicBookText->viewport()->setPalette(palette);
    d->comicBookText->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->comicBookText->completer()->setBackgroundColor(Ui::DesignSystem::color().background());

    d->splitter->setBackgroundColor(Ui::DesignSystem::color().background());

    d->sidebarTabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->sidebarTabs->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->sidebarContent->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
