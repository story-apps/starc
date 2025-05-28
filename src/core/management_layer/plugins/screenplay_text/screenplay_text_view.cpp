#include "screenplay_text_view.h"

#include "text/screenplay_text_edit.h"
#include "text/screenplay_text_edit_shortcuts_manager.h"
#include "text/screenplay_text_edit_toolbar.h"
#include "text/screenplay_text_scrollbar_manager.h"
#include "ui/dictionaries_view.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/ai_assistant/ai_assistant_view.h>
#include <ui/modules/bookmarks/bookmarks_model.h>
#include <ui/modules/bookmarks/bookmarks_view.h>
#include <ui/modules/cards/card_item_parameters_view.h>
#include <ui/modules/comments/comments_model.h>
#include <ui/modules/comments/comments_toolbar.h>
#include <ui/modules/comments/comments_view.h>
#include <ui/modules/fast_format_widget/fast_format_widget.h>
#include <ui/modules/search_toolbar/search_manager.h>
#include <ui/widgets/floating_tool_bar/floating_toolbar_animator.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QRandomGenerator>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

#include <optional>

namespace Ui {

namespace {
const int kTypeDataRole = Qt::UserRole + 100;

enum {
    kFastFormatTabIndex = 0,
    kSceneParametersTabIndex,
    kCommentsTabIndex,
    kAiAssistantTabIndex,
    kBookmarksTabIndex,
    kDictionariesTabIndex,
};

const QString kSettingsKey = "screenplay-text";
const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
const QString kIsBeatsVisibleKey = kSettingsKey + "/is-beats-visible";
const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
const QString kIsAiAssistantEnabledKey = kSettingsKey + "/is-ai-assistant-enabled";
const QString kIsItemIsolationEnabledKey = kSettingsKey + "/is-item-isolation-enabled";
const QString kIsSceneParametersVisibleKey = kSettingsKey + "/is-scene-parameters-visible";
const QString kIsBookmarksListVisibleKey = kSettingsKey + "/is-bookmarks-list-visible";
const QString kIsDictionariesVisibleKey = kSettingsKey + "/is-dictionaries-visible";
const QString kSidebarPanelIndexKey = kSettingsKey + "/sidebar-panel-index";
} // namespace

class ScreenplayTextView::Implementation
{
public:
    explicit Implementation(ScreenplayTextView* _q);

    /**
     * @brief Переконфигурировать представление
     */
    void reconfigureTemplate(bool _withModelReinitialization = true);
    void reconfigureSceneNumbersVisibility();
    void reconfigureDialoguesNumbersVisibility();

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
    void updateToolBarCurrentParagraphTypeName();

    /**
     * @brief Обновить компоновку страницы
     */
    void updateTextEditPageMargins();

    /**
     * @brief Обновить параметры режима автоматических редакторских заметок
     */
    void updateTextEditAutoReviewMode();

    /**
     * @brief Обновить видимость и положение панели инструментов рецензирования
     */
    void updateCommentsToolbar(bool _force = false);

    /**
     * @brief Обновить видимость боковой панели (показана, если показана хотя бы одна из вложенных
     * панелей)
     */
    void updateSideBarVisibility(QWidget* _container);

    /**
     * @brief Отобразить параметры заданной сцены
     */
    void showParametersFor(BusinessLayer::TextModelItem* _item);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment, bool _isRevision, bool _isAddition,
                       bool _isRemoval);


    ScreenplayTextView* q = nullptr;

    //
    // Модели
    //
    QPointer<BusinessLayer::ScreenplayTextModel> model;
    BusinessLayer::TextModelItem* lastSelectedItem = nullptr;
    BusinessLayer::CommentsModel* commentsModel = nullptr;
    BusinessLayer::BookmarksModel* bookmarksModel = nullptr;

    //
    // Редактор сценария
    //
    ScreenplayTextEdit* textEdit = nullptr;
    ScreenplayTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;
    ScreenplayTextScrollBarManager* screenplayTextScrollbarManager = nullptr;
    std::optional<int> pendingCursorPosition;

    //
    // Панели инструментов
    //
    ScreenplayTextEditToolbar* toolbar = nullptr;
    BusinessLayer::SearchManager* searchManager = nullptr;
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
    FastFormatWidget* fastFormatWidget = nullptr;
    CardItemParametersView* itemParametersView = nullptr;
    CommentsView* commentsView = nullptr;
    AiAssistantView* aiAssistantView = nullptr;
    BookmarksView* bookmarksView = nullptr;
    DictionariesView* dictionariesView = nullptr;
    //
    Splitter* splitter = nullptr;

    //
    // Действия опций редактора
    //
    QAction* showSceneParametersAction = nullptr;
    QAction* showBookmarksAction = nullptr;
    QAction* showDictionariesAction = nullptr;

    /**
     * @brief Группируем события об изменении положения курсора, чтобы сильно не спамить сервер
     */
    Debouncer cursorChangeNotificationsDebounser;
};

ScreenplayTextView::Implementation::Implementation(ScreenplayTextView* _q)
    : q(_q)
    , commentsModel(new BusinessLayer::CommentsModel(_q))
    , bookmarksModel(new BusinessLayer::BookmarksModel(_q))
    , textEdit(new ScreenplayTextEdit(_q))
    , shortcutsManager(textEdit)
    , scalableWrapper(new ScalableWrapper(textEdit, _q))
    , toolbar(new ScreenplayTextEditToolbar(scalableWrapper))
    , searchManager(new BusinessLayer::SearchManager(scalableWrapper, textEdit))
    , toolbarAnimation(new FloatingToolbarAnimator(_q))
    , paragraphTypesModel(new QStandardItemModel(toolbar))
    , commentsToolbar(new CommentsToolbar(_q))
    , sidebarShadow(new Shadow(Qt::RightEdge, scalableWrapper))
    , sidebarWidget(new Widget(_q))
    , sidebarTabs(new TabBar(_q))
    , sidebarContent(new StackWidget(_q))
    , fastFormatWidget(new FastFormatWidget(_q))
    , itemParametersView(new CardItemParametersView(_q))
    , commentsView(new CommentsView(_q))
    , aiAssistantView(new AiAssistantView(_q))
    , bookmarksView(new BookmarksView(_q))
    , dictionariesView(new DictionariesView(_q))
    , splitter(new Splitter(_q))
    , showSceneParametersAction(new QAction(_q))
    , showBookmarksAction(new QAction(_q))
    , showDictionariesAction(new QAction(_q))
    , cursorChangeNotificationsDebounser(500)

{
    toolbar->setParagraphTypesModel(paragraphTypesModel);

    commentsToolbar->hide();

    textEdit->setVerticalScrollBar(new ScrollBar);
    textEdit->verticalScrollBar()->setObjectName("screenplay-vertical-scroll-bar");
    textEdit->setHorizontalScrollBar(new ScrollBar);
    shortcutsManager.setShortcutsContext(scalableWrapper);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();
    screenplayTextScrollbarManager = new ScreenplayTextScrollBarManager(scalableWrapper);
    screenplayTextScrollbarManager->initScrollBarsSyncing();

    textEdit->setUsePageMode(true);

    sidebarWidget->hide();
    sidebarTabs->setFixed(false);
    sidebarTabs->addTab({}); // fastformat
    sidebarTabs->setTabVisible(kFastFormatTabIndex, false);
    sidebarTabs->addTab({}); // scene parameters
    sidebarTabs->setTabVisible(kSceneParametersTabIndex, false);
    sidebarTabs->addTab({}); // comments
    sidebarTabs->setTabVisible(kCommentsTabIndex, false);
    sidebarTabs->addTab({}); // ai assistant
    sidebarTabs->setTabVisible(kAiAssistantTabIndex, false);
    sidebarTabs->addTab({}); // bookmarks
    sidebarTabs->setTabVisible(kBookmarksTabIndex, false);
    sidebarTabs->addTab({}); // dictionaries
    sidebarTabs->setTabVisible(kDictionariesTabIndex, false);
    sidebarContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidebarContent->setAnimationType(StackWidget::AnimationType::Slide);
    sidebarContent->addWidget(fastFormatWidget);
    sidebarContent->addWidget(itemParametersView);
    sidebarContent->addWidget(commentsView);
    sidebarContent->addWidget(aiAssistantView);
    sidebarContent->addWidget(bookmarksView);
    sidebarContent->addWidget(dictionariesView);
    fastFormatWidget->hide();
    fastFormatWidget->setParagraphTypesModel(paragraphTypesModel);
    itemParametersView->setStartDateTimeVisible(false);
    itemParametersView->setStampVisible(false);
    itemParametersView->hide();
    commentsView->setModel(commentsModel);
    commentsView->hide();
    aiAssistantView->hide();
    aiAssistantView->setSynopsisGenerationAvaiable(true);
    aiAssistantView->setNovelGenerationAvaiable(true);
    bookmarksView->setModel(bookmarksModel);
    bookmarksView->hide();
    dictionariesView->hide();

    showSceneParametersAction->setCheckable(true);
    showSceneParametersAction->setIconText(u8"\U000F1A7D");
    showSceneParametersAction->setSeparator(true);
    showBookmarksAction->setCheckable(true);
    showBookmarksAction->setIconText(u8"\U000F0E16");
    showDictionariesAction->setCheckable(true);
    showDictionariesAction->setIconText(u8"\U000F0EBF");
}

