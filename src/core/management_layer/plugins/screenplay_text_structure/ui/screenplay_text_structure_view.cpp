#include "screenplay_text_structure_view.h"

#include "counters_info_widget.h"
#include "screenplay_text_structure_delegate.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
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

class ScreenplayTextStructureView::Implementation
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
    ScreenplayTextStructureDelegate* contentDelegate = nullptr;
    CountersInfoWidget* countersWidget = nullptr;
};

ScreenplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new ScreenplayTextStructureDelegate(content))
    , countersWidget(new CountersInfoWidget(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    content->setContextMenuPolicy(Qt::CustomContextMenu);
    content->setDragDropEnabled(true);
    content->setSelectionMode(QAbstractItemView::ExtendedSelection);
    content->setItemDelegate(contentDelegate);

    new Shadow(Qt::TopEdge, content);
    new Shadow(Qt::BottomEdge, content);
}

void ScreenplayTextStructureView::Implementation::updateCounters()
{
    if (model == nullptr || model->sourceModel() == nullptr) {
        return;
    }

    auto screenplayModel = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model->sourceModel());
    if (screenplayModel == nullptr) {
        return;
    }

    const auto pageCount = [screenplayModel] {
        //
        // Если в модели уже задано количество страниц, то используем его
        //
        if (screenplayModel->scriptPageCount() > 0) {
            return screenplayModel->scriptPageCount();
        }

        //
        // А если не задано, то придётся считать вручную
        // NOTE: это возможно, когда не был активирован редактор текста сценария
        //
        const auto& screenplayTemplate = BusinessLayer::TemplatesFacade::screenplayTemplate(
            screenplayModel->informationModel()->templateId());

        PageTextEdit textEdit;
        textEdit.setUsePageMode(true);
        textEdit.setPageSpacing(0);
        textEdit.setPageFormat(screenplayTemplate.pageSizeId());
        textEdit.setPageMarginsMm(screenplayTemplate.pageMargins());
        BusinessLayer::ScreenplayTextDocument screenplayDocument;
        textEdit.setDocument(&screenplayDocument);

        const bool kCanChangeModel = false;
        screenplayDocument.setModel(screenplayModel, kCanChangeModel);

        return screenplayDocument.pageCount();
    }();

    countersWidget->setCounters({
        tr("%n page(s)", "", pageCount),
        tr("%n scene(s)", "", screenplayModel->scenesCount()),
        tr("%n word(s)", "", screenplayModel->wordsCount()),
        tr("%n character(s)", "", screenplayModel->charactersCount().first),
        tr("%n character(s) with spaces", "", screenplayModel->charactersCount().second),
    });
}


// ****


ScreenplayTextStructureView::ScreenplayTextStructureView(QWidget* _parent)
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


    connect(d->backIcon, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->content, &Tree::clicked, this,
            &ScreenplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::doubleClicked, this,
            &ScreenplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit customContextMenuRequested(d->content->mapToParent(_pos));
    });
}

ScreenplayTextStructureView::~ScreenplayTextStructureView() = default;

QWidget* ScreenplayTextStructureView::asQWidget()
{
    return this;
}

void ScreenplayTextStructureView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->content->setContextMenuPolicy(readOnly ? Qt::NoContextMenu : Qt::CustomContextMenu);
    d->content->setDragDropEnabled(!readOnly);
}

void ScreenplayTextStructureView::setCurrentModelIndex(const QModelIndex& _mappedIndex)
{
    d->content->setCurrentIndex(_mappedIndex);
}

void ScreenplayTextStructureView::reconfigure()
{
    const bool showSceneNumber
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey)
              .toBool();
    d->contentDelegate->showSceneNumber(showSceneNumber);

    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey)
                  .toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void ScreenplayTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void ScreenplayTextStructureView::setModel(QAbstractItemModel* _model)
{
    if (d->model != nullptr) {
        d->model->disconnect(this);
    }

    d->content->setModel(_model);

    d->model = qobject_cast<QSortFilterProxyModel*>(_model);
    if (d->model != nullptr) {
        connect(d->model, &BusinessLayer::ScreenplayTextModel::modelReset, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::ScreenplayTextModel::dataChanged, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsInserted, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsMoved, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsRemoved, this,
                [this] { d->updateCounters(); });
    }
}

QModelIndexList ScreenplayTextStructureView::selectedIndexes() const
{
    return d->content->selectedIndexes();
}

void ScreenplayTextStructureView::updateTranslations()
{
    d->updateCounters();
}

void ScreenplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
                                              Ui::DesignSystem::layout().px8(),
                                              isLeftToRight() ? Ui::DesignSystem::layout().px4()
                                                              : Ui::DesignSystem::layout().px12(),
                                              Ui::DesignSystem::layout().px8())
                                        .toMargins());
    d->backText->setContentsMargins(
        QMarginsF(isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px16(),
                  Ui::DesignSystem::layout().px12(),
                  isLeftToRight() ? Ui::DesignSystem::layout().px16() : 0.0,
                  Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
