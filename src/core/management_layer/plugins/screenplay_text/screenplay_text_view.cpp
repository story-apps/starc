#include "screenplay_text_view.h"

#include "comments/screenplay_text_comments_model.h"
#include "comments/screenplay_text_comments_toolbar.h"
#include "comments/screenplay_text_comments_view.h"
#include "text/screenplay_text_block_data.h"
#include "text/screenplay_text_cursor.h"
#include "text/screenplay_text_edit.h"
#include "text/screenplay_text_edit_shortcuts_manager.h"
#include "text/screenplay_text_edit_toolbar.h"
#include "text/screenplay_text_fast_format_widget.h"
#include "text/screenplay_text_scrollbar_manager.h"
#include "text/screenplay_text_search_manager.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/floating_tool_bar/floating_toolbar_animator.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>

#include <QAction>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

namespace Ui
{

namespace {
    const int kTypeDataRole = Qt::UserRole + 100;

    const int kFastFormatTabIndex = 0;
    const int kCommentsTabIndex = 1;

    const QString kSettingsKey = "screenplay-text";
    const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
    const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
    const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
    const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
}

class ScreenplayTextView::Implementation
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
     * @brief Обновить видимость и положение панели инструментов рецензирования
     */
    void updateCommentsToolBar();

    /**
     * @brief Обновить видимость боковой панели (показана, если показана хотя бы одна из вложенных панелей)
     */
    void updateSideBarVisibility(QWidget* _container);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor, const QString& _comment);


    BusinessLayer::ScreenplayTextCommentsModel* commentsModel = nullptr;

    ScreenplayTextEdit* screenplayText = nullptr;
    ScreenplayTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;
    ScreenplayTextScrollBarManager* screenplayTextScrollbarManager = nullptr;

    ScreenplayTextEditToolbar* toolbar = nullptr;
    BusinessLayer::ScreenplayTextSearchManager* searchManager = nullptr;
    FloatingToolbarAnimator* toolbarAnimation = nullptr;
    QHash<BusinessLayer::ScreenplayParagraphType, QString> typesToDisplayNames;
    BusinessLayer::ScreenplayParagraphType currentParagraphType = BusinessLayer::ScreenplayParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;

    ScreenplayTextCommentsToolbar* commentsToolbar = nullptr;

    Shadow* sidebarShadow = nullptr;

    bool isSidebarShownFirstTime = true;
    Widget* sidebarWidget = nullptr;
    TabBar* sidebarTabs = nullptr;
    StackWidget* sidebarContent = nullptr;
    ScreenplayTextFastFormatWidget* fastFormatWidget = nullptr;
    ScreenplayTextCommentsView* commentsView = nullptr;

    Splitter* splitter = nullptr;
};

ScreenplayTextView::Implementation::Implementation(QWidget* _parent)
    : commentsModel(new BusinessLayer::ScreenplayTextCommentsModel(_parent)),
      screenplayText(new ScreenplayTextEdit(_parent)),
      shortcutsManager(screenplayText),
      scalableWrapper(new ScalableWrapper(screenplayText, _parent)),
      screenplayTextScrollbarManager(new ScreenplayTextScrollBarManager(scalableWrapper)),
      toolbar(new ScreenplayTextEditToolbar(scalableWrapper)),
      searchManager(new BusinessLayer::ScreenplayTextSearchManager(scalableWrapper, screenplayText)),
      toolbarAnimation(new FloatingToolbarAnimator(_parent)),
      paragraphTypesModel(new QStandardItemModel(toolbar)),
      commentsToolbar(new ScreenplayTextCommentsToolbar(_parent)),
      sidebarShadow(new Shadow(Qt::RightEdge, scalableWrapper)),
      sidebarWidget(new Widget(_parent)),
      sidebarTabs(new TabBar(_parent)),
      sidebarContent(new StackWidget(_parent)),
      fastFormatWidget(new ScreenplayTextFastFormatWidget(_parent)),
      commentsView(new ScreenplayTextCommentsView(_parent)),
      splitter(new Splitter(_parent))

