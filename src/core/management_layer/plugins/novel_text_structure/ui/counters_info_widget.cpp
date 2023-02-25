#include "counters_info_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QBoxLayout>
#include <QSettings>


namespace Ui {

namespace {

QString settingsCounterKey(int _index)
{
    return QString("widgets/counters-info-widget/novel/%1").arg(_index);
}

} // namespace

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
        bool isVisible = false;
    };
    QVector<CounterInfo> counters;
};

CountersInfoWidget::Implementation::Implementation(QWidget* _parent)
    : countersLabel(new Subtitle2Label(_parent))
    , showCountersMenuLabel(new IconsMidLabel(_parent))
{
    showCountersMenuLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
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
            countersText += " | ";
        }
        countersText += counter.info;
    }

    if (countersText.isEmpty()) {
        countersText = tr("No counter selected");
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
        QVector<QAction*> menuActions;
        for (int index = 0; index < d->counters.size(); ++index) {
            auto action = new QAction(this);
            action->setCheckable(true);
            action->setText(d->counters[index].info);
            action->setChecked(d->counters[index].isVisible);
            action->setIconText(d->counters[index].isVisible ? u8"\U000F012C" : u8"\U000F68C0");
            connect(action, &QAction::toggled, this, [this, action, index](bool _checked) {
                d->counters[index].isVisible = _checked;
                action->setIconText(_checked ? u8"\U000F012C" : u8"\U000F68C0");
                d->updateCountersLabelText();

                QSettings().setValue(settingsCounterKey(index), _checked);
            });

            menuActions.append(action);
        }

        auto menu = new ContextMenu(this);
        menu->setActions(menuActions);
        menu->setBackgroundColor(Ui::DesignSystem::color().background());
        menu->setTextColor(Ui::DesignSystem::color().onBackground());
        connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

        menu->showContextMenu(
            mapToGlobal(rect().topLeft() + QPoint(this->layout()->contentsMargins().left(), 0)));
    };
    connect(d->countersLabel, &AbstractLabel::clicked, this, showCountersMenu);
    connect(d->showCountersMenuLabel, &AbstractLabel::clicked, this, showCountersMenu);
}

CountersInfoWidget::~CountersInfoWidget() = default;

void CountersInfoWidget::setCounters(const QVector<QString>& _counters)
{
    Q_ASSERT(!_counters.isEmpty());

    //
    // Настроим размер контейнера со счётчиками
    //
    if (d->counters.size() != _counters.size()) {
        d->counters.resize(_counters.size());
        //
        // ... если это первое использование, включим отображение первого из счётчиков
        //
        d->counters[0].isVisible = QSettings().value(settingsCounterKey(0), true).toBool();
        for (int index = 1; index < d->counters.size(); ++index) {
            d->counters[index].isVisible
                = QSettings().value(settingsCounterKey(index), false).toBool();
        }
    }

    for (int index = 0; index < d->counters.size(); ++index) {
        d->counters[index].info = _counters[index];
    }

    d->updateCountersLabelText();
}

void CountersInfoWidget::updateTranslations()
{
    d->updateCountersLabelText();
    const auto tooltip = tr("Click to select counters to show");
    d->showCountersMenuLabel->setToolTip(tooltip);
    d->showCountersMenuLabel->setToolTip(tooltip);
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
    }

    layout()->setContentsMargins(
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px12());
}

} // namespace Ui