void ScreenplayTextView::Implementation::reconfigureTemplate(bool _withModelReinitialization)
{
    using namespace BusinessLayer;

    paragraphTypesModel->clear();

    //
    // Настраиваем список доступных для работы типов блоков
    //
    QVector<TextParagraphType> types = {
        TextParagraphType::SceneHeading,
        TextParagraphType::SceneCharacters,
        TextParagraphType::BeatHeading,
        TextParagraphType::Action,
        TextParagraphType::Character,
        TextParagraphType::Parenthetical,
        TextParagraphType::Dialogue,
        TextParagraphType::Lyrics,
        TextParagraphType::Shot,
        TextParagraphType::Transition,
        TextParagraphType::InlineNote,
        TextParagraphType::UnformattedText,
        TextParagraphType::SequenceHeading,
        TextParagraphType::SequenceFooter,
        TextParagraphType::ActHeading,
        TextParagraphType::ActFooter,
    };
    if (!toolbar->isBeatsVisible()) {
        types.removeOne(TextParagraphType::BeatHeading);
    }

    //
    // Настраиваем фильтры моделей
    //
    commentsModel->setParagraphTypesFiler(types);
    bookmarksModel->setParagraphTypesFiler(types);

    //
    // Убираем типы окончаний, для списка форматов редактора текста
    //
    types.removeOne(TextParagraphType::SequenceFooter);
    types.removeOne(TextParagraphType::ActFooter);
    const auto& usedTemplate = BusinessLayer::TemplatesFacade::screenplayTemplate(
        model && model->informationModel() ? model->informationModel()->templateId() : "");
    for (const auto type : std::as_const(types)) {
        if (!usedTemplate.paragraphStyle(type).isActive()) {
            continue;
        }

        auto typeItem = new QStandardItem(toDisplayString(type));
        typeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        typeItem->setData(shortcutsManager.shortcut(type), Qt::ToolTipRole);
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        paragraphTypesModel->appendRow(typeItem);
    }

    shortcutsManager.reconfigure();

    if (_withModelReinitialization) {
        textEdit->reinit();
    }
}

void ScreenplayTextView::Implementation::reconfigureSceneNumbersVisibility()
{
    if (model && model->informationModel()) {
        textEdit->setShowSceneNumber(model->informationModel()->showSceneNumbers(),
                                     model->informationModel()->showSceneNumbersOnLeft(),
                                     model->informationModel()->showSceneNumbersOnRight());
    } else {
        textEdit->setShowSceneNumber(
            settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey)
                .toBool(),
            settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
                .toBool(),
            settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
                .toBool());
    }
}

void ScreenplayTextView::Implementation::reconfigureDialoguesNumbersVisibility()
{
    textEdit->setShowDialogueNumber(
        model && model->informationModel()
            ? model->informationModel()->showDialoguesNumbers()
            : settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
                  .toBool());
}

void ScreenplayTextView::Implementation::updateOptionsTranslations()
{
    showSceneParametersAction->setText(showSceneParametersAction->isChecked()
                                           ? tr("Hide scene parameters")
                                           : tr("Show scene parameters"));
    showBookmarksAction->setText(showBookmarksAction->isChecked() ? tr("Hide bookmarks list")
                                                                  : tr("Show bookmarks list"));
    showDictionariesAction->setText(showDictionariesAction->isChecked()
                                        ? tr("Hide screenplay dictionaries")
                                        : tr("Show screenplay dictionaries"));
}

void ScreenplayTextView::Implementation::updateToolbarUi()
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

void ScreenplayTextView::Implementation::updateToolbarPositon()
{
    toolbar->move(QPointF((scalableWrapper->width() - toolbar->width()) / 2.0,
                          -Ui::DesignSystem::card().shadowMargins().top())
                      .toPoint());
    searchManager->toolbar()->move(
        QPointF((scalableWrapper->width() - searchManager->toolbar()->width()) / 2.0,
                -Ui::DesignSystem::card().shadowMargins().top())
            .toPoint());
}

