#include "screenplay_breakdown_structure_view.h"

#include "../business_layer/screenplay_breakdown_structure_model_item.h"
#include "counters_info_widget.h"
#include "screenplay_breakdown_structure_scene_delegate.h"
#include "screenplay_breakdown_structure_tag_delegate.h"

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
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

namespace {

/**
 * @brief Вкладки навигатора
 */
enum {
    kScenesTab = 0,
    kCharactersTab,
    kLocationsTab,
    kTagsTab,
};

} // namespace

class ScreenplayBreakdownStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Widget* backDivider = nullptr;
    TabBar* tabs = nullptr;
    StackWidget* content = nullptr;
    Tree* scenesView = nullptr;
    Tree* charactersView = nullptr;
    Tree* locationsView = nullptr;
    QVariant locationsViewState;
    Tree* tagsView = nullptr;
    ScreenplayBreakdownStructureSceneDelegate* scenesDelegate = nullptr;
    ScreenplayBreakdownStructureTagDelegate* tagsDelegate = nullptr;
    CountersInfoWidget* countersWidget = nullptr;
};

ScreenplayBreakdownStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , backDivider(new Widget(_parent))
    , tabs(new TabBar(_parent))
    , content(new StackWidget(_parent))
    , scenesView(new Tree(_parent))
    , charactersView(new Tree(_parent))
    , locationsView(new Tree(_parent))
    , tagsView(new Tree(_parent))
    , scenesDelegate(new ScreenplayBreakdownStructureSceneDelegate(_parent))
    , tagsDelegate(new ScreenplayBreakdownStructureTagDelegate(_parent))
    , countersWidget(new CountersInfoWidget(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    tabs->addTab("Scenes");
    tabs->addTab("Characters");
    tabs->addTab("Locations");
    tabs->addTab("Tags");
    content->setAnimationType(StackWidget::AnimationType::Slide);
    content->setCurrentWidget(scenesView);
    content->addWidget(charactersView);
    content->addWidget(locationsView);
    content->addWidget(tagsView);

    scenesView->setContextMenuPolicy(Qt::CustomContextMenu);
    scenesView->setDragDropEnabled(true);
    scenesView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    scenesView->setItemDelegate(scenesDelegate);
    charactersView->setContextMenuPolicy(Qt::CustomContextMenu);
    charactersView->setItemDelegate(tagsDelegate);
    locationsView->setContextMenuPolicy(Qt::CustomContextMenu);
    locationsView->setItemDelegate(tagsDelegate);
    tagsView->setContextMenuPolicy(Qt::CustomContextMenu);
    tagsView->setItemDelegate(tagsDelegate);

    countersWidget->hide();

    new Shadow(Qt::BottomEdge, scenesView);
    new Shadow(Qt::BottomEdge, charactersView);
    new Shadow(Qt::BottomEdge, locationsView);
    new Shadow(Qt::BottomEdge, tagsView);
}


// ****


ScreenplayBreakdownStructureView::ScreenplayBreakdownStructureView(QWidget* _parent)
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
    layout->addWidget(d->backDivider);
    layout->addWidget(d->tabs);
    layout->addWidget(d->content, 1);
    layout->addWidget(d->countersWidget);
    setLayout(layout);


    connect(d->backIcon, &AbstractLabel::clicked, this,
            &ScreenplayBreakdownStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this,
            &ScreenplayBreakdownStructureView::backPressed);
    connect(d->tabs, &TabBar::currentIndexChanged, d->content, [this](int _tabIndex) {
        switch (_tabIndex) {
        default:
        case kScenesTab: {
            d->content->setCurrentWidget(d->scenesView);
            break;
        }
        case kCharactersTab: {
            d->content->setCurrentWidget(d->charactersView);
            break;
        }
        case kLocationsTab: {
            d->content->setCurrentWidget(d->locationsView);
            break;
        }
        case kTagsTab: {
            d->content->setCurrentWidget(d->tagsView);
            break;
        }
        }
    });
    connect(d->scenesView, &Tree::clicked, this,
            &ScreenplayBreakdownStructureView::currentSceneModelIndexChanged);
    connect(d->locationsView, &Tree::clicked, this, [this](const QModelIndex& _index) {
        const auto screenplayItemIndex
            = _index.data(BusinessLayer::ScreenplayBreakdownStructureModelItem::ScreenplayIndexRole)
                  .toModelIndex();
        if (screenplayItemIndex.isValid()) {
            emit currentLocationSceneModelIndexChanged(screenplayItemIndex);
        }
    });
    connect(d->locationsView, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit locationsViewContextMenuRequested(d->locationsView->mapTo(this, _pos));
    });
}

ScreenplayBreakdownStructureView::~ScreenplayBreakdownStructureView() = default;

QWidget* ScreenplayBreakdownStructureView::asQWidget()
{
    return this;
}

