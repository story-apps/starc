#include "title_page_view.h"

#include "text/title_page_edit.h"
#include "text/title_page_edit_toolbar.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/audioplay/audioplay_title_page_model.h>
#include <business_layer/model/comic_book/comic_book_title_page_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/stageplay/stageplay_title_page_model.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
const QString kSettingsKey = "simple-text";
const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
} // namespace

class TitlePageView::Implementation
{
public:
    explicit Implementation(TitlePageView* _q);

    /**
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolbarUi();
    void updateToolbarPositon();

    /**
     * @brief Обновить текущий отображаемый тип абзаца в панели инструментов
     */
    void updateToolBarCurrentParagraphTypeName();


    TitlePageView* q = nullptr;

    TitlePageEdit* textEdit = nullptr;
    ScalableWrapper* scalableWrapper = nullptr;

    TitlePageEditToolbar* toolbar = nullptr;
    BusinessLayer::TextParagraphType currentParagraphType
        = BusinessLayer::TextParagraphType::Undefined;

    Domain::DocumentObjectType currentModelType = Domain::DocumentObjectType::Undefined;
};

TitlePageView::Implementation::Implementation(TitlePageView* _q)
    : q(_q)
    , textEdit(new TitlePageEdit(_q))
    , scalableWrapper(new ScalableWrapper(textEdit, _q))
    , toolbar(new TitlePageEditToolbar(scalableWrapper))
{
    textEdit->setVerticalScrollBar(new ScrollBar);
    textEdit->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

    textEdit->setUsePageMode(true);
}

void TitlePageView::Implementation::updateToolbarUi()
{
    toolbar->resize(toolbar->sizeHint());
    updateToolbarPositon();
    toolbar->setBackgroundColor(Ui::DesignSystem::color().background());
    toolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    toolbar->raise();
}

void TitlePageView::Implementation::updateToolbarPositon()
{
    toolbar->move(QPointF(q->isLeftToRight()
                              ? Ui::DesignSystem::layout().px24()
                              : q->width() - toolbar->width() - Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24())
                      .toPoint());
}

void TitlePageView::Implementation::updateToolBarCurrentParagraphTypeName()
{
    toolbar->setCurrentFont(textEdit->textCursor().charFormat().font());
}


// ****


TitlePageView::TitlePageView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->scalableWrapper);
    d->scalableWrapper->installEventFilter(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->scalableWrapper);

    connect(d->toolbar, &TitlePageEditToolbar::undoPressed, d->textEdit, &TitlePageEdit::undo);
    connect(d->toolbar, &TitlePageEditToolbar::redoPressed, d->textEdit, &TitlePageEdit::redo);
    connect(d->toolbar, &TitlePageEditToolbar::fontChanged, this, [this](const QFont& _font) {
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
    connect(d->toolbar, &TitlePageEditToolbar::restoreTitlePagePressed, d->textEdit,
            &TitlePageEdit::restoreFromTemplate);
    //
    auto handleCursorPositionChanged = [this] { d->updateToolBarCurrentParagraphTypeName(); };
    connect(d->textEdit, &TitlePageEdit::paragraphTypeChanged, this, handleCursorPositionChanged);
    connect(d->textEdit, &TitlePageEdit::cursorPositionChanged, this, handleCursorPositionChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);

    reconfigure({});
}

TitlePageView::~TitlePageView() = default;

QWidget* TitlePageView::asQWidget()
{
    return this;
}

void TitlePageView::toggleFullScreen(bool _isFullScreen)
{
    d->toolbar->setVisible(!_isFullScreen);
}

void TitlePageView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->textEdit->setReadOnly(readOnly);
    d->toolbar->setReadOnly(readOnly);
}

void TitlePageView::reconfigure(const QStringList& _changedSettingsKeys)
{
    using namespace BusinessLayer;

    UiHelper::initSpellingFor(d->textEdit);

    QString defaultTemplateKey;
    switch (d->currentModelType) {
    case Domain::DocumentObjectType::ComicBookTitlePage: {
        defaultTemplateKey = DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey;
        break;
    }

    case Domain::DocumentObjectType::ScreenplayTitlePage: {
        defaultTemplateKey = DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey;
        break;
    }

    case Domain::DocumentObjectType::AudioplayTitlePage: {
        defaultTemplateKey = DataStorageLayer::kComponentsAudioplayEditorDefaultTemplateKey;
        break;
    }

    case Domain::DocumentObjectType::StageplayTitlePage: {
        defaultTemplateKey = DataStorageLayer::kComponentsStageplayEditorDefaultTemplateKey;
        break;
    }

    default: {
        break;
    }
    }
    if (_changedSettingsKeys.isEmpty() || _changedSettingsKeys.contains(defaultTemplateKey)) {
        d->textEdit->reinit();
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

void TitlePageView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor = settingsValue(kScaleFactorKey, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);
}

void TitlePageView::saveViewSettings()
{
    setSettingsValue(kScaleFactorKey, d->scalableWrapper->zoomRange());
}

void TitlePageView::setModel(BusinessLayer::SimpleTextModel* _model)
{
    if (qobject_cast<BusinessLayer::ComicBookTitlePageModel*>(_model)) {
        d->currentModelType = Domain::DocumentObjectType::ComicBookTitlePage;
    } else if (qobject_cast<BusinessLayer::ScreenplayTitlePageModel*>(_model)) {
        d->currentModelType = Domain::DocumentObjectType::ScreenplayTitlePage;
    } else if (qobject_cast<BusinessLayer::AudioplayTitlePageModel*>(_model)) {
        d->currentModelType = Domain::DocumentObjectType::AudioplayTitlePage;
    } else if (qobject_cast<BusinessLayer::StageplayTitlePageModel*>(_model)) {
        d->currentModelType = Domain::DocumentObjectType::StageplayTitlePage;
    } else {
        d->currentModelType = Domain::DocumentObjectType::Undefined;
    }

    d->textEdit->initWithModel(_model);

    d->updateToolBarCurrentParagraphTypeName();
}

int TitlePageView::cursorPosition() const
{
    return d->textEdit->textCursor().position();
}

void TitlePageView::setCursorPosition(int _position)
{
    auto cursor = d->textEdit->textCursor();
    cursor.setPosition(_position);
    d->textEdit->ensureCursorVisible(cursor, false);
}

void TitlePageView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->updateToolbarPositon();
}

void TitlePageView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
}

} // namespace Ui
