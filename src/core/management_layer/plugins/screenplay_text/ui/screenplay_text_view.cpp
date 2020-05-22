#include "screenplay_text_view.h"

#include "screenplay_text_block_data.h"
#include "screenplay_text_edit.h"
#include "screenplay_text_edit_shortcuts_manager.h"
#include "screenplay_text_edit_toolbar.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/spell_check/spell_checker.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>

#include <QAction>
#include <QScrollArea>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui
{

namespace {
    const int kTypeDataRole = Qt::UserRole + 100;
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


    ScreenplayTextEditToolBar* toolBar = nullptr;
    QHash<BusinessLayer::ScreenplayParagraphType, QString> typesToDisplayNames;
    BusinessLayer::ScreenplayParagraphType currentParagraphType = BusinessLayer::ScreenplayParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;

    ScreenplayTextEdit* screenplayText = nullptr;
    ScreenplayTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;
};

ScreenplayTextView::Implementation::Implementation(QWidget* _parent)
    : toolBar(new ScreenplayTextEditToolBar(_parent)),
      paragraphTypesModel(new QStandardItemModel(toolBar)),
      screenplayText(new ScreenplayTextEdit(_parent)),
      shortcutsManager(screenplayText),
      scalableWrapper(new ScalableWrapper(screenplayText, _parent))
{
    toolBar->setParagraphTypesModel(paragraphTypesModel);

    screenplayText->setVerticalScrollBar(new ScrollBar);
    screenplayText->setHorizontalScrollBar(new ScrollBar);
    shortcutsManager.setShortcutsContext(scalableWrapper);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

    screenplayText->setUsePageMode(true);
    screenplayText->setCursorWidth(DesignSystem::scaleFactor() * 4);
}

void ScreenplayTextView::Implementation::updateToolBarUi()
{
    toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    toolBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolBar->raise();
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
            return;
        }
    }
}


// ****


ScreenplayTextView::ScreenplayTextView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    setFocusProxy(d->scalableWrapper);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->scalableWrapper);
    setLayout(layout);

    connect(d->toolBar, &ScreenplayTextEditToolBar::paragraphTypeChanged, this, [this] (const QModelIndex& _index) {
        const auto type = static_cast<BusinessLayer::ScreenplayParagraphType>( _index.data(kTypeDataRole).toInt());
        d->screenplayText->setCurrentParagraphType(type);
        d->scalableWrapper->setFocus();
    });
    connect(d->screenplayText, &ScreenplayTextEdit::currentModelIndexChanged, this, &ScreenplayTextView::currentModelIndexChanged);
    connect(d->screenplayText, &ScreenplayTextEdit::paragraphTypeChanged, this, [this] {
        d->updateToolBarCurrentParagraphTypeName();
    });
    connect(d->screenplayText, &ScreenplayTextEdit::cursorPositionChanged, this, [this] {
        d->updateToolBarCurrentParagraphTypeName();
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
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        d->paragraphTypesModel->appendRow(typeItem);
    }

    d->shortcutsManager.reconfigure();
}

void ScreenplayTextView::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->screenplayText->initWithModel(_model);

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

void ScreenplayTextView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                             Ui::DesignSystem::layout().px24()).toPoint());
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
}

} // namespace Ui
