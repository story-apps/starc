#include "screenplay_title_page_view.h"

#include "text/screenplay_title_page_edit.h"
#include "text/screenplay_title_page_edit_toolbar.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/text_template.h>
#include <business_layer/templates/templates_facade.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_toolbar_animator.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>

#include <utils/helpers/text_helper.h>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui
{

namespace {
    const int kTypeDataRole = Qt::UserRole + 100;

    const QString kSettingsKey = "simple-text";
    const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
    const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
    const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
    const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
}

class ScreenplayTitlePageView::Implementation
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


    ScreenplayTitlePageEdit* textEdit = nullptr;
    ScalableWrapper* scalableWrapper = nullptr;

    ScreenplayTitlePageEditToolbar* toolbar = nullptr;
    FloatingToolbarAnimator* toolbarAnimation = nullptr;
    QHash<BusinessLayer::TextParagraphType, QString> typesToDisplayNames;
    BusinessLayer::TextParagraphType currentParagraphType = BusinessLayer::TextParagraphType::Undefined;
};

ScreenplayTitlePageView::Implementation::Implementation(QWidget* _parent)
    : textEdit(new ScreenplayTitlePageEdit(_parent)),
      scalableWrapper(new ScalableWrapper(textEdit, _parent)),
      toolbar(new ScreenplayTitlePageEditToolbar(scalableWrapper)),
      toolbarAnimation(new FloatingToolbarAnimator(_parent))
{
    textEdit->setVerticalScrollBar(new ScrollBar);
    textEdit->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

    textEdit->setUsePageMode(true);
}

void ScreenplayTitlePageView::Implementation::updateToolBarUi()
{
    toolbar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    toolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolbar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolbar->raise();

    toolbarAnimation->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolbarAnimation->setTextColor(Ui::DesignSystem::color().onPrimary());
}

void ScreenplayTitlePageView::Implementation::updateToolBarCurrentParagraphTypeName()
{
    toolbar->setCurrentFont(textEdit->textCursor().charFormat().font());
}


// ****


ScreenplayTitlePageView::ScreenplayTitlePageView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    setFocusProxy(d->scalableWrapper);
    d->scalableWrapper->installEventFilter(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->scalableWrapper);

    connect(d->toolbar, &ScreenplayTitlePageEditToolbar::undoPressed, d->textEdit, &ScreenplayTitlePageEdit::undo);
    connect(d->toolbar, &ScreenplayTitlePageEditToolbar::redoPressed, d->textEdit, &ScreenplayTitlePageEdit::redo);
    connect(d->toolbar, &ScreenplayTitlePageEditToolbar::fontChanged, this, [this] (const QFont& _font) {
        //
        // Применяем шрифт
        //
        d->textEdit->setTextFont(_font);
        //
        // Обновим высоту блока, если требуется
        //
        auto cursor = d->textEdit->textCursor();
        qreal blockHeight = 0.0;
        const auto formats = cursor.block().textFormats();
        for (const auto& format : formats) {
            blockHeight = std::max(blockHeight, TextHelper::fineLineSpacing(format.format.font()));
        }
        if (!qFuzzyCompare(cursor.blockFormat().lineHeight(), blockHeight)) {
            auto blockFormat = cursor.blockFormat();
            blockFormat.setLineHeight(blockHeight, QTextBlockFormat::FixedHeight);
            cursor.setBlockFormat(blockFormat);
        }
        //
        // Возвращем фокус в редактор текста
        //
        d->scalableWrapper->setFocus();
    });
    //
    auto handleCursorPositionChanged = [this] { d->updateToolBarCurrentParagraphTypeName(); };
    connect(d->textEdit, &ScreenplayTitlePageEdit::paragraphTypeChanged, this, handleCursorPositionChanged);
    connect(d->textEdit, &ScreenplayTitlePageEdit::cursorPositionChanged, this, handleCursorPositionChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);

    reconfigure({});
}

ScreenplayTitlePageView::~ScreenplayTitlePageView() = default;

void ScreenplayTitlePageView::reconfigure(const QStringList& _changedSettingsKeys)
{
    using namespace BusinessLayer;

    auto settingsValue = [] (const QString& _key) {
        return DataStorageLayer::StorageFacade::settingsStorage()->value(
                    _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    };

    const bool useSpellChecker
            = settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool();
    d->textEdit->setUseSpellChecker(useSpellChecker);
    if (useSpellChecker) {
        const QString languageCode
                = settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString();
        d->textEdit->setSpellCheckLanguage(languageCode);
    }

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey)) {
        TemplatesFacade::setDefaultTextTemplate(
                    settingsValue(DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey).toString());
        d->textEdit->reinit();
    }

    if (_changedSettingsKeys.isEmpty()
        || _changedSettingsKeys.contains(DataStorageLayer::kComponentsScreenplayEditorHighlightCurrentLineKey)) {
        d->textEdit->setHighlightCurrentLine(
            settingsValue(DataStorageLayer::kComponentsSimpleTextEditorHighlightCurrentLineKey).toBool());
    }
}

void ScreenplayTitlePageView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor
            = StorageFacade::settingsStorage()->value(
                  kScaleFactorKey, SettingsStorage::SettingsPlace::Application, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);
}

void ScreenplayTitlePageView::saveViewSettings()
{
    using namespace DataStorageLayer;

    StorageFacade::settingsStorage()->setValue(
        kScaleFactorKey, d->scalableWrapper->zoomRange(), SettingsStorage::SettingsPlace::Application);
}

void ScreenplayTitlePageView::setModel(BusinessLayer::TextModel* _model)
{
    d->textEdit->initWithModel(_model);

    d->updateToolBarCurrentParagraphTypeName();
}

int ScreenplayTitlePageView::cursorPosition() const
{
    return d->textEdit->textCursor().position();
}

void ScreenplayTitlePageView::setCursorPosition(int _position)
{
    auto cursor = d->textEdit->textCursor();
    cursor.setPosition(_position);
    d->textEdit->ensureCursorVisible(cursor, false);
}

void ScreenplayTitlePageView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    const auto toolbarPosition = QPointF(Ui::DesignSystem::layout().px24(),
                                         Ui::DesignSystem::layout().px24()).toPoint();
    d->toolbar->move(toolbarPosition);
}

void ScreenplayTitlePageView::updateTranslations()
{
    using namespace  BusinessLayer;
    d->typesToDisplayNames = {{ TextParagraphType::Heading1, tr("Heading 1") },
                              { TextParagraphType::Heading2, tr("Heading 2") },
                              { TextParagraphType::Heading3, tr("Heading 3") },
                              { TextParagraphType::Heading4, tr("Heading 4") },
                              { TextParagraphType::Heading5, tr("Heading 5") },
                              { TextParagraphType::Heading6, tr("Heading 6") },
                              { TextParagraphType::Text, tr("Text") },
                              { TextParagraphType::InlineNote, tr("Inline note") }};
}

void ScreenplayTitlePageView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolBarUi();

    d->textEdit->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().background());
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onBackground());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    d->scalableWrapper->setPalette(palette);
    d->textEdit->setPalette(palette);

}

} // namespace Ui