{
    toolbar->setParagraphTypesModel(paragraphTypesModel);

    commentsToolbar->hide();

    screenplayText->setVerticalScrollBar(new ScrollBar);
    screenplayText->setHorizontalScrollBar(new ScrollBar);
    shortcutsManager.setShortcutsContext(scalableWrapper);
    //
    // Вертикальный скрол настраивается менеджером screenplayTextScrollbarManager
    //
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();
    screenplayTextScrollbarManager->initScrollBarsSyncing();

    screenplayText->setUsePageMode(true);

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

void ScreenplayTextView::Implementation::updateToolBarUi()
{
    toolbar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    toolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolbar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolbar->raise();

    searchManager->toolbar()->move(QPointF(Ui::DesignSystem::layout().px24(),
                                Ui::DesignSystem::layout().px24()).toPoint());
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

void ScreenplayTextView::Implementation::updateToolBarCurrentParagraphTypeName()
{
    auto paragraphType = screenplayText->currentParagraphType();
    if (currentParagraphType == paragraphType) {
        return;
    }

    currentParagraphType = paragraphType;

    if (paragraphType == BusinessLayer::ScreenplayParagraphType::FolderFooter) {
        paragraphType = BusinessLayer::ScreenplayParagraphType::FolderHeader;
        toolbar->setParagraphTypesEnabled(false);
        fastFormatWidget->setEnabled(false);
    } else {
        toolbar->setParagraphTypesEnabled(true);
        fastFormatWidget->setEnabled(true);
    }

    for (int itemRow = 0; itemRow < paragraphTypesModel->rowCount(); ++itemRow) {
        const auto item = paragraphTypesModel->item(itemRow);
        const auto itemType = static_cast<BusinessLayer::ScreenplayParagraphType>(item->data(kTypeDataRole).toInt());
        if (itemType == paragraphType) {
            toolbar->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            fastFormatWidget->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            return;
        }
    }
}

void ScreenplayTextView::Implementation::updateCommentsToolBar()
{
    if (!toolbar->isCommentsModeEnabled()
        || !screenplayText->textCursor().hasSelection()) {
        commentsToolbar->hideToolbar();
        return;
    }

    //
    // Определяем точку на границе страницы, либо если страница не влезает в экран, то с боку экрана
    //
    const int x = (screenplayText->width() - screenplayText->viewport()->width()) / 2
                  + screenplayText->viewport()->width()
                  - commentsToolbar->width();
    const qreal textRight = scalableWrapper->mapFromEditor(QPoint(x, 0)).x();
    const auto cursorRect = screenplayText->cursorRect();
    const auto globalCursorCenter = screenplayText->mapToGlobal(cursorRect.center());
    const auto localCursorCenter = commentsToolbar->parentWidget()->mapFromGlobal(globalCursorCenter);
    //
    // И смещаем панель рецензирования к этой точке
    //
    commentsToolbar->moveToolbar(
                QPoint(std::min(scalableWrapper->width()
                                - commentsToolbar->width()
                                - Ui::DesignSystem::layout().px24(),
                                textRight),
                       localCursorCenter.y()
                       - (commentsToolbar->height() / 3)));

    //
    // Если панель ещё не была показана, отобразим её
    //
    commentsToolbar->showToolbar();
}

void ScreenplayTextView::Implementation::updateSideBarVisibility(QWidget* _container)
{
    const bool isSidebarShouldBeVisible = toolbar->isFastFormatPanelVisible()
                                          || toolbar->isCommentsModeEnabled();
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

void ScreenplayTextView::Implementation::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor, const QString& _comment)
{
    //
    // Добавим заметку
    //
    screenplayText->addReviewMark(_textColor, _backgroundColor, _comment);

    //
    // Снимем выделение, чтобы пользователь получил обратную связь от приложения, что выделение добавлено
    //
    BusinessLayer::ScreenplayTextCursor cursor(screenplayText->textCursor());
    const auto selectionInterval = cursor.selectionInterval();
    //
    // ... делаем танец с бубном, чтобы получить сигнал об обновлении позиции курсора
    //     и выделить новую заметку в общем списке
    //
    cursor.setPosition(selectionInterval.to);
    screenplayText->setTextCursorReimpl(cursor);
    cursor.setPosition(selectionInterval.from);
    screenplayText->setTextCursorReimpl(cursor);

    //
    // Фокусируем редактор сценария, чтобы пользователь мог продолжать работать с ним
    //
    scalableWrapper->setFocus();
}


// ****


ScreenplayTextView::ScreenplayTextView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
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

    connect(d->toolbar, &ScreenplayTextEditToolbar::undoPressed, d->screenplayText, &ScreenplayTextEdit::undo);
    connect(d->toolbar, &ScreenplayTextEditToolbar::redoPressed, d->screenplayText, &ScreenplayTextEdit::redo);
    connect(d->toolbar, &ScreenplayTextEditToolbar::paragraphTypeChanged, this, [this] (const QModelIndex& _index) {
        const auto type = static_cast<BusinessLayer::ScreenplayParagraphType>(_index.data(kTypeDataRole).toInt());
        d->screenplayText->setCurrentParagraphType(type);
        d->scalableWrapper->setFocus();
    });
    connect(d->toolbar, &ScreenplayTextEditToolbar::fastFormatPanelVisibleChanged, this, [this] (bool _visible) {
        d->sidebarTabs->setTabVisible(kFastFormatTabIndex, _visible);
        d->fastFormatWidget->setVisible(_visible);
        if (_visible) {
            d->sidebarTabs->setCurrentTab(kFastFormatTabIndex);
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
        }
        d->updateSideBarVisibility(this);
    });
    connect(d->toolbar, &ScreenplayTextEditToolbar::commentsModeEnabledChanged, this, [this] (bool _enabled) {
        d->sidebarTabs->setTabVisible(kCommentsTabIndex, _enabled);
        d->commentsView->setVisible(_enabled);
        if (_enabled) {
            d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
            d->sidebarContent->setCurrentWidget(d->commentsView);
            d->updateCommentsToolBar();
        }
        d->updateSideBarVisibility(this);
    });
    connect(d->toolbar, &ScreenplayTextEditToolbar::searchPressed, this, [this] {
        d->toolbarAnimation->switchToolbars(d->toolbar->searchIcon(), d->toolbar->searchIconPosition(),
                                            d->toolbar, d->searchManager->toolbar());
    });
    //
    connect(d->searchManager, &BusinessLayer::ScreenplayTextSearchManager::hideToolbarRequested, this, [this] {
        d->toolbarAnimation->switchToolbarsBack();
    });
    //
    connect(d->commentsToolbar, &ScreenplayTextCommentsToolbar::textColorChangeRequested,
            this, [this](const QColor& _color) { d->addReviewMark(_color, {}, {}); });
    connect(d->commentsToolbar, &ScreenplayTextCommentsToolbar::textBackgoundColorChangeRequested,
            this, [this](const QColor& _color) { d->addReviewMark({}, _color, {}); });
    connect(d->commentsToolbar, &ScreenplayTextCommentsToolbar::commentAddRequested, this, [this] (const QColor& _color) {
        d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
        d->commentsView->showAddCommentView(_color);
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::addReviewMarkRequested, this, [this] (const QColor& _color, const QString& _comment) {
        d->addReviewMark({}, _color, _comment);
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::addReviewMarkCommentRequested, this,
            [this] (const QModelIndex& _index, const QString& _comment) {
        QSignalBlocker blocker(d->commentsView);
        d->commentsModel->addComment(_index, _comment);
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::commentSelected, this, [this] (const QModelIndex& _index) {
        const auto positionHint = d->commentsModel->mapToScreenplay(_index);
        const auto position = d->screenplayText->positionForModelIndex(positionHint.index)
                              + positionHint.blockPosition;
        auto cursor = d->screenplayText->textCursor();
        cursor.setPosition(position);
        d->screenplayText->ensureCursorVisible(cursor);
        d->scalableWrapper->setFocus();
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::markAsDoneRequested, this, [this] (const QModelIndexList& _indexes) {
        QSignalBlocker blocker(d->commentsView);
        d->commentsModel->markAsDone(_indexes);
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::markAsUndoneRequested, this, [this] (const QModelIndexList& _indexes) {
        QSignalBlocker blocker(d->commentsView);
        d->commentsModel->markAsUndone(_indexes);
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::removeRequested, this, [this] (const QModelIndexList& _indexes) {
        QSignalBlocker blocker(d->commentsView);
        d->commentsModel->remove(_indexes);
    });
    //
    connect(d->sidebarTabs, &TabBar::currentIndexChanged, this, [this] (int _currentIndex) {
        if (_currentIndex == kFastFormatTabIndex) {
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
        } else {
            d->sidebarContent->setCurrentWidget(d->commentsView);
        }
    });
    //
    connect(d->fastFormatWidget, &ScreenplayTextFastFormatWidget::paragraphTypeChanged, this, [this](const QModelIndex& _index) {
        const auto type = static_cast<BusinessLayer::ScreenplayParagraphType>(_index.data(kTypeDataRole).toInt());
        d->screenplayText->setCurrentParagraphType(type);
        d->scalableWrapper->setFocus();
    });
    //
    connect(d->scalableWrapper->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        d->updateCommentsToolBar();
    });
    connect(d->scalableWrapper->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        d->updateCommentsToolBar();
    });
    connect(d->scalableWrapper, &ScalableWrapper::zoomRangeChanged, this, [this] {
        d->updateCommentsToolBar();
    }, Qt::QueuedConnection);
    //
    auto handleCursorPositionChanged = [this] {
        //
        // Обновим состояние панелей форматов
        //
        d->updateToolBarCurrentParagraphTypeName();
        //
        // Уведомим навигатор клиентов, о смене текущего элемента
        //
        const auto screenplayModelIndex = d->screenplayText->currentModelIndex();
        emit currentModelIndexChanged(screenplayModelIndex);
        //
        // Если необходимо выберем соответствующий комментарий
        //
        const auto positionInBlock = d->screenplayText->textCursor().positionInBlock();
        const auto commentModelIndex = d->commentsModel->mapFromScreenplay(screenplayModelIndex, positionInBlock);
        d->commentsView->setCurrentIndex(commentModelIndex);
    };
    connect(d->screenplayText, &ScreenplayTextEdit::paragraphTypeChanged, this, handleCursorPositionChanged);
    connect(d->screenplayText, &ScreenplayTextEdit::cursorPositionChanged, this, handleCursorPositionChanged);
    connect(d->screenplayText, &ScreenplayTextEdit::selectionChanged, this, [this] {
        d->updateCommentsToolBar();
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);

    reconfigure();
}

void ScreenplayTextView::reconfigure()
{
    d->paragraphTypesModel->clear();

    using namespace BusinessLayer;
    const auto usedTemplate = BusinessLayer::ScreenplayTemplateFacade::getTemplate();
    const QVector<ScreenplayParagraphType> types
            = { ScreenplayParagraphType::SceneHeading,
                ScreenplayParagraphType::SceneCharacters,
                ScreenplayParagraphType::Action,
                ScreenplayParagraphType::Character,
                ScreenplayParagraphType::Parenthetical,
                ScreenplayParagraphType::Dialogue,
                ScreenplayParagraphType::Lyrics,
                ScreenplayParagraphType::Shot,
                ScreenplayParagraphType::Transition,
                ScreenplayParagraphType::InlineNote,
                ScreenplayParagraphType::UnformattedText,
                ScreenplayParagraphType::FolderHeader };
    for (const auto type : types) {
        if (!usedTemplate.blockStyle(type).isActive()) {
            continue;
        }

        auto typeItem = new QStandardItem(d->typesToDisplayNames.value(type));
        typeItem->setData(d->shortcutsManager.shortcut(type), Qt::WhatsThisRole);
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        d->paragraphTypesModel->appendRow(typeItem);
    }

    auto settingsValue = [] (const QString& _key) {
        return DataStorageLayer::StorageFacade::settingsStorage()->value(
                    _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    };

    const bool useSpellChecker
            = settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool();
    d->screenplayText->setUseSpellChecker(useSpellChecker);
    if (useSpellChecker) {
        const QString languageCode
                = settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString();
        d->screenplayText->setSpellCheckLanguage(languageCode);
    }

    d->shortcutsManager.reconfigure();

    d->screenplayText->setShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumberOnLeftKey).toBool());
    d->screenplayText->setShowDialogueNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumberKey).toBool());
    d->screenplayText->setHighlightCurrentLine(
                settingsValue(DataStorageLayer::kComponentsScreenplayEditorHighlightCurrentLineKey).toBool());
}

void ScreenplayTextView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor
            = StorageFacade::settingsStorage()->value(
                  kScaleFactorKey, SettingsStorage::SettingsPlace::Application, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);

    //
    // Тут важен порядок, чтобы при загрузке активной была таки первая из вкладок
    //
    const auto isCommentsModeEnabled
            = StorageFacade::settingsStorage()->value(
                  kIsCommentsModeEnabledKey, SettingsStorage::SettingsPlace::Application, false).toBool();
    d->toolbar->setCommentsModeEnabled(isCommentsModeEnabled);
    const auto isFastFormatPanelVisible
            = StorageFacade::settingsStorage()->value(
                  kIsFastFormatPanelVisibleKey, SettingsStorage::SettingsPlace::Application, false).toBool();
    d->toolbar->setFastFormatPanelVisible(isFastFormatPanelVisible);

    const auto sidebarState
            = StorageFacade::settingsStorage()->value(
                  kSidebarStateKey, SettingsStorage::SettingsPlace::Application);
    if (sidebarState.isValid()) {
        d->isSidebarShownFirstTime = false;
        d->splitter->restoreState(sidebarState.toByteArray());
    }
}

void ScreenplayTextView::saveViewSettings()
{
    using namespace DataStorageLayer;

    StorageFacade::settingsStorage()->setValue(
        kScaleFactorKey, d->scalableWrapper->zoomRange(), SettingsStorage::SettingsPlace::Application);

    StorageFacade::settingsStorage()->setValue(
        kIsFastFormatPanelVisibleKey, d->toolbar->isFastFormatPanelVisible(), SettingsStorage::SettingsPlace::Application);
    StorageFacade::settingsStorage()->setValue(
        kIsCommentsModeEnabledKey, d->toolbar->isCommentsModeEnabled(), SettingsStorage::SettingsPlace::Application);

    StorageFacade::settingsStorage()->setValue(
        kSidebarStateKey, d->splitter->saveState(), SettingsStorage::SettingsPlace::Application);
}

void ScreenplayTextView::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->screenplayText->initWithModel(_model);
    d->screenplayTextScrollbarManager->setModel(_model);
    d->commentsModel->setModel(_model);

    d->updateToolBarCurrentParagraphTypeName();
}

QModelIndex ScreenplayTextView::currentModelIndex() const
{
    return d->screenplayText->currentModelIndex();
}

void ScreenplayTextView::setCurrentModelIndex(const QModelIndex& _index)
{
    d->screenplayText->setCurrentModelIndex(_index);
}

int ScreenplayTextView::cursorPosition() const
{
    return d->screenplayText->textCursor().position();
}

void ScreenplayTextView::setCursorPosition(int _position)
{
    auto cursor = d->screenplayText->textCursor();
    cursor.setPosition(_position);
    d->screenplayText->ensureCursorVisible(cursor, false);
}

bool ScreenplayTextView::eventFilter(QObject* _target, QEvent* _event)
{
    if (_target == d->scalableWrapper && _event->type() == QEvent::Resize) {
        QTimer::singleShot(0, this, [this] { d->updateCommentsToolBar(); });
    }

    return Widget::eventFilter(_target, _event);
}

void ScreenplayTextView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    const auto toolbarPosition = QPointF(Ui::DesignSystem::layout().px24(),
                                         Ui::DesignSystem::layout().px24()).toPoint();
    d->toolbar->move(toolbarPosition);
    d->searchManager->toolbar()->move(toolbarPosition);
    d->updateCommentsToolBar();
}