void ScreenplayTextView::Implementation::updateToolBarCurrentParagraphTypeName()
{
    auto paragraphType = textEdit->currentParagraphType();
    if (currentParagraphType == paragraphType) {
        return;
    }

    currentParagraphType = paragraphType;

    if (paragraphType == BusinessLayer::TextParagraphType::ActFooter) {
        paragraphType = BusinessLayer::TextParagraphType::ActHeading;
        toolbar->setParagraphTypesEnabled(false);
        fastFormatWidget->setEnabled(false);
    } else if (paragraphType == BusinessLayer::TextParagraphType::SequenceFooter) {
        paragraphType = BusinessLayer::TextParagraphType::SequenceHeading;
        toolbar->setParagraphTypesEnabled(false);
        fastFormatWidget->setEnabled(false);
    } else {
        toolbar->setParagraphTypesEnabled(!textEdit->isReadOnly() && true);
        fastFormatWidget->setEnabled(!textEdit->isReadOnly() && true);
    }

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

void ScreenplayTextView::Implementation::updateTextEditPageMargins()
{
    if (textEdit->usePageMode()) {
        return;
    }

    const QMarginsF pageMargins
        = QMarginsF{ 15, 20 / scalableWrapper->zoomRange(), 12 / scalableWrapper->zoomRange(), 5 };
    textEdit->setPageMarginsMm(pageMargins);
}

void ScreenplayTextView::Implementation::updateTextEditAutoReviewMode()
{
    switch (commentsToolbar->commentsType()) {
    case Ui::CommentsToolbar::CommentsType::Review: {
        textEdit->setAutoReviewModeEnabled(false);
        break;
    }

    case Ui::CommentsToolbar::CommentsType::Changes: {
        textEdit->setAutoReviewModeEnabled(toolbar->isCommentsModeEnabled() && true);
        textEdit->setAutoReviewMode({}, commentsToolbar->color(), false, true);
        break;
    }

    case Ui::CommentsToolbar::CommentsType::Revision: {
        textEdit->setAutoReviewModeEnabled(toolbar->isCommentsModeEnabled() && true);
        textEdit->setAutoReviewMode(commentsToolbar->color(), {}, true, false);
        break;
    }
    }
}

void ScreenplayTextView::Implementation::updateCommentsToolbar(bool _force)
{
    if (!q->isVisible()) {
        return;
    }

    if (commentsView->isReadOnly() || !toolbar->isCommentsModeEnabled()) {
        commentsToolbar->hideToolbar();
        return;
    }

    //
    // Настроим список доступных действий панели рецензирования
    //
    if (!textEdit->textCursor().hasSelection() && commentsView->currentIndex().isValid()) {
        commentsToolbar->setMode(CommentsToolbar::Mode::EditReview);
        const auto currentIndex = commentsView->currentIndex();
        commentsToolbar->setCurrentCommentState(
            currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsDoneRole).toBool(),
            currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsAdditionRole).toBool()
                || currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsRemovalRole)
                       .toBool(),
            currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsRevisionRole).toBool());
    } else {
        commentsToolbar->setMode(CommentsToolbar::Mode::AddReview);
    }

    //
    // Настроим доступность действий добавления редакторских заметок
    //
    commentsToolbar->setAddingAvailable(textEdit->textCursor().hasSelection());

    const auto cursorRect = textEdit->cursorRect();
    const auto globalCursorCenter = textEdit->mapToGlobal(cursorRect.center());
    const auto localCursorCenter
        = commentsToolbar->parentWidget()->mapFromGlobal(globalCursorCenter);
    //
    // ... если курсор не виден на экране, то тулбар нужно скрыть
    //
    const bool isToolbarVisible = localCursorCenter.y() >= 0
        && localCursorCenter.y()
            < scalableWrapper->height() - screenplayTextScrollbarManager->scrollBarHeight();

    //
    // Определеим положение тулбара, с учётом края экрана
    //
    auto toolbarYPos = localCursorCenter.y() - commentsToolbar->width();
    if (toolbarYPos + commentsToolbar->height()
        > scalableWrapper->height() - screenplayTextScrollbarManager->scrollBarHeight()) {
        toolbarYPos = scalableWrapper->height() - commentsToolbar->height()
            - screenplayTextScrollbarManager->scrollBarHeight();
    }

    //
    // Если вьюпорт вмещается аккурат в видимую область, или не влезает,
    //
    if (textEdit->width() - textEdit->verticalScrollBar()->width()
        <= textEdit->viewport()->width() + commentsToolbar->width()) {
        commentsToolbar->setCurtain(true, q->isLeftToRight() ? Qt::RightEdge : Qt::LeftEdge);
        //
        // ... то позиционируем панель рецензирования по краю панели комментариев
        //
        commentsToolbar->moveToolbar(
            QPoint(q->isLeftToRight() ? (scalableWrapper->width() - commentsToolbar->width()
                                         + DesignSystem::layout().px(3))
                                      : (sidebarWidget->width() - DesignSystem::layout().px(3)),
                   toolbarYPos),
            _force);
    }
    //
    // В противном случае позиционируем её по краю листа
    //
    else {
        commentsToolbar->setCurtain(true, q->isLeftToRight() ? Qt::LeftEdge : Qt::RightEdge);
        //
        // ... определяем точку на границе страницы
        //
        const auto textEditWidth = scalableWrapper->zoomRange() * textEdit->width();
        const auto textEditViewportWidth
            = scalableWrapper->zoomRange() * textEdit->viewport()->width();
        const auto pos = q->isLeftToRight()
            ? ((textEditWidth - textEditViewportWidth) / 2.0 + textEditViewportWidth
               - (scalableWrapper->zoomRange()
                      * (DesignSystem::card().shadowMargins().left()
                         + DesignSystem::card().shadowMargins().right()
                         - DesignSystem::layout().px8())
                  + DesignSystem::floatingToolBar().shadowMargins().left()))
            : ((textEditWidth - textEditViewportWidth) / 2.0 + sidebarWidget->width()
               - (scalableWrapper->zoomRange()
                      * (DesignSystem::card().shadowMargins().left()
                         + DesignSystem::card().shadowMargins().right()
                         - DesignSystem::layout().px8())
                  + DesignSystem::floatingToolBar().shadowMargins().left()));
        //
        // ... и смещаем панель рецензирования к этой точке
        //
        commentsToolbar->moveToolbar(QPoint(pos, toolbarYPos), _force);
    }

    //
    // Если панель ещё не была показана, отобразим её
    //
    if (isToolbarVisible) {
        commentsToolbar->showToolbar();
    } else {
        commentsToolbar->hideToolbar();
    }
}

