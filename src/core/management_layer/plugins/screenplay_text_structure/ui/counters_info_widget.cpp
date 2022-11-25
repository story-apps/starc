#include "counters_info_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>


namespace Ui {

class CountersInfoWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить текст лейбла со счётчиками в соответствии с выбранными настройками
     */
    void updateCountersLabelText();


    AbstractLabel* countersLabel = nullptr;
    AbstractLabel* showCountersMenuLabel = nullptr;

    struct CounterInfo {
        QString info = {};
        bool isVisible = true;
    };
    QVector<CounterInfo> counters;
};

CountersInfoWidget::Implementation::Implementation(QWidget* _parent)
    : countersLabel(new Subtitle2Label(_parent))
    , showCountersMenuLabel(new IconsMidLabel(_parent))
{
    showCountersMenuLabel->setText(u8"\U000F035D");
}

void CountersInfoWidget::Implementation::updateCountersLabelText()
{
    QString countersText;
    for (const auto& counter : std::as_const(counters)) {
        if (!counter.isVisible || counter.info.isEmpty()) {
            continue;
        }

        if (!countersText.isEmpty()) {
            countersText += "; ";
        }
        countersText += counter.info;
    }
    countersLabel->setText(countersText);
}


// ****


CountersInfoWidget::CountersInfoWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->countersLabel);
    layout->addWidget(d->showCountersMenuLabel);
    layout->addStretch();
    setLayout(layout);


    auto showCountersMenu = [this] {

    };
    connect(d->countersLabel, &AbstractLabel::clicked, this, showCountersMenu);
    connect(d->showCountersMenuLabel, &AbstractLabel::clicked, this, showCountersMenu);
}

CountersInfoWidget::~CountersInfoWidget() = default;

void CountersInfoWidget::setCounters(const QVector<QString>& _counters)
{
    if (d->counters.size() != _counters.size()) {
        d->counters.resize(_counters.size());
    }

    for (int index = 0; index < d->counters.size(); ++index) {
        d->counters[index].info = _counters[index];
    }

    d->updateCountersLabelText();
}

void CountersInfoWidget::updateTranslations()
{
    d->showCountersMenuLabel->setToolTip(tr("Choose counters to show"));
}

void CountersInfoWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    for (auto label : {
             d->countersLabel,
             d->showCountersMenuLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().primary());
        label->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                     Ui::DesignSystem::inactiveTextOpacity()));
        label->setContentsMargins(0, 0, Ui::DesignSystem::layout().px12(), 0);
    }

    layout()->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24());
}

} // namespace Ui
