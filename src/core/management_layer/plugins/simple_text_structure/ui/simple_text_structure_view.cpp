#include "simple_text_structure_view.h"

#include "simple_text_structure_delegate.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/templates/simple_text_template.h>
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

#include <QSortFilterProxyModel>
#include <QVBoxLayout>


namespace Ui {

class SimpleTextStructureView::Implementation
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
    SimpleTextStructureDelegate* contentDelegate = nullptr;
    CountersInfoWidget* countersWidget = nullptr;

    /**
     * @brief Редактор текста, который будет использоваться для подсчёта кол-ва страниц, если в
     *        приложении ещё не был открыт редактор текста модели, структуру которого отображаем
     */
    QScopedPointer<PageTextEdit> textEdit;
};

SimpleTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new SimpleTextStructureDelegate(content))
    , countersWidget(new CountersInfoWidget(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    content->setContextMenuPolicy(Qt::CustomContextMenu);
    content->setDragDropEnabled(true);
    content->setSelectionMode(QAbstractItemView::ExtendedSelection);
    content->setItemDelegate(contentDelegate);

    countersWidget->setSettingsCounterModuleKey(QLatin1String("simple_text"));

    new Shadow(Qt::TopEdge, content);
    new Shadow(Qt::BottomEdge, content);
}

void SimpleTextStructureView::Implementation::updateCounters()
{
    if (model == nullptr || model->sourceModel() == nullptr) {
        return;
    }

    auto simpleTextModel = qobject_cast<BusinessLayer::SimpleTextModel*>(model->sourceModel());
    if (simpleTextModel == nullptr) {
        return;
    }

    const auto pageCount = [this, simpleTextModel] {
        //
        // Если в модели уже задано количество страниц, то используем его
        //
        if (simpleTextModel->textPageCount() > 0) {
            textEdit.reset();
            return simpleTextModel->textPageCount();
        }

        //
        // А если не задано, то придётся считать вручную
        // NOTE: это возможно, когда не был активирован редактор текста сценария
        //
        if (textEdit.isNull()) {
            const auto& simpleTextTemplate = BusinessLayer::TemplatesFacade::simpleTextTemplate();

            textEdit.reset(new PageTextEdit);
            textEdit->setUsePageMode(true);
            textEdit->setPageSpacing(0);
            textEdit->setPageFormat(simpleTextTemplate.pageSizeId());
            textEdit->setPageMarginsMm(simpleTextTemplate.pageMargins());
            auto simpleTextDocument = new BusinessLayer::SimpleTextDocument(textEdit.data());
            textEdit->setDocument(simpleTextDocument);

            const bool canChangeModel = false;
            simpleTextDocument->setModel(simpleTextModel, canChangeModel);
        }

        return textEdit->document()->pageCount();
    }();

    countersWidget->setCounters({
        tr("%n page(s)", "", pageCount),
        tr("%n word(s)", "", simpleTextModel->wordsCount()),
        tr("%n character(s)", "", simpleTextModel->charactersCount().first),
        tr("%n character(s) with spaces", "", simpleTextModel->charactersCount().second),
    });
}


// ****


SimpleTextStructureView::SimpleTextStructureView(QWidget* _parent)
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

    connect(d->backIcon, &AbstractLabel::clicked, this, &SimpleTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &SimpleTextStructureView::backPressed);
    connect(d->content, &Tree::clicked, this, &SimpleTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::doubleClicked, this,
            &SimpleTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit customContextMenuRequested(d->content->mapToParent(_pos));
    });

    reconfigure();
}

SimpleTextStructureView::~SimpleTextStructureView() = default;

QWidget* SimpleTextStructureView::asQWidget()
{
    return this;
}

void SimpleTextStructureView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->content->setContextMenuPolicy(readOnly ? Qt::NoContextMenu : Qt::CustomContextMenu);
    d->content->setDragDropEnabled(!readOnly);
}

void SimpleTextStructureView::setCurrentModelIndex(const QModelIndex& _index)
{
    d->content->setCurrentIndex(_index);
}

void SimpleTextStructureView::reconfigure()
{
    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorSceneTextLinesKey)
                  .toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void SimpleTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void SimpleTextStructureView::setModel(QAbstractItemModel* _model)
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

QModelIndexList SimpleTextStructureView::selectedIndexes() const
{
    return d->content->selectedIndexes();
}

void SimpleTextStructureView::expandAll()
{
    d->content->expandAll();
}

void SimpleTextStructureView::collapseAll()
{
    d->content->collapseAll();
}

void SimpleTextStructureView::updateTranslations()
{
    d->backIcon->setToolTip(tr("Back to navigator"));
    d->backText->setToolTip(tr("Back to navigator"));
    d->updateCounters();
}

void SimpleTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