void ScreenplayTextView::Implementation::updateSideBarVisibility(QWidget* _container)
{
    const bool isSidebarShouldBeVisible = toolbar->isFastFormatPanelVisible()
        || toolbar->isCommentsModeEnabled() || toolbar->isAiAssistantEnabled()
        || showSceneParametersAction->isChecked() || showBookmarksAction->isChecked()
        || showDictionariesAction->isChecked();
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

void ScreenplayTextView::Implementation::showParametersFor(BusinessLayer::TextModelItem* _item)
{
    if (_item == nullptr
        || (_item->type() != BusinessLayer::TextModelItemType::Folder
            && _item->type() != BusinessLayer::TextModelItemType::Group)) {
        return;
    }

    //
    // На время установки данных о другом элемента, блокируем сигналы сайдбара
    //
    QSignalBlocker signalBlocker(itemParametersView);

    lastSelectedItem = _item;

    switch (_item->type()) {
    case BusinessLayer::TextModelItemType::Folder: {
        itemParametersView->setItemType(Ui::CardItemType::Folder);

        auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(lastSelectedItem);
        itemParametersView->setColor(folderItem->color());
        itemParametersView->setTitle(folderItem->heading());
        itemParametersView->setDescription(folderItem->description());
        itemParametersView->setStamp(folderItem->stamp());
        break;
    }

    case BusinessLayer::TextModelItemType::Group: {
        const auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(lastSelectedItem);
        if (groupItem->groupType() != BusinessLayer::TextGroupType::Scene) {
            return;
        }

        itemParametersView->setItemType(Ui::CardItemType::Scene);

        const auto sceneItem = static_cast<BusinessLayer::ScreenplayTextModelSceneItem*>(groupItem);
        itemParametersView->setColor(sceneItem->color());
        itemParametersView->setTitle(sceneItem->title());
        itemParametersView->setHeading(sceneItem->heading());
        itemParametersView->setBeats(sceneItem->beats());
        itemParametersView->setStoryDay(sceneItem->storyDay(),
                                        model->dictionariesModel()->storyDays());
        itemParametersView->setStamp(sceneItem->stamp());
        if (const auto sceneNumber = sceneItem->number(); sceneNumber.has_value()) {
            itemParametersView->setNumber(sceneNumber->followNumber + sceneNumber->value,
                                          sceneNumber->isCustom, sceneNumber->isEatNumber,
                                          sceneNumber->isLocked);
        } else {
            itemParametersView->setNumber({}, false, true, false);
        }
        itemParametersView->setTags(sceneItem->tags(), model->dictionariesModel()->tags());
        break;
    }

    default: {
        break;
        ;
    }
    }
}

void ScreenplayTextView::Implementation::addReviewMark(const QColor& _textColor,
                                                       const QColor& _backgroundColor,
                                                       const QString& _comment, bool _isRevision,
                                                       bool _isAddition, bool _isRemoval)
{
    //
    // Добавим заметку
    //
    const auto textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    textEdit->addReviewMark(textColor, _backgroundColor, _comment, _isRevision, _isAddition,
                            _isRemoval);

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
    cursor.setPosition(selectionInterval.from);
    textEdit->setTextCursorAndKeepScrollBars(cursor);
    cursor.setPosition(selectionInterval.to);
    textEdit->setTextCursorAndKeepScrollBars(cursor);

    //
    // Фокусируем редактор сценария, чтобы пользователь мог продолжать работать с ним
    //
    scalableWrapper->setFocus();
}


// ****


ScreenplayTextView::ScreenplayTextView(QWidget* _parent)
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

    connect(d->toolbar, &ScreenplayTextEditToolbar::undoPressed, d->textEdit,
            &ScreenplayTextEdit::undo);
    connect(d->toolbar, &ScreenplayTextEditToolbar::redoPressed, d->textEdit,
            &ScreenplayTextEdit::redo);
    connect(d->toolbar, &ScreenplayTextEditToolbar::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::fastFormatPanelVisibleChanged, this,
            [this](bool _visible) {
                d->sidebarTabs->setTabVisible(kFastFormatTabIndex, _visible);
                d->fastFormatWidget->setVisible(_visible);
                if (_visible) {
                    d->sidebarTabs->setCurrentTab(kFastFormatTabIndex);
                    d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
                }
                d->updateSideBarVisibility(this);
                d->updateToolbarPositon();
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::beatsVisibleChanged, this,
            [this](bool _visible) {
                d->textEdit->setBeatsVisible(_visible);
                const bool withModelReinitialization = false;
                d->reconfigureTemplate(withModelReinitialization);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::commentsModeEnabledChanged, this,
            [this](bool _enabled) {
                d->sidebarTabs->setTabVisible(kCommentsTabIndex, _enabled);
                d->commentsView->setVisible(_enabled);
                if (_enabled) {
                    d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                    d->sidebarContent->setCurrentWidget(d->commentsView);
                }
                d->updateTextEditAutoReviewMode();
                d->updateCommentsToolbar();
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::aiAssistantEnabledChanged, this,
            [this](bool _enabled) {
                d->sidebarTabs->setTabVisible(kAiAssistantTabIndex, _enabled);
                d->aiAssistantView->setVisible(_enabled);
                if (_enabled) {
                    d->sidebarTabs->setCurrentTab(kAiAssistantTabIndex);
                    d->sidebarContent->setCurrentWidget(d->aiAssistantView);
                }
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::itemIsolationEnabledChanged, this,
            [this](bool _enabled) {
                d->textEdit->setVisibleTopLevelItemIndex(_enabled ? d->textEdit->currentModelIndex()
                                                                  : QModelIndex());

                const bool animate = false;
                d->screenplayTextScrollbarManager->setScrollBarVisible(!_enabled, animate);
                d->textEdit->ensureCursorVisible(d->textEdit->textCursor(), animate);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::searchPressed, this, [this] {
        d->toolbarAnimation->switchToolbars(d->toolbar->searchIcon(),
                                            d->toolbar->searchIconPosition(), d->toolbar,
                                            d->searchManager->toolbar());
        d->searchManager->activateSearhToolbar();
    });
    //
    connect(d->searchManager, &BusinessLayer::SearchManager::hideToolbarRequested, this,
            [this] { d->toolbarAnimation->switchToolbarsBack(); });
    //
    connect(d->commentsToolbar, &CommentsToolbar::commentsTypeChanged, this,
            [this] { d->updateTextEditAutoReviewMode(); });
    connect(d->commentsToolbar, &CommentsToolbar::colorChanged, this,
            [this] { d->updateTextEditAutoReviewMode(); });
    connect(
        d->commentsToolbar, &CommentsToolbar::textColorChangeRequested, this,
        [this](const QColor& _color) { d->addReviewMark(_color, {}, {}, false, false, false); });
    connect(
        d->commentsToolbar, &CommentsToolbar::textBackgoundColorChangeRequested, this,
        [this](const QColor& _color) { d->addReviewMark({}, _color, {}, false, false, false); });
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
    connect(d->commentsToolbar, &CommentsToolbar::changeAdditionAddRequested, this,
            [this](const QColor& _color) { d->addReviewMark({}, _color, {}, false, true, false); });
    connect(d->commentsToolbar, &CommentsToolbar::changeRemovalAddRequested, this,
            [this](const QColor& _color) { d->addReviewMark({}, _color, {}, false, false, true); });
    connect(d->commentsToolbar, &CommentsToolbar::revisionMarkAddRequested, this,
            [this](const QColor& _color) { d->addReviewMark(_color, {}, {}, true, false, false); });
    connect(d->commentsToolbar, &CommentsToolbar::markAsDoneRequested, this, [this](bool _checked) {
        QSignalBlocker blocker(d->commentsView);
        const auto commentIndex = d->commentsView->currentIndex();
        if (d->commentsModel->isChange(commentIndex)) {
            d->commentsModel->applyChanges({ commentIndex });
        } else {
            if (_checked) {
                d->commentsModel->markAsDone({ commentIndex });
            } else {
                d->commentsModel->markAsUndone({ commentIndex });
            }
        }
    });
    connect(d->commentsToolbar, &CommentsToolbar::removeRequested, this, [this] {
        QSignalBlocker blocker(d->commentsView);
        const auto commentIndex = d->commentsView->currentIndex();
        if (d->commentsModel->isChange(commentIndex)) {
            d->commentsModel->cancelChanges({ commentIndex });
        } else {
            d->commentsModel->remove({ d->commentsView->currentIndex() });
        }
        d->commentsToolbar->setMode(CommentsToolbar::Mode::AddReview);
    });
    //
    connect(d->scalableWrapper->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolbar(true); });
    connect(d->scalableWrapper->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolbar(true); });
    connect(
        d->scalableWrapper, &ScalableWrapper::zoomRangeChanged, this,
        [this] {
            d->updateTextEditPageMargins();
            d->updateCommentsToolbar();
        },
        Qt::QueuedConnection);
    //
    auto findCurrentModelItem = [this]() -> BusinessLayer::TextModelItem* {
        if (d->model.isNull()) {
            return nullptr;
        }

        const auto currentModelIndex = this->currentModelIndex();
        if (!currentModelIndex.isValid()) {
            return nullptr;
        }

        auto currentItem = d->model->itemForIndex(currentModelIndex.parent());
        if (currentItem->type() == BusinessLayer::TextModelItemType::Folder
            && static_cast<BusinessLayer::TextFolderType>(currentItem->subtype())
                == BusinessLayer::TextFolderType::Root) {
            return nullptr;
        }

        if (currentItem->type() == BusinessLayer::TextModelItemType::Group
            && static_cast<BusinessLayer::TextGroupType>(currentItem->subtype())
                == BusinessLayer::TextGroupType::Beat) {
            currentItem = currentItem->parent();
        }
        return currentItem;
    };
    auto handleCursorPositionChanged = [this, findCurrentModelItem] {
        //
        // Обновим состояние панелей форматов
        //
        d->updateToolBarCurrentParagraphTypeName();

        //
        // Уведомим навигатор клиентов, о смене текущего элемента
        //
        const auto screenplayModelIndex = d->textEdit->currentModelIndex();
        if (hasFocus() || d->searchManager->toolbar()->hasFocus()) {
            emit currentModelIndexChanged(screenplayModelIndex);
        }

        //
        // Отобразим параметры сцены
        //
        auto currentItem = findCurrentModelItem();
        if (currentItem != nullptr && currentItem != d->lastSelectedItem) {
            d->showParametersFor(currentItem);
        }

        //
        // Если необходимо выберем соответствующий комментарий
        //
        const auto positionInBlock = d->textEdit->textCursor().positionInBlock();
        const auto commentModelIndex
            = d->commentsModel->mapFromModel(screenplayModelIndex, positionInBlock);
        d->commentsView->setCurrentIndex(commentModelIndex);

        //
        // После того, как комментарий был выбран, скорректируем состояние панели рецензирования
        //
        d->updateCommentsToolbar();

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
    connect(d->textEdit, &ScreenplayTextEdit::paragraphTypeChanged, this,
            handleCursorPositionChanged);
    connect(d->textEdit, &ScreenplayTextEdit::cursorPositionChanged, this,
            handleCursorPositionChanged);
    connect(d->textEdit, &ScreenplayTextEdit::selectionChanged, this, handleCursorPositionChanged);
    connect(d->textEdit, &ScreenplayTextEdit::addBookmarkRequested, this, [this] {
        //
        // Если список закладок показан, добавляем новую через него
        //
        if (d->showBookmarksAction->isChecked()) {
            d->sidebarTabs->setCurrentTab(kBookmarksTabIndex);
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
    connect(d->textEdit, &ScreenplayTextEdit::editBookmarkRequested, this, [this] {
        //
        // Если список закладок показан, редактируем через него
        //
        if (d->showBookmarksAction->isChecked()) {
            d->sidebarTabs->setCurrentTab(kBookmarksTabIndex);
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
    connect(d->textEdit, &ScreenplayTextEdit::removeBookmarkRequested, this,
            &ScreenplayTextView::removeBookmarkRequested);
    connect(d->textEdit, &ScreenplayTextEdit::showBookmarksRequested, d->showBookmarksAction,
            &QAction::toggle);
    //
    connect(d->sidebarTabs, &TabBar::currentIndexChanged, this, [this](int _currentIndex) {
        switch (_currentIndex) {
        case kFastFormatTabIndex: {
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
            break;
        }

        case kSceneParametersTabIndex: {
            d->sidebarContent->setCurrentWidget(d->itemParametersView);
            break;
        }

        case kCommentsTabIndex: {
            d->sidebarContent->setCurrentWidget(d->commentsView);
            break;
        }

        case kAiAssistantTabIndex: {
            d->sidebarContent->setCurrentWidget(d->aiAssistantView);
            break;
        }

        case kBookmarksTabIndex: {
            d->sidebarContent->setCurrentWidget(d->bookmarksView);
            break;
        }

        case kDictionariesTabIndex: {
            d->sidebarContent->setCurrentWidget(d->dictionariesView);
            break;
        }
        }
    });
    //
    connect(d->fastFormatWidget, &FastFormatWidget::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    //
    connect(d->itemParametersView, &CardItemParametersView::colorChanged, this,
            [this, findCurrentModelItem](const QColor& _color) {
                auto item = findCurrentModelItem();
                if (item == nullptr) {
                    return;
                }

                switch (item->type()) {
                case BusinessLayer::TextModelItemType::Folder: {
                    auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(item);
                    folderItem->setColor(_color);
                    break;
                }

                case BusinessLayer::TextModelItemType::Group: {
                    auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
                    groupItem->setColor(_color);
                    break;
                }

                default: {
                    Q_ASSERT(false);
                }
                }

                d->model->updateItem(item);
            });
    connect(d->itemParametersView, &CardItemParametersView::titleChanged, this,
            [this, findCurrentModelItem](const QString& _title) {
                auto item = findCurrentModelItem();
                if (item == nullptr) {
                    return;
                }

                switch (item->type()) {
                case BusinessLayer::TextModelItemType::Folder: {
                    auto textItem
                        = static_cast<BusinessLayer::TextModelTextItem*>(item->childAt(0));
                    textItem->setText(_title);
                    item = textItem;
                    break;
                }

                case BusinessLayer::TextModelItemType::Group: {
                    auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
                    groupItem->setTitle(_title);
                    break;
                }

                default: {
                    Q_ASSERT(false);
                }
                }

                d->model->updateItem(item);
            });
    connect(d->itemParametersView, &CardItemParametersView::headingChanged, this,
            [this, findCurrentModelItem](const QString& _heading) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item->childAt(0));
                textItem->setText(_heading);
                d->model->updateItem(textItem);
            });
    connect(d->itemParametersView, &CardItemParametersView::descriptionChanged, this,
            [this, findCurrentModelItem](const QString& _description) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Folder) {
                    return;
                }

                auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(item);
                folderItem->setDescription(_description);
                d->model->updateItem(folderItem);
            });
    connect(d->itemParametersView, &CardItemParametersView::beatAdded, this,
            [this, findCurrentModelItem](int _beatIndex) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                //
                // Определим элемент бита, после которого нужно вставить новый
                //
                int currentBeatIndex = 0;
                for (int childIndex = 1; childIndex < item->childCount(); ++childIndex) {
                    auto childItem = item->childAt(childIndex);
                    if (childItem->type() != BusinessLayer::TextModelItemType::Group) {
                        continue;
                    }

                    if (currentBeatIndex != _beatIndex - 1) {
                        ++currentBeatIndex;
                        continue;
                    }

                    //
                    // ... и вставляем новый бит после обнаруженного
                    //
                    auto beatHeadingItem = d->model->createTextItem();
                    beatHeadingItem->setParagraphType(
                        BusinessLayer::TextParagraphType::BeatHeading);
                    auto beatItem = d->model->createGroupItem(BusinessLayer::TextGroupType::Beat);
                    beatItem->appendItems({ beatHeadingItem });
                    d->model->insertItem(beatItem, childItem);
                    break;
                }
            });
    connect(
        d->itemParametersView, &CardItemParametersView::beatChanged, this,
        [this, findCurrentModelItem](int _beatIndex, const QString& _beat) {
            auto item = findCurrentModelItem();
            if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                return;
            }

            //
            // Определим элемент изменяемого бита
            //
            int currentBeatIndex = 0;
            BusinessLayer::TextModelTextItem* beatHeadingItem = nullptr;
            for (int childIndex = 1; childIndex < item->childCount(); ++childIndex) {
                auto child = item->childAt(childIndex);
                if (child->type() != BusinessLayer::TextModelItemType::Group) {
                    continue;
                }

                if (currentBeatIndex != _beatIndex) {
                    ++currentBeatIndex;
                    continue;
                }

                beatHeadingItem = static_cast<BusinessLayer::TextModelTextItem*>(child->childAt(0));
                break;
            }
            //
            // Если не удалось найти бит (обычно это происходит в ситуации когда не было ни
            // одного бита в сцене, и пользователь добавляет описание на карточку
            //
            if (beatHeadingItem == nullptr) {
                beatHeadingItem = d->model->createTextItem();
                beatHeadingItem->setParagraphType(BusinessLayer::TextParagraphType::BeatHeading);
                auto beatItem = d->model->createGroupItem(BusinessLayer::TextGroupType::Beat);
                beatItem->appendItems({ beatHeadingItem });
                d->model->appendItem(beatItem, item);
            }
            //
            // Обновляем текст заголовка бита
            //
            beatHeadingItem->setText(_beat);
            d->model->updateItem(beatHeadingItem);
        });
    connect(d->itemParametersView, &CardItemParametersView::beatRemoved, this,
            [this, findCurrentModelItem](int _beatIndex) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                //
                // Определим элемент бита, который нужно удалить
                //
                int currentBeatIndex = 0;
                for (int childIndex = 1; childIndex < item->childCount(); ++childIndex) {
                    auto beatItem = item->childAt(childIndex);
                    if (beatItem->type() != BusinessLayer::TextModelItemType::Group) {
                        continue;
                    }

                    if (currentBeatIndex != _beatIndex) {
                        ++currentBeatIndex;
                        continue;
                    }

                    //
                    // ... извлечём всех детей и перенесём их по назначению
                    //
                    if (beatItem->hasChildren() && beatItem->childCount() > 1) {
                        QVector<BusinessLayer::TextModelItem*> beatChildren;
                        while (beatItem->childCount() > 1) {
                            auto beatChildItem = beatItem->childAt(1);
                            d->model->takeItem(beatChildItem);
                            beatChildren.append(beatChildItem);
                        }

                        const int beatItemIndex = beatItem->parent()->rowOfChild(beatItem);
                        if (beatItemIndex == 0) {
                            d->model->prependItems(beatChildren);
                        } else {
                            auto beforeBeatItem = beatItem->parent()->childAt(beatItemIndex - 1);
                            Q_ASSERT(beforeBeatItem);
                            if (beforeBeatItem->type() == BusinessLayer::TextModelItemType::Group) {
                                d->model->appendItems(beatChildren, beforeBeatItem);
                            } else {
                                d->model->insertItems(beatChildren, beforeBeatItem);
                            }
                        }
                    }

                    //
                    // ... и удалим его
                    //
                    d->model->removeItem(beatItem);
                    break;
                }
            });
    connect(d->itemParametersView, &CardItemParametersView::storyDayChanged, this,
            [this, findCurrentModelItem](const QString& _storyDay) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);

                d->model->dictionariesModel()->removeStoryDay(groupItem->storyDay());
                d->model->dictionariesModel()->addStoryDay(_storyDay);
                //
                groupItem->setStoryDay(_storyDay);
                d->model->updateItem(groupItem);
            });
    connect(
        d->itemParametersView, &CardItemParametersView::numberChanged, this,
        [this, findCurrentModelItem](const QString& _number, bool _isCustom, bool _isEatNumber) {
            auto item = findCurrentModelItem();
            if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                return;
            }

            auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
            if (_isCustom) {
                groupItem->setCustomNumber(_number, _isEatNumber);
            } else {
                groupItem->resetNumber();
            }
            d->model->updateItem(groupItem);
            d->model->updateNumbering();
        });
    connect(d->itemParametersView, &CardItemParametersView::tagsChanged, this,
            [this, findCurrentModelItem](const QVector<QPair<QString, QColor>>& _tags) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);

                d->model->dictionariesModel()->removeTags(groupItem->tags());
                d->model->dictionariesModel()->addTags(_tags);

                groupItem->setTags(_tags);
                d->model->updateItem(groupItem);
            });
    //
    connect(d->commentsView, &CommentsView::addReviewMarkRequested, this,
            [this](const QColor& _color, const QString& _comment) {
                d->addReviewMark({}, _color, _comment, false, false, false);
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

                if (d->toolbar->isItemIsolationEnabled()) {
                    d->textEdit->setVisibleTopLevelItemIndex(positionHint.index);
                }

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
    connect(d->commentsView, &CommentsView::applyChangeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->applyChanges(_indexes);
            });
    connect(d->commentsView, &CommentsView::cancelChangeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->cancelChanges(_indexes);
            });
    connect(d->commentsView, &CommentsView::removeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->remove(_indexes);
            });
    //
    connect(d->aiAssistantView, &AiAssistantView::rephraseRequested, this,
            &ScreenplayTextView::rephraseTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::expandRequested, this,
            &ScreenplayTextView::expandTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::shortenRequested, this,
            &ScreenplayTextView::shortenTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::insertRequested, this,
            &ScreenplayTextView::insertTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::summarizeRequested, this,
            &ScreenplayTextView::summarizeTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::translateRequested, this,
            &ScreenplayTextView::translateTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::translateDocumentRequested, this,
            &ScreenplayTextView::translateDocumentRequested);
    connect(d->aiAssistantView, &AiAssistantView::generateSynopsisRequested, this,
            &ScreenplayTextView::generateSynopsisRequested);
    connect(d->aiAssistantView, &AiAssistantView::generateNovelRequested, this,
            &ScreenplayTextView::generateNovelRequested);
    connect(d->aiAssistantView, &AiAssistantView::generateTextRequested, this,
            &ScreenplayTextView::generateTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::insertTextRequested, this,
            [this](const QString& _text) { d->textEdit->insertPlainText(_text); });
    connect(d->aiAssistantView, &AiAssistantView::buyCreditsPressed, this,
            &ScreenplayTextView::buyCreditsRequested);
    //
    connect(d->bookmarksView, &BookmarksView::addBookmarkRequested, this,
            &ScreenplayTextView::createBookmarkRequested);
    connect(d->bookmarksView, &BookmarksView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                emit changeBookmarkRequested(d->bookmarksModel->mapToModel(_index), _text, _color);
            });
    connect(d->bookmarksView, &BookmarksView::bookmarkSelected, this,
            [this](const QModelIndex& _index) {
                const auto index = d->bookmarksModel->mapToModel(_index);

                if (d->toolbar->isItemIsolationEnabled()) {
                    d->textEdit->setVisibleTopLevelItemIndex(index);
                }

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
    connect(d->showSceneParametersAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kSceneParametersTabIndex, _checked);
        d->itemParametersView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kSceneParametersTabIndex);
            d->sidebarContent->setCurrentWidget(d->itemParametersView);
        }
        d->updateSideBarVisibility(this);
    });
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
    connect(d->showDictionariesAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kDictionariesTabIndex, _checked);
        d->dictionariesView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kDictionariesTabIndex);
            d->sidebarContent->setCurrentWidget(d->dictionariesView);
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(&d->cursorChangeNotificationsDebounser, &Debouncer::gotWork, this, [this] {
        emit cursorChanged(QString::number(d->textEdit->textCursor().position()).toUtf8());
    });

    reconfigure({});
}

