#include "stageplay_text_structure_view.h"

#include "stageplay_text_structure_delegate.h"

#include <business_layer/document/stageplay/text/stageplay_text_document.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/counters_info/counters_info_widget.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

class StageplayTextStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить счётчики
     */
    void updateCounters();


    QSortFilterProxyModel* model = nullptr;

    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* content = nullptr;
    StageplayTextStructureDelegate* contentDelegate = nullptr;
    CountersInfoWidget* countersWidget = nullptr;

    /**
     * @brief Редактор текста, который будет использоваться для подсчёта кол-ва страниц, если в
     *        приложении ещё не был открыт редактор текста модели, структуру которого отображаем
     */
    QScopedPointer<PageTextEdit> textEdit;
};

StageplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new StageplayTextStructureDelegate(content))
    , countersWidget(new CountersInfoWidget(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    content->setContextMenuPolicy(Qt::CustomContextMenu);
    content->setDragDropEnabled(true);
    content->setSelectionMode(QAbstractItemView::ExtendedSelection);
    content->setItemDelegate(contentDelegate);

    countersWidget->setSettingsCounterModuleKey(QLatin1String("stageplay"));

    new Shadow(Qt::TopEdge, content);
    new Shadow(Qt::BottomEdge, content);
}

void StageplayTextStructureView::Implementation::updateCounters()
{
    if (model == nullptr || model->sourceModel() == nullptr) {
        return;
    }

    auto stageplayModel = qobject_cast<BusinessLayer::StageplayTextModel*>(model->sourceModel());
    if (stageplayModel == nullptr) {
        return;
    }

    const auto pageCount = [this, stageplayModel] {
        //
        // Если в модели уже задано количество страниц, то используем его
        //
        if (stageplayModel->textPageCount() > 0) {
            textEdit.reset();
            return stageplayModel->textPageCount();
        }

        //
        // А если не задано, то придётся считать вручную
        // NOTE: это возможно, когда не был активирован редактор текста сценария
        //
        if (textEdit.isNull()) {
            const auto& stageplayTemplate = BusinessLayer::TemplatesFacade::stageplayTemplate(
                stageplayModel->informationModel()->templateId());

            textEdit.reset(new PageTextEdit);
            textEdit->setUsePageMode(true);
            textEdit->setPageSpacing(0);
            textEdit->setPageFormat(stageplayTemplate.pageSizeId());
            textEdit->setPageMarginsMm(stageplayTemplate.pageMargins());
            auto stageplayDocument = new BusinessLayer::StageplayTextDocument(textEdit.data());
            textEdit->setDocument(stageplayDocument);

            const bool canChangeModel = false;
            stageplayDocument->setModel(stageplayModel, canChangeModel);
        }

        return textEdit->document()->pageCount();
    }();

    countersWidget->setCounters({
        tr("%n page(s)", "", pageCount),
        tr("%n scene(s)", "", stageplayModel->scenesCount()),
        tr("%n word(s)", "", stageplayModel->wordsCount()),
        tr("%n character(s)", "", stageplayModel->charactersCount().first),
        tr("%n character(s) with spaces", "", stageplayModel->charactersCount().second),
    });
}


// ****


StageplayTextStructureView::StageplayTextStructureView(QWidget* _parent)
    : AbstractNavigator(_parent)
    , d(new Implementation(this))
{
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->backIcon);
    topLayout->addWidget(d->backText, 1);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->content);
    layout->addWidget(d->countersWidget);
    setLayout(layout);


    connect(d->backIcon, &AbstractLabel::clicked, this, &StageplayTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &StageplayTextStructureView::backPressed);
    connect(d->content, &Tree::clicked, this,
            &StageplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::doubleClicked, this,
            &StageplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit customContextMenuRequested(d->content->mapToParent(_pos));
    });
}

StageplayTextStructureView::~StageplayTextStructureView() = default;

QWidget* StageplayTextStructureView::asQWidget()
{
    return this;
}

void StageplayTextStructureView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->content->setContextMenuPolicy(readOnly ? Qt::NoContextMenu : Qt::CustomContextMenu);
    d->content->setDragDropEnabled(!readOnly);
}

void StageplayTextStructureView::setCurrentModelIndex(const QModelIndex& _sourceIndex)
{
    d->content->setCurrentIndex(_sourceIndex);
}

void StageplayTextStructureView::reconfigure()
{
    const bool showSceneNumber
        = settingsValue(DataStorageLayer::kComponentsStageplayNavigatorShowSceneNumberKey).toBool();
    d->contentDelegate->showSceneNumber(showSceneNumber);

    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsStageplayNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsStageplayNavigatorSceneTextLinesKey)
                  .toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void StageplayTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void StageplayTextStructureView::setModel(QAbstractItemModel* _model)
{
    if (d->model != nullptr) {
        d->model->disconnect(this);
        d->textEdit.reset();
    }

    d->content->setModel(_model);

    d->model = qobject_cast<QSortFilterProxyModel*>(_model);
    if (d->model != nullptr) {
        connect(d->model, &QSortFilterProxyModel::modelReset, this,
                [this] { d->updateCounters(); });
        connect(d->model, &QSortFilterProxyModel::dataChanged, this,
                [this] { d->updateCounters(); });
        connect(d->model, &QSortFilterProxyModel::rowsInserted, this,
                [this] { d->updateCounters(); });
        connect(d->model, &QSortFilterProxyModel::rowsMoved, this, [this] { d->updateCounters(); });
        connect(d->model, &QSortFilterProxyModel::rowsRemoved, this,
                [this] { d->updateCounters(); });
    }
}

QModelIndexList StageplayTextStructureView::selectedIndexes() const
{
    return d->content->selectedIndexes();
}

void StageplayTextStructureView::updateTranslations()
{
    d->backText->setText(tr("Back to navigator"));
    d->updateCounters();
}

void StageplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    auto backTextColor = DesignSystem::color().onPrimary();
    backTextColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    for (auto widget : std::vector<Widget*>{
             d->backIcon,
             d->backText,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(backTextColor);
    }
    d->content->setBackgroundColor(DesignSystem::color().primary());
    d->content->setTextColor(DesignSystem::color().onPrimary());

    d->backIcon->setContentsMargins(QMarginsF(isLeftToRight() ? Ui::DesignSystem::layout().px12()
                                                              : Ui::DesignSystem::layout().px4(),
                                              Ui::DesignSystem::compactLayout().px8(),
                                              isLeftToRight() ? Ui::DesignSystem::layout().px4()
                                                              : Ui::DesignSystem::layout().px12(),
                                              Ui::DesignSystem::compactLayout().px8())
                                        .toMargins());
    d->backText->setContentsMargins(
        QMarginsF(isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px16(),
                  Ui::DesignSystem::compactLayout().px12(),
                  isLeftToRight() ? Ui::DesignSystem::layout().px16() : 0.0,
                  Ui::DesignSystem::compactLayout().px12())
            .toMargins());
}

} // namespace Ui
