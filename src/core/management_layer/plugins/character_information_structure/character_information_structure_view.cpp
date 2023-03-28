#include "character_information_structure_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
enum {
    kMainIndex = 0,
    kStoryIndex,
    kBasicIndex,
    kPhysiqueIndex,
    kLifeIndex,
    kAttitudeIndex,
    kBiographyIndex,
};
} // namespace

class CharacterInformationStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* traits = nullptr;
};

CharacterInformationStructureView::Implementation::Implementation(QWidget* _parent)
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
    traitsModel->appendRow(createItem(u8"\U000F0007"));
    traitsModel->appendRow(createItem(u8"\U000F0306"));
    traitsModel->appendRow(createItem(u8"\U000F1570"));
    traitsModel->appendRow(createItem(u8"\U000F0FD7"));
    traitsModel->appendRow(createItem(u8"\U000F1744"));
    traitsModel->appendRow(createItem(u8"\U000F133E"));
    traitsModel->appendRow(createItem(u8"\U000F06BC"));
    traits->setModel(traitsModel);
    traits->setCurrentIndex(traitsModel->index(0, 0));
}


// ****


CharacterInformationStructureView::CharacterInformationStructureView(QWidget* _parent)
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
            &CharacterInformationStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this,
            &CharacterInformationStructureView::backPressed);
    connect(d->traits, &Tree::clicked, this,
            &CharacterInformationStructureView::traitsCategoryIndexChanged);
    connect(d->traits, &Tree::doubleClicked, this,
            &CharacterInformationStructureView::traitsCategoryIndexChanged);
}

CharacterInformationStructureView::~CharacterInformationStructureView() = default;

QWidget* CharacterInformationStructureView::asQWidget()
{
    return this;
}

void CharacterInformationStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void CharacterInformationStructureView::updateTranslations()
{
    auto traitsModel = qobject_cast<QStandardItemModel*>(d->traits->model());
    traitsModel->item(kMainIndex)->setText(tr("Main"));
    traitsModel->item(kStoryIndex)->setText(tr("Story"));
    traitsModel->item(kBasicIndex)->setText(tr("Personal info"));
    traitsModel->item(kPhysiqueIndex)->setText(tr("Physique"));
    traitsModel->item(kLifeIndex)->setText(tr("Life"));
    traitsModel->item(kAttitudeIndex)->setText(tr("Attitude"));
    traitsModel->item(kBiographyIndex)->setText(tr("Biography"));
}

void CharacterInformationStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