ScreenplayTextView::~ScreenplayTextView() = default;

QWidget* ScreenplayTextView::asQWidget()
{
    return this;
}

void ScreenplayTextView::toggleFullScreen(bool _isFullScreen)
{
    d->toolbar->setVisible(!_isFullScreen);
    d->screenplayTextScrollbarManager->setScrollBarVisible(!_isFullScreen);
}

QVector<QAction*> ScreenplayTextView::options() const
{
    return {
        d->showSceneParametersAction,
        d->showBookmarksAction,
        d->showDictionariesAction,
    };
}

void ScreenplayTextView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->textEdit->setReadOnly(readOnly);
    d->toolbar->setReadOnly(readOnly);
    d->searchManager->setReadOnly(readOnly);
    d->itemParametersView->setReadOnly(readOnly);
    d->commentsView->setReadOnly(_mode == ManagementLayer::DocumentEditingMode::Read);
    d->aiAssistantView->setReadOnly(_mode == ManagementLayer::DocumentEditingMode::Read);
    d->bookmarksView->setReadOnly(readOnly);
    d->dictionariesView->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->shortcutsManager.setEnabled(enabled);
    d->fastFormatWidget->setEnabled(enabled);
}

void ScreenplayTextView::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->textEdit->setCursors(_cursors);
}

