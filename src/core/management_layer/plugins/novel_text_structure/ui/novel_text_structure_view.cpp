#include "novel_text_structure_view.h"

#include "counters_info_widget.h"
#include "novel_text_structure_delegate.h"

#include <business_layer/document/novel/text/novel_text_document.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/novel_template.h>
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

class NovelTextStructureView::Implementation
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
    NovelTextStructureDelegate* contentDelegate = nullptr;
    CountersInfoWidget* countersWidget = nullptr;
};

NovelTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new NovelTextStructureDelegate(content))
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

void NovelTextStructureView::Implementation::updateCounters()
{
    if (model == nullptr || model->sourceModel() == nullptr) {
        return;
    }

    auto novelModel = qobject_cast<BusinessLayer::NovelTextModel*>(model->sourceModel());
    if (novelModel == nullptr) {
        return;
    }

    const auto pageCount = [novelModel] {
        //
        // Если в модели уже задано количество страниц, то используем его
        //
        if (novelModel->textPageCount() > 0) {
            return novelModel->textPageCount();
        }

        //
        // А если не задано, то придётся считать вручную
        // NOTE: это возможно, когда не был активирован редактор текста сценария
        //
        const auto& novelTemplate = BusinessLayer::TemplatesFacade::novelTemplate(
            novelModel->informationModel()->templateId());

        PageTextEdit textEdit;
        textEdit.setUsePageMode(true);
        textEdit.setPageSpacing(0);
        textEdit.setPageFormat(novelTemplate.pageSizeId());
        textEdit.setPageMarginsMm(novelTemplate.pageMargins());
        BusinessLayer::NovelTextDocument novelDocument;
        textEdit.setDocument(&novelDocument);

        const bool kCanChangeModel = false;
        novelDocument.setModel(novelModel, kCanChangeModel);

        return novelDocument.pageCount();
    }();

    countersWidget->setCounters({
        tr("%n page(s)", "", pageCount),
        tr("%n scene(s)", "", novelModel->scenesCount()),
        tr("%n word(s)", "", novelModel->wordsCount()),
        tr("%n character(s)", "", novelModel->charactersCount().first),
        tr("%n character(s) with spaces", "", novelModel->charactersCount().second),
    });
}


// ****


NovelTextStructureView::NovelTextStructureView(QWidget* _parent)
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


    connect(d->backIcon, &AbstractLabel::clicked, this, &NovelTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &NovelTextStructureView::backPressed);
    connect(d->content, &Tree::clicked, this, &NovelTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::doubleClicked, this,
            &NovelTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit customContextMenuRequested(d->content->mapToParent(_pos));
    });
}

NovelTextStructureView::~NovelTextStructureView() = default;

QWidget* NovelTextStructureView::asQWidget()
{
    return this;
}

void NovelTextStructureView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->content->setContextMenuPolicy(readOnly ? Qt::NoContextMenu : Qt::CustomContextMenu);
    d->content->setDragDropEnabled(!readOnly);
}

void NovelTextStructureView::setCurrentModelIndex(const QModelIndex& _mappedIndex)
{
    d->content->setCurrentIndex(_mappedIndex);
}

void NovelTextStructureView::reconfigure()
{
    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsNovelNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsNovelNavigatorSceneTextLinesKey).toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void NovelTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void NovelTextStructureView::setModel(QAbstractItemModel* _model)
{
    if (d->model != nullptr) {
        d->model->disconnect(this);
    }

    d->content->setModel(_model);

    d->model = qobject_cast<QSortFilterProxyModel*>(_model);
    if (d->model != nullptr) {
        connect(d->model, &BusinessLayer::NovelTextModel::modelReset, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::NovelTextModel::dataChanged, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::NovelTextModel::rowsInserted, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::NovelTextModel::rowsMoved, this,
                [this] { d->updateCounters(); });
        connect(d->model, &BusinessLayer::NovelTextModel::rowsRemoved, this,
                [this] { d->updateCounters(); });
    }
}

QModelIndexList NovelTextStructureView::selectedIndexes() const
{
    return d->content->selectedIndexes();
}

void NovelTextStructureView::updateTranslations()
{
    d->updateCounters();
}

void NovelTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
