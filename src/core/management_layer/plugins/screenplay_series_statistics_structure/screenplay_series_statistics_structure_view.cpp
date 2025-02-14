#include "screenplay_series_statistics_structure_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
enum {
    kSummaryReportIndex = 0,
    kSceneReportIndex,
    kLocationReportIndex,
    kCastReportIndex,
    kDialoguesReportIndex,
};
enum {
    kCharactersActivityPlotIndex = 0,
};
} // namespace

class ScreenplayStatisticsStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    ButtonLabel* reportsTitle = nullptr;
    Tree* reports = nullptr;
    ButtonLabel* plotsTitle = nullptr;
    Tree* plots = nullptr;
};

ScreenplayStatisticsStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , reportsTitle(new ButtonLabel(_parent))
    , reports(new Tree(_parent))
    , plotsTitle(new ButtonLabel(_parent))
    , plots(new Tree(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    reports->setContextMenuPolicy(Qt::NoContextMenu);
    reports->setSelectionMode(QAbstractItemView::SingleSelection);
    plots->setContextMenuPolicy(Qt::NoContextMenu);
    plots->setSelectionMode(QAbstractItemView::SingleSelection);

    auto createItem = [](const QString& _icon) {
        auto item = new QStandardItem;
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    auto reportsModel = new QStandardItemModel(reports);
    reportsModel->appendRow(createItem(u8"\U000F014D"));
    reportsModel->appendRow(createItem(u8"\U000F014D"));
    reportsModel->appendRow(createItem(u8"\U000F014D"));
    reportsModel->appendRow(createItem(u8"\U000F014D"));
    reportsModel->appendRow(createItem(u8"\U000F014D"));
    reports->setModel(reportsModel);
    reports->setCurrentIndex(reportsModel->index(0, 0));
    //
    auto plotsModel = new QStandardItemModel(reports);
    plotsModel->appendRow(createItem(u8"\U000F012A"));
    plots->setModel(plotsModel);
}


// ****


ScreenplayStatisticsStructureView::ScreenplayStatisticsStructureView(QWidget* _parent)
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
    layout->addWidget(d->reportsTitle);
    layout->addWidget(d->reports);
    layout->addWidget(d->plotsTitle);
    layout->addWidget(d->plots);
    layout->addStretch();
    setLayout(layout);


    connect(d->backIcon, &AbstractLabel::clicked, this,
            &ScreenplayStatisticsStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this,
            &ScreenplayStatisticsStructureView::backPressed);
    connect(d->reports, &Tree::clicked, this,
            &ScreenplayStatisticsStructureView::currentReportIndexChanged);
    connect(d->reports, &Tree::doubleClicked, this,
            &ScreenplayStatisticsStructureView::currentReportIndexChanged);
    connect(this, &ScreenplayStatisticsStructureView::currentReportIndexChanged, this, [this] {
        QSignalBlocker signalBlocker(d->plots);
        d->plots->setCurrentIndex({});
    });
    connect(d->plots, &Tree::clicked, this,
            &ScreenplayStatisticsStructureView::currentPlotIndexChanged);
    connect(d->plots, &Tree::doubleClicked, this,
            &ScreenplayStatisticsStructureView::currentPlotIndexChanged);
    connect(this, &ScreenplayStatisticsStructureView::currentPlotIndexChanged, this, [this] {
        QSignalBlocker signalBlocker(d->reports);
        d->reports->setCurrentIndex({});
    });
}

ScreenplayStatisticsStructureView::~ScreenplayStatisticsStructureView() = default;

QWidget* ScreenplayStatisticsStructureView::asQWidget()
{
    return this;
}

void ScreenplayStatisticsStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void ScreenplayStatisticsStructureView::updateTranslations()
{
    d->reportsTitle->setText(tr("Reports"));
    auto reportsModel = qobject_cast<QStandardItemModel*>(d->reports->model());
    reportsModel->item(kSummaryReportIndex)->setText(tr("Summary statistics"));
    reportsModel->item(kSceneReportIndex)->setText(tr("Scene report"));
    reportsModel->item(kLocationReportIndex)->setText(tr("Location report"));
    reportsModel->item(kCastReportIndex)->setText(tr("Cast report"));
    reportsModel->item(kDialoguesReportIndex)->setText(tr("Dialogues report"));
    //
    d->plotsTitle->setText(tr("Plots"));
    auto plotsModel = qobject_cast<QStandardItemModel*>(d->plots->model());
    plotsModel->item(kCharactersActivityPlotIndex)->setText(tr("Characters activity"));
}

void ScreenplayStatisticsStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    auto backTextColor = DesignSystem::color().onPrimary();
    backTextColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    for (auto widget : std::vector<Widget*>{
             d->backIcon,
             d->backText,
             d->reportsTitle,
             d->plotsTitle,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(backTextColor);
    }
    d->reports->setBackgroundColor(DesignSystem::color().primary());
    d->reports->setTextColor(DesignSystem::color().onPrimary());
    d->reports->setFixedHeight(DesignSystem::treeOneLineItem().height()
                               * d->reports->model()->rowCount());
    d->plots->setBackgroundColor(DesignSystem::color().primary());
    d->plots->setTextColor(DesignSystem::color().onPrimary());
    d->plots->setFixedHeight(DesignSystem::treeOneLineItem().height()
                             * d->plots->model()->rowCount());

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

    auto titleMargins = DesignSystem::label().margins();
    titleMargins.setLeft(DesignSystem::layout().px(48));
    titleMargins.setBottom(DesignSystem::layout().px8());
    d->reportsTitle->setContentsMargins(titleMargins.toMargins());
    d->plotsTitle->setContentsMargins(titleMargins.toMargins());
}

} // namespace Ui