void ScreenplayTextView::setCurrentCursor(const Domain::CursorInfo& _cursor)
{
    setCursorPosition(_cursor.cursorData.toInt());
}

void ScreenplayTextView::setCurrentModelIndex(const QModelIndex& _index)
{
    if (d->toolbar->isItemIsolationEnabled()) {
        d->textEdit->setVisibleTopLevelItemIndex(_index);
    }

    d->textEdit->setCurrentModelIndex(_index);
}

void ScreenplayTextView::setAvailableCredits(int _credits)
{
    d->aiAssistantView->setAvailableWords(_credits);
}

void ScreenplayTextView::setRephrasedText(const QString& _text)
{
    d->aiAssistantView->setRephraseResult(_text);
}

void ScreenplayTextView::setExpandedText(const QString& _text)
{
    d->aiAssistantView->setExpandResult(_text);
}

void ScreenplayTextView::setShortenedText(const QString& _text)
{
    d->aiAssistantView->setShortenResult(_text);
}

void ScreenplayTextView::setInsertedText(const QString& _text)
{
    d->aiAssistantView->setInsertResult(_text);
}

void ScreenplayTextView::setSummarizedText(const QString& _text)
{
    d->aiAssistantView->setSummarizeResult(_text);
}

void ScreenplayTextView::setTranslatedText(const QString& _text)
{
    d->aiAssistantView->setTransateResult(_text);
}

void ScreenplayTextView::setTranslatedDocument(const QVector<QString>& _text)
{
    auto lines = _text;
    std::function<void(const QModelIndex&)> updateLines;
    updateLines = [this, &updateLines, &lines](const QModelIndex& _parentItemIndex) {
        for (int row = 0; row < d->model->rowCount(_parentItemIndex); ++row) {
            const auto itemIndex = d->model->index(row, 0, _parentItemIndex);
            const auto item = d->model->itemForIndex(itemIndex);
            switch (item->type()) {
            case BusinessLayer::TextModelItemType::Folder: {
                updateLines(itemIndex);
                break;
            }

            case BusinessLayer::TextModelItemType::Group: {
                updateLines(itemIndex);
                break;
            }

            case BusinessLayer::TextModelItemType::Text: {
                auto textItem = static_cast<BusinessLayer::ScreenplayTextModelTextItem*>(item);
                if (!textItem->text().isEmpty()) {
                    textItem->setText(lines.takeFirst());
                    textItem->setFormats({});
                    d->model->updateItem(textItem);
                }
                break;
            }

            default: {
                break;
            }
            }
        }
    };
    updateLines({});
}

void ScreenplayTextView::setGeneratedSynopsis(const QString& _text)
{
    d->aiAssistantView->setGenerateSynopsisResult(_text);
}