void ScreenplayBreakdownStructureView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->scenesView->setContextMenuPolicy(readOnly ? Qt::NoContextMenu : Qt::CustomContextMenu);
    d->scenesView->setDragDropEnabled(!readOnly);
}

void ScreenplayBreakdownStructureView::setCurrentModelIndex(const QModelIndex& _mappedIndex)
{
    d->scenesView->setCurrentIndex(_mappedIndex);
}

void ScreenplayBreakdownStructureView::reconfigure()
{
    const bool showSceneNumber
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey)
              .toBool();
    d->scenesDelegate->showSceneNumber(showSceneNumber);

    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->scenesDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey)
                  .toInt();
        d->scenesDelegate->setTextLinesSize(sceneTextLines);
    }

    d->scenesView->setItemDelegate(nullptr);
    d->scenesView->setItemDelegate(d->scenesDelegate);
    d->charactersView->setItemDelegate(d->tagsDelegate);
    d->locationsView->setItemDelegate(d->tagsDelegate);
    d->tagsView->setItemDelegate(d->tagsDelegate);
}

void ScreenplayBreakdownStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void ScreenplayBreakdownStructureView::setModels(QAbstractItemModel* _scenesModel,
                                                 QAbstractItemModel* _charactersModel,
                                                 QAbstractItemModel* _locationsModel)
{
    d->scenesView->setModel(_scenesModel);

    d->charactersView->setModel(_charactersModel);

    d->locationsView->setModel(_locationsModel);
    connect(_locationsModel, &QAbstractItemModel::modelAboutToBeReset, this,
            [this] { d->locationsViewState = d->locationsView->saveState(); });
    connect(_locationsModel, &QAbstractItemModel::modelReset, this,
            [this] { d->locationsView->restoreState(d->locationsViewState); });
}

void ScreenplayBreakdownStructureView::expandLocations(int _level)
{
    //
    // Сначала всё свернём
    //
    collapseAllLocations();
    //
    // ... а потом открываем необходимый уовень
    //
    switch (_level) {
    default:
    case -1: {
        d->locationsView->expandAll();
        break;
    }

    case 0: {
        for (int row = 0; row < d->locationsView->model()->rowCount(); ++row) {
            const auto index = d->locationsView->model()->index(row, 0);
            d->locationsView->expand(index);
        }
        break;
    }

    case 1: {
        for (int row = 0; row < d->locationsView->model()->rowCount(); ++row) {
            const auto index = d->locationsView->model()->index(row, 0);
            d->locationsView->expand(index);
            for (int childRow = 0; childRow < d->locationsView->model()->rowCount(index);
                 ++childRow) {
                const auto childIndex = d->locationsView->model()->index(childRow, 0, index);
                d->locationsView->expand(childIndex);
            }
        }
        break;
    }
    }
}

void ScreenplayBreakdownStructureView::collapseAllLocations()
{
    d->locationsView->collapseAll();
}

QModelIndexList ScreenplayBreakdownStructureView::selectedIndexes() const
{
    return d->scenesView->selectedIndexes();
}

void ScreenplayBreakdownStructureView::updateTranslations()
{
    d->tabs->setTabName(kScenesTab, tr("Scenes"));
    d->tabs->setTabName(kCharactersTab, tr("Characters"));
    d->tabs->setTabName(kLocationsTab, tr("Locations"));
    d->tabs->setTabName(kTagsTab, tr("Tags"));
}

void ScreenplayBreakdownStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    auto backTextColor = DesignSystem::color().onPrimary();
    backTextColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    for (auto widget : std::vector<Widget*>{
             d->backIcon,
             d->backText,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(backTextColor);
    }
    for (auto view : std::vector<Widget*>{
             d->tabs,
             d->content,
             d->scenesView,
             d->charactersView,
             d->locationsView,
             d->tagsView,
         }) {
        view->setBackgroundColor(DesignSystem::color().primary());
        view->setTextColor(DesignSystem::color().onPrimary());
    }

    d->backIcon->setContentsMargins(
        QMarginsF(isLeftToRight() ? DesignSystem::layout().px12() : DesignSystem::layout().px4(),
                  DesignSystem::layout().px8(),
                  isLeftToRight() ? DesignSystem::layout().px4() : DesignSystem::layout().px12(),
                  DesignSystem::layout().px8())
            .toMargins());
    d->backText->setContentsMargins(QMarginsF(isLeftToRight() ? 0.0 : DesignSystem::layout().px16(),
                                              DesignSystem::layout().px12(),
                                              isLeftToRight() ? DesignSystem::layout().px16() : 0.0,
                                              DesignSystem::layout().px12())
                                        .toMargins());
    d->backDivider->setBackgroundColor(ColorHelper::transparent(
        DesignSystem::color().shadow(), DesignSystem::disabledTextOpacity()));
    d->backDivider->setFixedHeight(DesignSystem::layout().px());
}

} // namespace Ui
