#include "session_statistics_view.h"

#include <business_layer/plots/screenplay/screenplay_structure_analysis_plot.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/plot/plot.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QScrollArea>


namespace Ui {

class SessionStatisticsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить размер графика
     */
    void updatePlotSize();


    QScrollArea* content = nullptr;

    QHBoxLayout* filtersLayout = nullptr;
    CheckBox* showAllDevices = nullptr;
    QVector<Widget*> devices;
    Subtitle2LinkLabel* weekPlotFilter = nullptr;
    Subtitle2LinkLabel* monthPlotFilter = nullptr;
    Subtitle2LinkLabel* yearPlotFilter = nullptr;

    Card* plotCard = nullptr;
    QVBoxLayout* plotCardLayout = nullptr;
    Plot* plot = nullptr;
};

SessionStatisticsView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , filtersLayout(new QHBoxLayout)
    , showAllDevices(new CheckBox(_parent))
    , weekPlotFilter(new Subtitle2LinkLabel(_parent))
    , monthPlotFilter(new Subtitle2LinkLabel(_parent))
    , yearPlotFilter(new Subtitle2LinkLabel(_parent))
    , plotCard(new Card(_parent))
    , plotCardLayout(new QVBoxLayout)
    , plot(new Plot(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    weekPlotFilter->hide();
    monthPlotFilter->hide();
    yearPlotFilter->hide();

    //
    // Настроим легенду
    //
    plot->legend->setVisible(false);
    //
    // Настроим оси координат
    //
    plot->xAxis2->setVisible(true);
    plot->xAxis2->setTickLabels(false);
    plot->yAxis2->setVisible(true);
    plot->yAxis2->setTickLabels(true);
    //
    // Настроим возможности взаимодействия с графиком
    //
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    plot->axisRect()->setRangeZoom(Qt::Horizontal);
    plot->axisRect()->setRangeDragAxes(plot->xAxis, 0);
    plot->setNoAntialiasingOnDrag(true);
    //
    // Настроить оси
    //
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("dd.MM.yyyy");
    plot->xAxis->setTicker(dateTicker);
    plot->xAxis->setTickPen(Qt::NoPen);
    plot->xAxis->setSubTickPen(Qt::NoPen);
    plot->xAxis->grid()->setPen(Qt::NoPen);
    plot->xAxis->grid()->setSubGridVisible(false);
    plot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    plot->xAxis2->setBasePen(Qt::NoPen);
    plot->xAxis2->setTickPen(Qt::NoPen);
    plot->xAxis2->setSubTickPen(Qt::NoPen);
    //
    plot->yAxis->setTickPen(Qt::NoPen);
    plot->yAxis->setSubTickPen(Qt::NoPen);
    plot->yAxis->grid()->setSubGridVisible(false);
    plot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    plot->yAxis2->setBasePen(Qt::NoPen);
    plot->yAxis2->setTickPen(Qt::NoPen);
    plot->yAxis2->setSubTickPen(Qt::NoPen);
    plot->axisRect()->setAutoMargins(QCP::msLeft | QCP::msBottom);

    plotCardLayout->setContentsMargins({});
    plotCardLayout->setSpacing(0);
    plotCardLayout->addWidget(plot);
    plotCard->setContentLayout(plotCardLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    {
        filtersLayout->setContentsMargins({});
        filtersLayout->setSpacing(0);
        filtersLayout->addWidget(showAllDevices);
        filtersLayout->addStretch();
        filtersLayout->addWidget(weekPlotFilter);
        filtersLayout->addWidget(monthPlotFilter);
        filtersLayout->addWidget(yearPlotFilter);
        layout->addLayout(filtersLayout);
    }
    layout->addWidget(plotCard);
    layout->addStretch();
    contentWidget->setLayout(layout);
}

void SessionStatisticsView::Implementation::updatePlotSize()
{
    plot->setFixedSize(content->width() - Ui::DesignSystem::layout().px24() * 4
                           - Ui::DesignSystem::card().shadowMargins().left()
                           - Ui::DesignSystem::card().shadowMargins().right(),
                       content->height() - Ui::DesignSystem::layout().topContentMargin()
                           - Ui::DesignSystem::layout().px24() * 3
                           - Ui::DesignSystem::card().shadowMargins().top()
                           - Ui::DesignSystem::card().shadowMargins().bottom());
}


// ****


SessionStatisticsView::SessionStatisticsView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->showAllDevices, &CheckBox::checkedChanged, this,
            &SessionStatisticsView::viewPreferencesChanged);
}

SessionStatisticsView::~SessionStatisticsView() = default;

bool SessionStatisticsView::showDevices() const
{
    return d->showAllDevices->isChecked();
}