void ScreenplayTextView::setGeneratedText(const QString& _text)
{
    const QLatin1String textWritingTaskKey("text-writing-task");
    TaskBar::addTask(textWritingTaskKey);
    TaskBar::setTaskTitle(textWritingTaskKey, tr("Writing text"));

    //
    // Отключим отображение всплывающих подсказок
    //
    d->textEdit->setCompleterActive(false);

    //
    // Переходим в конец позицию вставки, а затем переводим его на новую строку, или сдвигаем
    // последующий текст, чтобы помещать текст в новом блоке
    //
    switch (d->aiAssistantView->textInsertPosition()) {
    case AiAssistantView::TextInsertPosition::AtBeginning: {
        d->textEdit->moveCursor(QTextCursor::Start);
        d->textEdit->addParagraph(d->textEdit->currentParagraphType());
        d->textEdit->moveCursor(QTextCursor::Start);
        break;
    }

    case AiAssistantView::TextInsertPosition::AtCursorPosition: {
        d->textEdit->moveCursor(QTextCursor::EndOfBlock);
        d->textEdit->addParagraph(BusinessLayer::TextParagraphType::Action);
        break;
    }

    case AiAssistantView::TextInsertPosition::AtEnd: {
        d->textEdit->moveCursor(QTextCursor::End);
        d->textEdit->addParagraph(BusinessLayer::TextParagraphType::Action);
        break;
    }
    }

    QElapsedTimer timer;
    int progress = 0;
    auto waitForNextOperation = [&timer, &progress, maximum = _text.length(), textWritingTaskKey] {
        timer.restart();
        const auto delay = QRandomGenerator::global()->bounded(10, 60);
        while (!timer.hasExpired(delay)) {
            QCoreApplication::processEvents();
        }

        ++progress;
        TaskBar::setTaskProgress(textWritingTaskKey, progress * 100 / static_cast<qreal>(maximum));
    };

    auto lines = _text.split('\n', Qt::SkipEmptyParts);
    bool nextBlockShoudBeDialogue = false;
    for (auto& line : lines) {
        if (line == TextHelper::smartToUpper(line)) {
            if (line.contains('.') && (line.contains('-') || line.contains("–"))) {
                d->textEdit->setCurrentParagraphType(
                    BusinessLayer::TextParagraphType::SceneHeading);
                nextBlockShoudBeDialogue = false;
            } else if (line == lines.constFirst() || line == lines.constLast()
                       || line.trimmed().endsWith(':')) {
                //
                // TODO: добавить проверку на стандартные переходы и кадры
                //
                d->textEdit->setCurrentParagraphType(BusinessLayer::TextParagraphType::Shot);
                nextBlockShoudBeDialogue = false;
            } else {
                d->textEdit->setCurrentParagraphType(BusinessLayer::TextParagraphType::Character);
                nextBlockShoudBeDialogue = true;
            }
        } else {
            if (line.startsWith('(') && line.endsWith(')')) {
                d->textEdit->setCurrentParagraphType(
                    BusinessLayer::TextParagraphType::Parenthetical);
                nextBlockShoudBeDialogue = true;
            } else if (nextBlockShoudBeDialogue || line.endsWith(':')) {
                d->textEdit->setCurrentParagraphType(BusinessLayer::TextParagraphType::Dialogue);
                nextBlockShoudBeDialogue = false;
                if (line.endsWith(':')) {
                    line.chop(1);
                }
            } else {
                d->textEdit->setCurrentParagraphType(BusinessLayer::TextParagraphType::Action);
                nextBlockShoudBeDialogue = false;
            }
        }

        for (int index = 0; index < line.length(); ++index) {
            QCoreApplication::postEvent(d->textEdit,
                                        new QKeyEvent(QEvent::KeyPress, Qt::Key_unknown,
                                                      Qt::NoModifier, line.mid(index, 1)));
            QCoreApplication::postEvent(d->textEdit,
                                        new QKeyEvent(QEvent::KeyRelease, Qt::Key_unknown,
                                                      Qt::NoModifier, line.mid(index, 1)));
            waitForNextOperation();
        }

        QCoreApplication::postEvent(
            d->textEdit, new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
        QCoreApplication::postEvent(
            d->textEdit, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier));
        waitForNextOperation();
    }

    //
    // Возвращаем возможность использования всплывающих подсказок
    //
    d->textEdit->setCompleterActive(true);

    TaskBar::finishTask(textWritingTaskKey);
}

DictionariesView* ScreenplayTextView::dictionariesView() const
{
    return d->dictionariesView;
}

void ScreenplayTextView::reconfigure(const QStringList& _changedSettingsKeys)
{
    UiHelper::initSpellingFor(d->textEdit);

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey)) {
        d->reconfigureTemplate();
    }

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey)) {
        d->reconfigureSceneNumbersVisibility();
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)) {
        d->reconfigureDialoguesNumbersVisibility();
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey)
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorCorrectTextOnPageBreaksKey)) {
        d->textEdit->setCorrectionOptions(
            settingsValue(DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey)
                .toBool(),
            settingsValue(DataStorageLayer::kComponentsScreenplayEditorCorrectTextOnPageBreaksKey)
                .toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey)) {
        d->textEdit->setShowSuggestionsInEmptyBlocks(
            settingsValue(DataStorageLayer::
                              kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey)
                .toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsScreenplayEditorShortcutsKey)) {
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
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationCorrectDoubleCapitalsKey)) {
        d->textEdit->setCorrectDoubleCapitals(
            settingsValue(DataStorageLayer::kApplicationCorrectDoubleCapitalsKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kApplicationCapitalizeSingleILetterKey)) {
        d->textEdit->setCapitalizeSingleILetter(
            settingsValue(DataStorageLayer::kApplicationCapitalizeSingleILetterKey).toBool());
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
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kApplicationReplaceTwoDashesWithEmDashKey)) {
        d->textEdit->setReplaceTwoDashes(
            settingsValue(DataStorageLayer::kApplicationReplaceTwoDashesWithEmDashKey).toBool());
    }
    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kApplicationAvoidMultipleSpacesKey)) {
        d->textEdit->setAvoidMultipleSpaces(
            settingsValue(DataStorageLayer::kApplicationAvoidMultipleSpacesKey).toBool());
    }
}

void ScreenplayTextView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor = settingsValue(kScaleFactorKey, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);

    const auto isItemIsolationEnabled = settingsValue(kIsItemIsolationEnabledKey, false).toBool();
    d->toolbar->setItemIsolationEnabled(isItemIsolationEnabled);
    const auto isCommentsModeEnabled = settingsValue(kIsCommentsModeEnabledKey, false).toBool();
    d->toolbar->setCommentsModeEnabled(isCommentsModeEnabled);
    const auto isAiAssistantEnabled = settingsValue(kIsAiAssistantEnabledKey, false).toBool();
    d->toolbar->setAiAssistantEnabled(isAiAssistantEnabled);
    const auto isFastFormatPanelVisible
        = settingsValue(kIsFastFormatPanelVisibleKey, false).toBool();
    d->toolbar->setFastFormatPanelVisible(isFastFormatPanelVisible);
    const auto isBeatsVisible = settingsValue(kIsBeatsVisibleKey, false).toBool();
    d->toolbar->setBeatsVisible(isBeatsVisible);
    const auto isSceneParametersVisible
        = settingsValue(kIsSceneParametersVisibleKey, false).toBool();
    d->showSceneParametersAction->setChecked(isSceneParametersVisible);
    const auto isBookmarksListVisible = settingsValue(kIsBookmarksListVisibleKey, false).toBool();
    d->showBookmarksAction->setChecked(isBookmarksListVisible);
    const auto isDictionariesVisible = settingsValue(kIsDictionariesVisibleKey, false).toBool();
    d->showDictionariesAction->setChecked(isDictionariesVisible);
    const auto sidebarPanelIndex = settingsValue(kSidebarPanelIndexKey, 0).toInt();
    d->sidebarTabs->setCurrentTab(sidebarPanelIndex);

    const auto sidebarState = settingsValue(kSidebarStateKey);
    if (sidebarState.isValid()) {
        d->splitter->restoreState(sidebarState.toByteArray());
    }
}

