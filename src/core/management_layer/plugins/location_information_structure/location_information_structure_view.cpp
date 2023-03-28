#include "location_information_structure_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
enum {
    kMainIndex = 0,
    kNatureInfoIndex,
    kGeographyIndex,
    kBackgroundIndex,
};
} // namespace

class LocationInformationStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* traits = nullptr;
};

LocationInformationStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , traits(new Tree(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    traits->setContextMenuPolicy(Qt::NoContextMenu);
    traits->setSelectionMode(QAbstractItemView::SingleSelection);

    auto createItem = [](const QString& _icon) {
        auto item = new QStandardItem;
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    auto traitsModel = new QStandardItemModel(traits);
    traitsModel->appendRow(createItem(u8"\U000F06A1"));
    traitsModel->appendRow(createItem(u8"\U000F0C68"));
    traitsModel->appendRow(createItem(u8"\U000F1A48"));
    traitsModel->appendRow(createItem(u8"\U000F0DD4"));
    traits->setModel(traitsModel);
    traits->setCurrentIndex(traitsModel->index(0, 0));
}


// ****


LocationInformationStructureView::LocationInformationStructureView(QWidget* _parent)
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
            &LocationInformationStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this,
            &LocationInformationStructureView::backPressed);
    connect(d->traits, &Tree::clicked, this,
            &LocationInformationStructureView::traitsCategoryIndexChanged);
    connect(d->traits, &Tree::doubleClicked, this,
            &LocationInformationStructureView::traitsCategoryIndexChanged);
}

LocationInformationStructureView::~LocationInformationStructureView() = default;

QWidget* LocationInformationStructureView::asQWidget()
{
    return this;
}

void LocationInformationStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void LocationInformationStructureView::updateTranslations()
{
    auto traitsModel = qobject_cast<QStandardItemModel*>(d->traits->model());
    traitsModel->item(kMainIndex)->setText(tr("Main"));
    traitsModel->item(kNatureInfoIndex)->setText(tr("Sense"));
    traitsModel->item(kGeographyIndex)->setText(tr("Geography"));
    traitsModel->item(kBackgroundIndex)->setText(tr("Background"));
}

void LocationInformationStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
                  DesignSystem::compactLayout().px8(),
                  isLeftToRight() ? DesignSystem::layout().px4() : DesignSystem::layout().px12(),
                  DesignSystem::compactLayout().px8())
            .toMargins());
    d->backText->setContentsMargins(QMarginsF(isLeftToRight() ? 0.0 : DesignSystem::layout().px16(),
                                              DesignSystem::compactLayout().px12(),
                                              isLeftToRight() ? DesignSystem::layout().px16() : 0.0,
                                              DesignSystem::compactLayout().px12())
                                        .toMargins());
}

} // namespace Ui