ScreenplayTextView::~ScreenplayTextView() = default;

void ScreenplayTextView::updateTranslations()
{
    using namespace  BusinessLayer;
    d->typesToDisplayNames = {{ ScreenplayParagraphType::SceneHeading, tr("Scene heading") },
                              { ScreenplayParagraphType::SceneCharacters, tr("Scene characters") },
                              { ScreenplayParagraphType::Action, tr("Action") },
                              { ScreenplayParagraphType::Character, tr("Character") },
                              { ScreenplayParagraphType::Parenthetical, tr("Parenthetical") },
                              { ScreenplayParagraphType::Dialogue, tr("Dialogue") },
                              { ScreenplayParagraphType::Lyrics, tr("Lyrics") },
                              { ScreenplayParagraphType::Shot, tr("Shot") },
                              { ScreenplayParagraphType::Transition, tr("Transition") },
                              { ScreenplayParagraphType::InlineNote, tr("Inline note") },
                              { ScreenplayParagraphType::UnformattedText, tr("Unformatted text") },
                              { ScreenplayParagraphType::FolderHeader, tr("Folder") }};
    d->sidebarTabs->setTabName(kFastFormatTabIndex, tr("Formatting"));
    d->sidebarTabs->setTabName(kCommentsTabIndex, tr("Comments"));
}

void ScreenplayTextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolBarUi();

    d->screenplayText->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().background());
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onBackground());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    d->scalableWrapper->setPalette(palette);
    d->screenplayText->setPalette(palette);
    d->screenplayText->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->screenplayText->completer()->setBackgroundColor(Ui::DesignSystem::color().background());

    d->splitter->setBackgroundColor(Ui::DesignSystem::color().background());

    d->sidebarTabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->sidebarTabs->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