void ScreenplayTextView::saveViewSettings()
{
    setSettingsValue(kScaleFactorKey, d->scalableWrapper->zoomRange());

    setSettingsValue(kIsFastFormatPanelVisibleKey, d->toolbar->isFastFormatPanelVisible());
    setSettingsValue(kIsBeatsVisibleKey, d->toolbar->isBeatsVisible());
    setSettingsValue(kIsCommentsModeEnabledKey, d->toolbar->isCommentsModeEnabled());
    setSettingsValue(kIsAiAssistantEnabledKey, d->toolbar->isAiAssistantEnabled());
    setSettingsValue(kIsItemIsolationEnabledKey, d->toolbar->isItemIsolationEnabled());
    setSettingsValue(kIsSceneParametersVisibleKey, d->showSceneParametersAction->isChecked());
    setSettingsValue(kIsBookmarksListVisibleKey, d->showBookmarksAction->isChecked());
    setSettingsValue(kIsDictionariesVisibleKey, d->showDictionariesAction->isChecked());
    setSettingsValue(kSidebarPanelIndexKey, d->sidebarTabs->currentTab());

    setSettingsValue(kSidebarStateKey, d->splitter->saveState());
}

void ScreenplayTextView::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    if (d->model) {
        d->model->disconnect(this);
        if (d->model->informationModel()) {
            d->model->informationModel()->disconnect(this);
        }
    }

    d->model = _model;

    //
    // Отслеживаем изменения некоторых параметров
    //
    if (d->model && d->model->informationModel()) {
        const bool reinitModel = true;
        d->reconfigureTemplate(!reinitModel);
        d->reconfigureSceneNumbersVisibility();
        d->reconfigureDialoguesNumbersVisibility();

        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::templateIdChanged, this,
                [this] { d->reconfigureTemplate(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::showSceneNumbersChanged, this,
                [this] { d->reconfigureSceneNumbersVisibility(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::showSceneNumbersOnLeftChanged, this,
                [this] { d->reconfigureSceneNumbersVisibility(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::showSceneNumbersOnRightChanged, this,
                [this] { d->reconfigureSceneNumbersVisibility(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::showDialoguesNumbersChanged, this,
                [this] { d->reconfigureDialoguesNumbersVisibility(); });

        connect(d->model, &BusinessLayer::ScreenplayTextModel::dataChanged, this,
                [this](const QModelIndex& _topLeft) {
                    auto updatedItem = d->model->itemForIndex(_topLeft);
                    if (updatedItem != d->lastSelectedItem) {
                        return;
                    }

                    d->showParametersFor(updatedItem);
                });

        //
        // Обновляем стоимость генерации при изменении модели
        //
        auto updateGenerationPrice = [this] {
            d->aiAssistantView->setTranslationDocumentOption(
                tr("Document translation will take %n word(s)", 0, d->model->wordsCount()));
            d->aiAssistantView->setGenerationSynopsisOptions(
                tr("Synopsis generation will take %n word(s)", 0, d->model->wordsCount()));
            d->aiAssistantView->setGenerationNovelOptions(
                tr("Novel generation will take %n word(s)", 0, d->model->wordsCount()));
        };
        connect(d->model, &BusinessLayer::ScreenplayTextModel::modelReset, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::dataChanged, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsInserted, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsMoved, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsRemoved, this,
                updateGenerationPrice);

        //
        // Перед началом сброса документа запоминаем текущую позицию курсора
        //
        connect(d->model, &BusinessLayer::ScreenplayTextModel::modelAboutToBeReset, this, [this] {
            if (!d->pendingCursorPosition.has_value()) {
                d->pendingCursorPosition = cursorPosition();
            }
        });
        //
        // ... после завершения сброса, отложенно возвращаем курсор на место
        //
        connect(
            d->model, &BusinessLayer::ScreenplayTextModel::modelReset, this,
            [this] {
                if (!d->pendingCursorPosition.has_value()) {
                    return;
                }

                //
                // Извлечём позицию для установки
                //
                const int position = d->pendingCursorPosition.value();
                //
                // ... затем сбрасываем буфер, чтобы позиция установилась внутрь редактора текста
                //
                d->pendingCursorPosition.reset();
                //
                // ... устанавливаем позицию в редактор
                //
                setCursorPosition(position);
            },
            Qt::QueuedConnection);
    }

    d->textEdit->setCursors({});
    d->textEdit->initWithModel(d->model);
    d->screenplayTextScrollbarManager->setModel(d->model);
    d->commentsModel->setTextModel(d->model);
    d->bookmarksModel->setTextModel(d->model);

    d->updateToolBarCurrentParagraphTypeName();
}

QModelIndex ScreenplayTextView::currentModelIndex() const
{
    return d->textEdit->currentModelIndex();
}

int ScreenplayTextView::cursorPosition() const
{
    return d->textEdit->textCursor().position();
}

void ScreenplayTextView::setCursorPosition(int _position)
{
    if (d->pendingCursorPosition.has_value()) {
        d->pendingCursorPosition = _position;
        return;
    }

    auto cursor = d->textEdit->textCursor();
    cursor.setPosition(_position);
    d->textEdit->ensureCursorVisible(cursor, false);
}

int ScreenplayTextView::verticalScroll() const
{
    return d->textEdit->verticalScrollBar()->value();
}

void ScreenplayTextView::setVerticalScroll(int _value)
{
    d->textEdit->stopVerticalScrollAnimation();
    d->textEdit->verticalScrollBar()->setValue(_value);
}

bool ScreenplayTextView::eventFilter(QObject* _target, QEvent* _event)
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

void ScreenplayTextView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->updateToolbarPositon();
    d->updateCommentsToolbar();
}

void ScreenplayTextView::updateTranslations()
{
    d->sidebarTabs->setTabName(kFastFormatTabIndex, tr("Formatting"));
    d->sidebarTabs->setTabName(kSceneParametersTabIndex, tr("Scene parameters"));
    d->sidebarTabs->setTabName(kCommentsTabIndex, tr("Comments"));
    d->sidebarTabs->setTabName(kAiAssistantTabIndex, tr("AI assistant"));
    d->sidebarTabs->setTabName(kBookmarksTabIndex, tr("Bookmarks"));
    d->sidebarTabs->setTabName(kDictionariesTabIndex, tr("Dictionaries"));

    d->aiAssistantView->setGenerationPromptHint(
        tr("Start prompt from something like \"Write a screenplay about ...\", or \"Write a short "
           "movie screenplay about ...\""));

    d->updateOptionsTranslations();

    //
    // Обновить список форматов в выпадающем меню
    //
    const auto withModelReinitialization = false;
    d->reconfigureTemplate(withModelReinitialization);
    //
    // ... и текст текущего формата
    //
    d->currentParagraphType = BusinessLayer::TextParagraphType::Undefined;
    d->updateToolBarCurrentParagraphTypeName();

    d->searchManager->setSearchInBlockTypes(
        { { tr("In the whole text"), BusinessLayer::TextParagraphType::Undefined },
          { tr("In scene heading"), BusinessLayer::TextParagraphType::SceneHeading },
          { tr("In cast list"), BusinessLayer::TextParagraphType::SceneCharacters },
          { tr("In action"), BusinessLayer::TextParagraphType::Action },
          { tr("In character"), BusinessLayer::TextParagraphType::Character },
          { tr("In dialogue"), BusinessLayer::TextParagraphType::Dialogue } });
}

void ScreenplayTextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolbarUi();

    d->textEdit->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().textEditor());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onTextEditor());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().accent());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onAccent());
    d->scalableWrapper->setPalette(palette);
    d->textEdit->setPalette(palette);
    palette.setColor(QPalette::Base, Qt::transparent);
    d->textEdit->viewport()->setPalette(palette);
    d->textEdit->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->textEdit->completer()->setBackgroundColor(Ui::DesignSystem::color().background());

    d->splitter->setBackgroundColor(Ui::DesignSystem::color().surface());

    d->sidebarTabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->sidebarTabs->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->sidebarContent->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
