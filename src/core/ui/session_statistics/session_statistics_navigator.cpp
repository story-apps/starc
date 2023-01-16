#include "session_statistics_navigator.h"

#include "counter_widget.h"
#include "overview_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/time_helper.h>

#include <QDateTime>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>


namespace Ui {

class SessionStatisticsNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить счётчик текущей сессии
     */
    void updateCurrentSessionDuration();


    QScrollArea* content = nullptr;

    H6Label* currentSessionTitle = nullptr;
    CounterWidget* currentSessionWordsCounter = nullptr;
    CounterWidget* currentSessionDurationCounter = nullptr;
    QDateTime currentSessionStartedAt;
    QTimer currentSessionDurationTimer;
    CounterWidget* currentSessionStartedAtCounter = nullptr;
    H6Label* sessionOverviewTitle = nullptr;
    OverviewWidget* sessionDuraionOverview = nullptr;
    OverviewWidget* sessionWordsOverview = nullptr;
};

SessionStatisticsNavigator::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , currentSessionTitle(new H6Label(_parent))
    , currentSessionWordsCounter(new CounterWidget(_parent))
    , currentSessionDurationCounter(new CounterWidget(_parent))
    , currentSessionStartedAtCounter(new CounterWidget(_parent))
    , sessionOverviewTitle(new H6Label(_parent))
    , sessionDuraionOverview(new OverviewWidget(_parent))
    , sessionWordsOverview(new OverviewWidget(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    currentSessionDurationTimer.setInterval(std::chrono::seconds{ 1 });
    currentSessionStartedAtCounter->setValueStyle(CounterWidget::ValueStyle::TwoLines);

    auto contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    auto layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(currentSessionTitle);
    layout->addWidget(currentSessionWordsCounter);
    layout->addWidget(currentSessionDurationCounter);
    layout->addWidget(currentSessionStartedAtCounter);
    layout->addWidget(sessionOverviewTitle);
    layout->addWidget(sessionDuraionOverview);
    layout->addWidget(sessionWordsOverview);
    layout->addStretch();
}

void SessionStatisticsNavigator::Implementation::updateCurrentSessionDuration()
{
    if (!currentSessionStartedAt.isValid()) {
        return;
    }

    currentSessionDurationCounter->setValue(TimeHelper::toString(
        std::chrono::seconds{ currentSessionStartedAt.secsTo(QDateTime::currentDateTime()) }));
}


// ****


SessionStatisticsNavigator::SessionStatisticsNavigator(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(this, &SessionStatisticsNavigator::appeared, &d->currentSessionDurationTimer,
            qOverload<>(&QTimer::start));
    connect(this, &SessionStatisticsNavigator::disappeared, &d->currentSessionDurationTimer,
            &QTimer::stop);
    connect(&d->currentSessionDurationTimer, &QTimer::timeout, this,
            [this] { d->updateCurrentSessionDuration(); });
}

SessionStatisticsNavigator::~SessionStatisticsNavigator() = default;

void SessionStatisticsNavigator::setCurrentSessionDetails(int _words, const QDateTime& _startedAt)
{
    if (!_startedAt.isValid()) {
        d->currentSessionWordsCounter->setValue({});
        d->currentSessionDurationCounter->setValue({});
        d->currentSessionStartedAtCounter->setValue({});
        return;
    }

    d->currentSessionWordsCounter->setValue(QString::number(_words));
    d->currentSessionStartedAt = _startedAt;
    d->updateCurrentSessionDuration();
    auto sessionStartedAt = [_startedAt] {
        switch (_startedAt.daysTo(QDateTime::currentDateTime())) {
        case 0: {
            return tr("today");
        }

        case 1: {
            return tr("yesterday");
        }

        default: {
            return _startedAt.toString("dd.MM.yyyy");
        }
        }
    };
    d->currentSessionStartedAtCounter->setValue(
        d->currentSessionStartedAt.toString("%1\n%2 hh:mm").arg(sessionStartedAt(), tr("at")));
}

void SessionStatisticsNavigator::set30DaysOverviewDetails(std::chrono::seconds _minSessionDuration,
                                                          std::chrono::seconds _maxSessionDuration,
                                                          int _minWords, int _maxWords)
{
    auto percent = [](int _val, int _max) {
        if (_max == 0) {
            return 0;
        }
        return _val * 100 / _max;
    };

    const auto avgSessionDuration = (_maxSessionDuration + _minSessionDuration) / 2;
    d->sessionDuraionOverview->setValues(TimeHelper::toLongString(_minSessionDuration),
                                         TimeHelper::toLongString(avgSessionDuration),
                                         TimeHelper::toLongString(_maxSessionDuration));
    d->sessionDuraionOverview->setProgress(
        percent(_minSessionDuration.count(), _maxSessionDuration.count()),
        percent((avgSessionDuration).count(), _maxSessionDuration.count()), 100,
        TimeHelper::toLongString(_maxSessionDuration));

    const auto avgWords = (_maxWords + _minWords) / 2;
    d->sessionWordsOverview->setValues(
        QString("%1 %2").arg(_minWords).arg(tr("word(s)", 0, _minWords)),
        QString("%1 %2").arg(avgWords).arg(tr("word(s)", 0, avgWords)),
        QString("%1 %2").arg(_maxWords).arg(tr("word(s)", 0, _maxWords)));
    d->sessionWordsOverview->setProgress(percent(_minWords, _maxWords),
                                         percent(avgWords, _maxWords), 100,
                                         QString::number(_maxWords));
}

void SessionStatisticsNavigator::updateTranslations()
{
    d->currentSessionTitle->setText(tr("Current session"));
    d->currentSessionWordsCounter->setLabel(tr("WORDS\nWRITTEN"));
    d->currentSessionDurationCounter->setLabel(tr("SESSION\nDURATION"));
    d->currentSessionStartedAtCounter->setLabel(tr("SESSION\nSTARTED"));
    d->sessionOverviewTitle->setText(tr("30 days overview"));
    d->sessionDuraionOverview->setTitle(u8"\U000F0150", tr("Session duration"));
    d->sessionDuraionOverview->setLabels(tr("THE LOWEST"), tr("AVERAGE"), tr("THE HIGHEST"));
    d->sessionWordsOverview->setTitle(u8"\U000F03EB", tr("Words written"));
    d->sessionWordsOverview->setLabels(tr("THE LOWEST"), tr("AVERAGE"), tr("THE HIGHEST"));
}

void SessionStatisticsNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    for (auto label : std::vector<Widget*>{
             d->currentSessionTitle,
             d->sessionOverviewTitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().primary());
        label->setTextColor(DesignSystem::color().onPrimary());
    }
    d->currentSessionTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px4(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16());
    d->sessionOverviewTitle->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16());


    for (auto counter : {
             d->currentSessionWordsCounter,
             d->currentSessionDurationCounter,
             d->currentSessionStartedAtCounter,
         }) {
        counter->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        counter->setTextColor(DesignSystem::color().onPrimary());
        counter->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                    Ui::DesignSystem::layout().px24(),
                                    Ui::DesignSystem::layout().px4());
    }
    d->currentSessionWordsCounter->setColor(ColorHelper::forNumber(0));
    d->currentSessionDurationCounter->setColor(ColorHelper::forNumber(1));
    d->currentSessionStartedAtCounter->setColor(ColorHelper::forNumber(2));
    for (auto overview : {
             d->sessionDuraionOverview,
             d->sessionWordsOverview,
         }) {
        overview->setBackgroundColor(DesignSystem::color().primary());
        overview->setTextColor(DesignSystem::color().onPrimary());
        overview->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                     Ui::DesignSystem::layout().px24(),
                                     Ui::DesignSystem::layout().px24());
    }
    d->sessionDuraionOverview->setColors(
        ColorHelper::forNumber(4),
        ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                 Ui::DesignSystem::inactiveTextOpacity()),
        ColorHelper::forNumber(2));
    d->sessionWordsOverview->setColors(
        ColorHelper::forNumber(4),
        ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                 Ui::DesignSystem::inactiveTextOpacity()),
        ColorHelper::forNumber(0));
}

} // namespace Ui
