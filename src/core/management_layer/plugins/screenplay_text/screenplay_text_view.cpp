#include "screenplay_text_view.h"

#include "comments/screenplay_text_comments_model.h"
#include "comments/screenplay_text_comments_toolbar.h"
#include "comments/screenplay_text_comments_view.h"
#include "text/screenplay_text_block_data.h"
#include "text/screenplay_text_edit.h"
#include "text/screenplay_text_edit_shortcuts_manager.h"
#include "text/screenplay_text_edit_toolbar.h"
#include "text/screenplay_text_fast_format_widget.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
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

    BusinessLayer::ScreenplayTextCommentsModel *commentsModel = nullptr;

    ScreenplayTextEditToolBar* toolBar = nullptr;
    QHash<BusinessLayer::ScreenplayParagraphType, QString> typesToDisplayNames;
    BusinessLayer::ScreenplayParagraphType currentParagraphType = BusinessLayer::ScreenplayParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;

    ScreenplayTextCommentsToolbar* commentsToolBar = nullptr;

    ScreenplayTextEdit* screenplayText = nullptr;
    ScreenplayTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;

    Widget* sidebarWidget = nullptr;
    TabBar* sidebarTabs = nullptr;
    StackWidget* sidebarContent = nullptr;
    ScreenplayTextFastFormatWidget* fastFormatWidget = nullptr;
    ScreenplayTextCommentsView* commentsView = nullptr;

    Splitter* splitter = nullptr;
};

ScreenplayTextView::Implementation::Implementation(QWidget* _parent)
    : commentsModel(new BusinessLayer::ScreenplayTextCommentsModel(_parent)),
      toolBar(new ScreenplayTextEditToolBar(_parent)),
      paragraphTypesModel(new QStandardItemModel(toolBar)),
      commentsToolBar(new ScreenplayTextCommentsToolbar(_parent)),
      screenplayText(new ScreenplayTextEdit(_parent)),
      shortcutsManager(screenplayText),
      scalableWrapper(new ScalableWrapper(screenplayText, _parent)),
      sidebarWidget(new Widget(_parent)),
      sidebarTabs(new TabBar(_parent)),
      sidebarContent(new StackWidget(_parent)),
      fastFormatWidget(new ScreenplayTextFastFormatWidget(_parent)),
      commentsView(new ScreenplayTextCommentsView(_parent)),
      splitter(new Splitter(_parent))

{
    toolBar->setParagraphTypesModel(paragraphTypesModel);

    commentsToolBar->hide();

    screenplayText->setVerticalScrollBar(new ScrollBar);
    screenplayText->setHorizontalScrollBar(new ScrollBar);
    shortcutsManager.setShortcutsContext(scalableWrapper);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

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
    commentsView->hide();
}

void ScreenplayTextView::Implementation::updateToolBarUi()
{
    toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    toolBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolBar->raise();

    commentsToolBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    commentsToolBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    commentsToolBar->raise();
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
        toolBar->setParagraphTypesEnabled(false);
    } else {
        toolBar->setParagraphTypesEnabled(true);
    }

    for (int itemRow = 0; itemRow < paragraphTypesModel->rowCount(); ++itemRow) {
        const auto item = paragraphTypesModel->item(itemRow);
        const auto itemType = static_cast<BusinessLayer::ScreenplayParagraphType>(item->data(kTypeDataRole).toInt());
        if (itemType == paragraphType) {
            toolBar->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            fastFormatWidget->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            return;
        }
    }
}

void ScreenplayTextView::Implementation::updateCommentsToolBar()
{
    if (!toolBar->isCommentsModeEnabled()
        || !screenplayText->textCursor().hasSelection()) {
        commentsToolBar->hideToolbar();
        return;
    }

    //
    // Определяем точку на границе страницы, либо если страница не влезает в экран, то с боку экрана
    //
    const int x = (screenplayText->width() - screenplayText->viewport()->width()) / 2
                  + screenplayText->viewport()->width()
                  - commentsToolBar->width();
    const qreal textRight = scalableWrapper->mapFromEditor(QPoint(x, 0)).x();
    const auto cursorRect = screenplayText->cursorRect();
    const auto globalCursorCenter = screenplayText->mapToGlobal(cursorRect.center());
    const auto localCursorCenter = commentsToolBar->parentWidget()->mapFromGlobal(globalCursorCenter);
    //
    // И смещаем панель рецензирования к этой точке
    //
    commentsToolBar->moveToolbar(
                QPoint(std::min(scalableWrapper->width()
                                - commentsToolBar->width()
                                - Ui::DesignSystem::layout().px24(),
                                textRight),
                       localCursorCenter.y()
                       - (commentsToolBar->height() / 3)));

    //
    // Если панель ещё не была показана, отобразим её
    //
    commentsToolBar->showToolbar();
}