void SessionStatisticsView::setPlot(const BusinessLayer::Plot& _plot)
{
    //
    // Очищаем фильтры
    //
    while (!d->devices.isEmpty()) {
        auto device = d->devices.takeFirst();
        device->hide();
        device->deleteLater();
    }

    //
    // Очищаем график
    //
    d->plot->clearGraphs();

    //
    // Загружаем информацию и данные
    //
    d->plot->setPlotInfo(_plot.info);
    qreal minX = std::numeric_limits<int>::max();
    qreal maxX = 0;
    qreal maxY = 0;
    for (const auto& singlePlotData : _plot.data) {
        //
        // Добавляем график и настраиваем его
        //
        QCPGraph* plot = d->plot->addGraph();
        plot->setSelectable(QCP::stNone);
        plot->setName(singlePlotData.name);
        plot->setPen(QPen(singlePlotData.color, Ui::DesignSystem::layout().px2()));
        if (singlePlotData.brushColor.isValid()) {
            plot->setBrush(singlePlotData.brushColor);
        }
        plot->setScatterStyle(
            QCPScatterStyle(QCPScatterStyle::ssCircle,
                            QPen(ColorHelper::transparent(singlePlotData.color,
                                                          Ui::DesignSystem::inactiveItemOpacity()),
                                 Ui::DesignSystem::layout().px(5)),
                            Qt::white, Ui::DesignSystem::layout().px8()));

        //
        // Отправляем данные в график
        //
        plot->setData(singlePlotData.x, singlePlotData.y);

        //
        // Определяем границы
        //
        for (const qreal& x : singlePlotData.x) {
            if (x < minX) {
                minX = x;
            }
            if (x > maxX) {
                maxX = x;
            }
        }
        for (const qreal& y : singlePlotData.y) {
            if (!std::isnan(y) && y > maxY) {
                maxY = y;
            }
        }


        if (d->showAllDevices->isChecked()) {
            auto deviceNameWidget = new Subtitle2Label(this);
            deviceNameWidget->setText(singlePlotData.name);
            deviceNameWidget->setTextColor(singlePlotData.color);
            deviceNameWidget->setBackgroundColor(Ui::DesignSystem::color().surface());
            deviceNameWidget->setContentsMargins(
                isLeftToRight() ? 0 : DesignSystem::layout().px16(), 0,
                isLeftToRight() ? DesignSystem::layout().px16() : 0, 0);

            d->filtersLayout->insertWidget(1 + d->devices.size(), deviceNameWidget, 0,
                                           Qt::AlignCenter);
            d->devices.append(deviceNameWidget);
        }
    }

    //
    // Масштабируем график
    //
    d->plot->xAxis->setRangeUpper(maxX);
    d->plot->xAxis->setRangeLower(minX);
    d->plot->yAxis->setRangeUpper(maxY + maxY * 0.05);
    d->plot->yAxis->setRangeLower(0);

    //
    // Возвращаем градиент
    //
    QLinearGradient backgroundGradient(0, 0, 0, d->plot->height());
    backgroundGradient.setColorAt(0, Qt::transparent);
    backgroundGradient.setColorAt(
        1,
        ColorHelper::transparent(Ui::DesignSystem::color().accent(),
                                 Ui::DesignSystem::hoverBackgroundOpacity()));
    d->plot->axisRect()->setBackground(backgroundGradient);

    d->plot->replot();
}

void SessionStatisticsView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->updatePlotSize();
    //    d->updateTablesGeometry();
}

void SessionStatisticsView::updateTranslations()
{
    d->showAllDevices->setText(tr("Separate statistics by devices"));
    d->weekPlotFilter->setText(tr("Dayly"));
    d->monthPlotFilter->setText(tr("Monthly"));
    d->yearPlotFilter->setText(tr("Yearly"));
    d->plot->yAxis->setLabel(tr("Words"));
    d->plot->yAxis2->setLabel(tr("Time, min"));
}

void SessionStatisticsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->showAllDevices->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->showAllDevices->setTextColor(Ui::DesignSystem::color().onSurface());
    for (auto checkBox : std::as_const(d->devices)) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().surface());
        checkBox->setTextColor(Ui::DesignSystem::color().onSurface());
    }
    for (auto filter : {
             d->weekPlotFilter,
             d->monthPlotFilter,
             d->yearPlotFilter,
         }) {
        filter->setBackgroundColor(DesignSystem::color().surface());
        filter->setTextColor(DesignSystem::color().accent());
        filter->setContentsMargins(Ui::DesignSystem::layout().px8(), 0,
                                   Ui::DesignSystem::layout().px8(), 0);
    }
    d->yearPlotFilter->setContentsMargins(Ui::DesignSystem::layout().px8(), 0,
                                          Ui::DesignSystem::layout().px24(), 0);

    for (auto card : { d->plotCard }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }
    for (auto cardLayout : { d->plotCardLayout }) {
        cardLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(), 0.0, 0.0,
                                                 Ui::DesignSystem::layout().px24())
                                           .toMargins());
    }

    d->updatePlotSize();
    d->plot->xAxis->setLabelFont(Ui::DesignSystem::font().subtitle2());
    d->plot->xAxis->setBasePen(
        QPen(Ui::DesignSystem::color().background(), Ui::DesignSystem::layout().px()));
    d->plot->yAxis->setBasePen(
        QPen(Ui::DesignSystem::color().background(), Ui::DesignSystem::layout().px()));
    d->plot->xAxis->setTickLabelColor(Ui::DesignSystem::color().onBackground());
    d->plot->yAxis->setTickLabelColor(Ui::DesignSystem::color().onBackground());
    d->plot->yAxis->grid()->setPen(
        QPen(ColorHelper::transparent(Ui::DesignSystem::color().accent(),
                                      Ui::DesignSystem::focusBackgroundOpacity()),
             Ui::DesignSystem::layout().px(), Qt::DashLine));
    d->plot->xAxis->setLabelColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::inactiveTextOpacity()));
    d->plot->yAxis->setLabelColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::inactiveTextOpacity()));
    d->plot->setBackground(Ui::DesignSystem::color().background());
    QLinearGradient backgroundGradient(0, 0, 0, d->plot->height());
    backgroundGradient.setColorAt(0, Qt::transparent);
    backgroundGradient.setColorAt(
        1,
        ColorHelper::transparent(Ui::DesignSystem::color().accent(),
                                 Ui::DesignSystem::hoverBackgroundOpacity()));
    d->plot->axisRect()->setBackground(backgroundGradient);
    d->plot->replot();
}

} // namespace Ui
