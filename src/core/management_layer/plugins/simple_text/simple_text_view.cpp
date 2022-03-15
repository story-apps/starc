#include "simple_text_view.h"

#include "text/simple_text_edit.h"
#include "text/simple_text_edit_shortcuts_manager.h"
#include "text/simple_text_edit_toolbar.h"
#include "text/simple_text_search_manager.h"

#include <business_layer/document/simple_text/simple_text_block_data.h>
#include <business_layer/document/simple_text/simple_text_cursor.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_toolbar_animator.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <utils/helpers/ui_helper.h>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
const int kTypeDataRole = Qt::UserRole + 100;

const int kFastFormatTabIndex = 0;
const int kCommentsTabIndex = 1;

const QString kSettingsKey = "simple-text";
const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
} // namespace

class SimpleTextView::Implementation
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


    SimpleTextEdit* textEdit = nullptr;
    SimpleTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;

    SimpleTextEditToolbar* toolbar = nullptr;
    BusinessLayer::SimpleTextSearchManager* searchManager = nullptr;
    FloatingToolbarAnimator* toolbarAnimation = nullptr;
    QHash<BusinessLayer::TextParagraphType, QString> typesToDisplayNames;
    BusinessLayer::TextParagraphType currentParagraphType
        = BusinessLayer::TextParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;
};

SimpleTextView::Implementation::Implementation(QWidget* _parent)
    : textEdit(new SimpleTextEdit(_parent))
    , shortcutsManager(textEdit)
    , scalableWrapper(new ScalableWrapper(textEdit, _parent))
    , toolbar(new SimpleTextEditToolbar(scalableWrapper))
    , searchManager(new BusinessLayer::SimpleTextSearchManager(scalableWrapper, textEdit))
    , toolbarAnimation(new FloatingToolbarAnimator(_parent))
    , paragraphTypesModel(new QStandardItemModel(toolbar))
{
    toolbar->setParagraphTypesModel(paragraphTypesModel);

    textEdit->setVerticalScrollBar(new ScrollBar);
    textEdit->setHorizontalScrollBar(new ScrollBar);
    shortcutsManager.setShortcutsContext(scalableWrapper);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

    textEdit->setUsePageMode(true);
}

void SimpleTextView::Implementation::updateToolBarUi()
{
    toolbar->move(
        QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toPoint());
    toolbar->setBackgroundColor(Ui::DesignSystem::color().background());
    toolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    toolbar->raise();

    searchManager->toolbar()->move(
        QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toPoint());
    searchManager->toolbar()->setBackgroundColor(Ui::DesignSystem::color().background());
    searchManager->toolbar()->setTextColor(Ui::DesignSystem::color().onBackground());
    searchManager->toolbar()->raise();

    toolbarAnimation->setBackgroundColor(Ui::DesignSystem::color().background());
    toolbarAnimation->setTextColor(Ui::DesignSystem::color().onBackground());
}

void SimpleTextView::Implementation::updateToolBarCurrentParagraphTypeName()
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


// ****


SimpleTextView::SimpleTextView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->scalableWrapper);
    d->scalableWrapper->installEventFilter(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->scalableWrapper);

    connect(d->toolbar, &SimpleTextEditToolbar::undoPressed, d->textEdit, &SimpleTextEdit::undo);
    connect(d->toolbar, &SimpleTextEditToolbar::redoPressed, d->textEdit, &SimpleTextEdit::redo);
    connect(d->toolbar, &SimpleTextEditToolbar::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
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
    auto handleCursorPositionChanged = [this] {
        //
        // Обновим состояние панелей форматов
        //
        d->updateToolBarCurrentParagraphTypeName();
        //
        // Уведомим навигатор клиентов, о смене текущего элемента
        //
        const auto screenplayModelIndex = d->textEdit->currentModelIndex();
        emit currentModelIndexChanged(screenplayModelIndex);
    };
    connect(d->textEdit, &SimpleTextEdit::paragraphTypeChanged, this, handleCursorPositionChanged);
    connect(d->textEdit, &SimpleTextEdit::cursorPositionChanged, this, handleCursorPositionChanged);
    //
    connect(
        d->scalableWrapper, &ScalableWrapper::zoomRangeChanged, this,
        [this] { d->updateTextEditPageMargins(); }, Qt::QueuedConnection);

    updateTranslations();
    designSystemChangeEvent(nullptr);

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

void SimpleTextView::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->paragraphTypesModel->clear();

    using namespace BusinessLayer;
    const auto usedTemplate = BusinessLayer::TemplatesFacade::simpleTextTemplate();
    const QVector<TextParagraphType> types
        = { TextParagraphType::ChapterHeading1, TextParagraphType::ChapterHeading2,  TextParagraphType::ChapterHeading3,
            TextParagraphType::ChapterHeading4, TextParagraphType::ChapterHeading5,  TextParagraphType::ChapterHeading6,
            TextParagraphType::Text,     TextParagraphType::InlineNote };
    for (const auto type : types) {
        if (!usedTemplate.paragraphStyle(type).isActive()) {
            continue;
        }

        auto typeItem = new QStandardItem(d->typesToDisplayNames.value(type));
        typeItem->setData(d->shortcutsManager.shortcut(type), Qt::WhatsThisRole);
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        d->paragraphTypesModel->appendRow(typeItem);
    }

    UiHelper::initSpellingFor(d->textEdit);

    d->shortcutsManager.reconfigure();

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(
            DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey)) {
        d->textEdit->reinit();
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
}

void SimpleTextView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor = settingsValue(kScaleFactorKey, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);
}

void SimpleTextView::saveViewSettings()
{
    setSettingsValue(kScaleFactorKey, d->scalableWrapper->zoomRange());
}

void SimpleTextView::setModel(BusinessLayer::SimpleTextModel* _model)
{
    d->textEdit->initWithModel(_model);

    d->updateToolBarCurrentParagraphTypeName();
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

bool SimpleTextView::eventFilter(QObject* _target, QEvent* _event)
{
    if (_target == d->scalableWrapper) {
        if (_event->type() == QEvent::KeyPress && d->searchManager->toolbar()->isVisible()
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

    const auto toolbarPosition
        = QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toPoint();
    d->toolbar->move(toolbarPosition);
    d->searchManager->toolbar()->move(toolbarPosition);
}

void SimpleTextView::updateTranslations()
{
    using namespace BusinessLayer;
    d->typesToDisplayNames = { { TextParagraphType::ChapterHeading1, tr("Heading 1") },
                               { TextParagraphType::ChapterHeading2, tr("Heading 2") },
                               { TextParagraphType::ChapterHeading3, tr("Heading 3") },
                               { TextParagraphType::ChapterHeading4, tr("Heading 4") },
                               { TextParagraphType::ChapterHeading5, tr("Heading 5") },
                               { TextParagraphType::ChapterHeading6, tr("Heading 6") },
                               { TextParagraphType::Text, tr("Text") },
                               { TextParagraphType::InlineNote, tr("Inline note") } };
}

void SimpleTextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolBarUi();

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
}

} // namespace Ui