void ScreenplayTextView::Implementation::updateSideBarVisibility(QWidget* _container)
{
    const bool isSidebarShouldBeVisible = toolBar->isFastFormatPanelVisible()
                                          || toolBar->isCommentsModeEnabled();
    if (sidebarWidget->isVisible() == isSidebarShouldBeVisible) {
        return;
    }

    sidebarWidget->setVisible(isSidebarShouldBeVisible);

    if (isSidebarShouldBeVisible) {
        const auto sideBarWidth = sidebarContent->sizeHint().width();
        splitter->setSizes({ _container->width() - sideBarWidth, sideBarWidth });
    }
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

    d->splitter->addWidget(d->scalableWrapper);
    d->splitter->addWidget(d->sidebarWidget);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);

    connect(d->toolBar, &ScreenplayTextEditToolBar::paragraphTypeChanged, this, [this] (const QModelIndex& _index) {
        const auto type = static_cast<BusinessLayer::ScreenplayParagraphType>(_index.data(kTypeDataRole).toInt());
        d->screenplayText->setCurrentParagraphType(type);
        d->scalableWrapper->setFocus();
    });
    connect(d->toolBar, &ScreenplayTextEditToolBar::fastFormatPanelVisibleChanged, this, [this] (bool _visible) {
        d->sidebarTabs->setTabVisible(kFastFormatTabIndex, _visible);
        d->fastFormatWidget->setVisible(_visible);
        if (_visible) {
            d->sidebarTabs->setCurrentTab(kFastFormatTabIndex);
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
        }
        d->updateSideBarVisibility(this);
    });
    connect(d->toolBar, &ScreenplayTextEditToolBar::commentsModeEnabledChanged, this, [this] (bool _enabled) {
        d->sidebarTabs->setTabVisible(kCommentsTabIndex, _enabled);
        d->commentsView->setVisible(_enabled);
        if (_enabled) {
            d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
            d->sidebarContent->setCurrentWidget(d->commentsView);
            d->updateCommentsToolBar();
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(d->commentsToolBar, &ScreenplayTextCommentsToolbar::textColorChangeRequested,
            this, [this](const QColor& _color) { d->screenplayText->addReviewMark(_color, {}, {}); });
    connect(d->commentsToolBar, &ScreenplayTextCommentsToolbar::textBackgoundColorChangeRequested,
            this, [this](const QColor& _color) { d->screenplayText->addReviewMark({}, _color, {}); });
    connect(d->commentsToolBar, &ScreenplayTextCommentsToolbar::commentAddRequested, this, [this] (const QColor& _color) {
        d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
        d->commentsView->showAddCommentView(_color);
    });
    connect(d->commentsView, &ScreenplayTextCommentsView::addCommentRequested, this, [this] (const QColor& _color, const QString& _comment) {
        d->screenplayText->addReviewMark({}, _color, _comment);
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
    connect(d->screenplayText, &ScreenplayTextEdit::currentModelIndexChanged, this, &ScreenplayTextView::currentModelIndexChanged);
    connect(d->screenplayText, &ScreenplayTextEdit::paragraphTypeChanged, this, [this] {
        d->updateToolBarCurrentParagraphTypeName();
    });
    connect(d->screenplayText, &ScreenplayTextEdit::cursorPositionChanged, this, [this] {
        d->updateToolBarCurrentParagraphTypeName();
    });
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

    const bool useSpellChecker =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                DataStorageLayer::kApplicationUseSpellCheckerKey,
                DataStorageLayer::SettingsStorage::SettingsPlace::Application
                ).toBool();
    d->screenplayText->setUseSpellChecker(useSpellChecker);
    if (useSpellChecker) {
        const QString languageCode =
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    DataStorageLayer::kApplicationSpellCheckerLanguageKey,
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application
                    ).toString();
        d->screenplayText->setSpellCheckLanguage(languageCode);
    }

    d->shortcutsManager.reconfigure();
}

void ScreenplayTextView::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->screenplayText->initWithModel(_model);
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

qreal ScreenplayTextView::scaleFactor() const
{
    return d->scalableWrapper->zoomRange();
}

void ScreenplayTextView::setScaleFactor(qreal _scaleFactor)
{
    d->scalableWrapper->setZoomRange(_scaleFactor);
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

    d->toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                             Ui::DesignSystem::layout().px24()).toPoint());
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

    d->splitter->setHandleColor(DesignSystem::color().primary());
    d->splitter->setHandleWidth(1);
    d->sidebarTabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->sidebarTabs->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
