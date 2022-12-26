#include "world_information_structure_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
enum {
    kMainIndex = 0,
    kWorldInfoIndex,
    kNatureIndex,
    kCultureIndex,
    kSystemIndex,
    kPoliticsIndex,
    kMagicIndex,
};
enum {
    kNatureInfoIndex = 0,
    kRacesIndex,
    kFloraIndex,
    kAnimalsIndex,
    kNaturalResourcesIndex,
    kClimateIndex,
};
enum {
    kReligionsIndex = 0,
    kEthicsIndex,
    kLanguagesIndex,
    kCastesIndex,
};
enum {
    kMagicInfoIndex = 0,
    kMagicTypesIndex,
};
} // namespace

class WorldInformationStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* traits = nullptr;
};

WorldInformationStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , traits(new Tree(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    traits->setContextMenuPolicy(Qt::NoContextMenu);
    traits->setSelectionMode(QAbstractItemView::SingleSelection);

    //
    // TODO: подобрать иконки
    //
    auto createItem = [](const QString& _icon) {
        auto item = new QStandardItem;
        //        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    auto traitsModel = new QStandardItemModel(traits);
    traitsModel->appendRow(createItem({}));
    traitsModel->appendRow(createItem({}));
    auto natureItem = createItem({});
    natureItem->appendRows({
        createItem({}),
        createItem({}),
        createItem({}),
        createItem({}),
        createItem({}),
        createItem({}),
    });
    traitsModel->appendRow(natureItem);
    auto cultureItem = createItem({});
    cultureItem->appendRows({
        createItem({}),
        createItem({}),
        createItem({}),
        createItem({}),
    });
    traitsModel->appendRow(cultureItem);
    traitsModel->appendRow(createItem({}));
    traitsModel->appendRow(createItem({}));
    auto magicItem = createItem({});
    magicItem->appendRows({
        createItem({}),
        createItem({}),
    });
    traitsModel->appendRow(magicItem);
    traits->setModel(traitsModel);
    traits->setCurrentIndex(traitsModel->index(0, 0));
}


// ****


WorldInformationStructureView::WorldInformationStructureView(QWidget* _parent)
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
    layout->addWidget(d->traits, 1);
    setLayout(layout);


    connect(d->backIcon, &AbstractLabel::clicked, this,
            &WorldInformationStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this,
            &WorldInformationStructureView::backPressed);
    connect(d->traits, &Tree::clicked, this,
            &WorldInformationStructureView::traitsCategoryIndexChanged);
    connect(d->traits, &Tree::doubleClicked, this,
            &WorldInformationStructureView::traitsCategoryIndexChanged);
}

WorldInformationStructureView::~WorldInformationStructureView() = default;

QWidget* WorldInformationStructureView::asQWidget()
{
    return this;
}

void WorldInformationStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void WorldInformationStructureView::updateTranslations()
{
    auto traitsModel = qobject_cast<QStandardItemModel*>(d->traits->model());
    traitsModel->item(kMainIndex)->setText(tr("Main"));
    traitsModel->item(kWorldInfoIndex)->setText(tr("World description"));
    auto natureItem = traitsModel->item(kNatureIndex);
    natureItem->setText(tr("Nature"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kNatureInfoIndex, 0, traitsModel->indexFromItem(natureItem)))
        ->setText(tr("Basic info"));
    traitsModel
        ->itemFromIndex(traitsModel->index(kRacesIndex, 0, traitsModel->indexFromItem(natureItem)))
        ->setText(tr("Races"));
    traitsModel
        ->itemFromIndex(traitsModel->index(kFloraIndex, 0, traitsModel->indexFromItem(natureItem)))
        ->setText(tr("Flora"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kAnimalsIndex, 0, traitsModel->indexFromItem(natureItem)))
        ->setText(tr("Animals"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kNaturalResourcesIndex, 0, traitsModel->indexFromItem(natureItem)))
        ->setText(tr("Natural resources"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kClimateIndex, 0, traitsModel->indexFromItem(natureItem)))
        ->setText(tr("Climate"));
    auto cultureItem = traitsModel->item(kCultureIndex);
    cultureItem->setText(tr("Culture"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kReligionsIndex, 0, traitsModel->indexFromItem(cultureItem)))
        ->setText(tr("Religions and beliefs"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kEthicsIndex, 0, traitsModel->indexFromItem(cultureItem)))
        ->setText(tr("Ethics and values"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kLanguagesIndex, 0, traitsModel->indexFromItem(cultureItem)))
        ->setText(tr("Languages"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kCastesIndex, 0, traitsModel->indexFromItem(cultureItem)))
        ->setText(tr("Class/caste system"));
    traitsModel->item(kSystemIndex)->setText(tr("System"));
    traitsModel->item(kPoliticsIndex)->setText(tr("Politics"));
    auto magicItem = traitsModel->item(kMagicIndex);
    magicItem->setText(tr("Magic"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kMagicInfoIndex, 0, traitsModel->indexFromItem(magicItem)))
        ->setText(tr("Basic info"));
    traitsModel
        ->itemFromIndex(
            traitsModel->index(kMagicTypesIndex, 0, traitsModel->indexFromItem(magicItem)))
        ->setText(tr("Magic types"));
}

void WorldInformationStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
    d->traits->setBackgroundColor(DesignSystem::color().primary());
    d->traits->setTextColor(DesignSystem::color().onPrimary());

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
}

} // namespace Ui
