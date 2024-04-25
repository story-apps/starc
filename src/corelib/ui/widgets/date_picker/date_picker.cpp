#include "date_picker.h"

#include "month_days_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QDate>

namespace {
constexpr auto kDaysInWeek = 7;
}


class DatePicker::Implementation
{
public:
    explicit Implementation(DatePicker* _q);

    QString monthName(int _month) const;

    void updateCurrentDate();


    /**
     * @brief Текущий месяц, который отображается на экране
     */
    QDate currentDate;

    /**
     * @brief Выбранная дата
     */
    QDate selectedDate;

    AbstractLabel* monthAndYearLabel = nullptr;
    IconButton* selectYearButton = nullptr;
    IconButton* prevMonthButton = nullptr;
    IconButton* nextMonthButton = nullptr;
    StackWidget* content = nullptr;
    Widget* yearsContent = nullptr;
    Widget* monthDaysContent = nullptr;
    QVector<AbstractLabel*> weekDayTitles;
    StackWidget* monthDays = nullptr;
    MonthDaysWidget* prevMonthDays = nullptr;
    MonthDaysWidget* currentMonthDays = nullptr;
    MonthDaysWidget* nextMonthDays = nullptr;
};

DatePicker::Implementation::Implementation(DatePicker* _q)
    : monthAndYearLabel(new Body2Label(_q))
    , selectYearButton(new IconButton(_q))
    , prevMonthButton(new IconButton(_q))
    , nextMonthButton(new IconButton(_q))
    , content(new StackWidget(_q))
    , yearsContent(new Widget(_q))
    , monthDaysContent(new Widget(_q))
    , monthDays(new StackWidget(_q))
    , prevMonthDays(new MonthDaysWidget(_q))
    , currentMonthDays(new MonthDaysWidget(_q))
    , nextMonthDays(new MonthDaysWidget(_q))
{
    //
    // TODO: реализовать выбор года
    //
    selectYearButton->hide();

    selectYearButton->setIcon(u8"\U000F035D");
    prevMonthButton->setIcon(u8"\U000F0141");
    nextMonthButton->setIcon(u8"\U000F0142");

    content->setAnimationType(StackWidget::AnimationType::Fade);
    content->addWidget(yearsContent);
    content->addWidget(monthDaysContent);
    content->setCurrentWidget(monthDaysContent);

    monthDays->setAnimationType(StackWidget::AnimationType::Slide);
    monthDays->addWidget(prevMonthDays);
    monthDays->addWidget(currentMonthDays);
    monthDays->addWidget(nextMonthDays);
    monthDays->setCurrentWidget(currentMonthDays);

    auto weekTitleLayout = new QHBoxLayout;
    weekTitleLayout->setContentsMargins({});
    weekTitleLayout->setSpacing(0);
    for (int i = 0; i < kDaysInWeek; ++i) {
        auto weekDayTitle = new CaptionLabel(_q);
        weekDayTitle->setAlignment(Qt::AlignCenter);
        weekDayTitles.append(weekDayTitle);
        weekTitleLayout->addWidget(weekDayTitle);
    }
    auto monthDaysLayout = new QVBoxLayout;
    monthDaysLayout->setContentsMargins({});
    monthDaysLayout->setSpacing(0);
    monthDaysLayout->addLayout(weekTitleLayout);
    monthDaysLayout->addWidget(monthDays, 1);
    monthDaysContent->setLayout(monthDaysLayout);
}

QString DatePicker::Implementation::monthName(int _month) const
{
    switch (_month) {
    case 1: {
        return tr("January");
    }
    case 2: {
        return tr("February");
    }
    case 3: {
        return tr("March");
    }
    case 4: {
        return tr("April");
    }
    case 5: {
        return tr("May");
    }
    case 6: {
        return tr("June");
    }
    case 7: {
        return tr("July");
    }
    case 8: {
        return tr("August");
    }
    case 9: {
        return tr("September");
    }
    case 10: {
        return tr("October");
    }
    case 11: {
        return tr("November");
    }
    case 12: {
        return tr("December");
    }
    default: {
        Q_ASSERT(false);
        return {};
    }
    }
}

void DatePicker::Implementation::updateCurrentDate()
{
    monthAndYearLabel->setText(
        QString("%1 %2").arg(monthName(currentDate.month()), QString::number(currentDate.year())));
}


// ****


DatePicker::DatePicker(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setCurrentDate(QDate::currentDate());
    setSelectedDate(QDate::currentDate());

    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->monthAndYearLabel);
    topLayout->addWidget(d->selectYearButton);
    topLayout->addStretch();
    topLayout->addWidget(d->prevMonthButton);
    topLayout->addWidget(d->nextMonthButton);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->content, 1);
    setLayout(layout);

    connect(d->prevMonthButton, &IconButton::clicked, this, [this] {
        //
        // next -> current
        // current -> prev
        // prev -> moved next
        //
        d->monthDays->takeWidget(d->nextMonthDays);
        d->monthDays->prependWidget(d->nextMonthDays);
        std::swap(d->nextMonthDays, d->currentMonthDays);
        std::swap(d->currentMonthDays, d->prevMonthDays);

        setCurrentDate(d->currentDate.addMonths(-1));

        d->monthDays->setCurrentWidget(d->currentMonthDays);
    });
    connect(d->nextMonthButton, &IconButton::clicked, this, [this] {
        //
        // prev -> current
        // current -> next
        // next -> moved prev
        //
        d->monthDays->takeWidget(d->prevMonthDays);
        d->monthDays->addWidget(d->prevMonthDays);
        std::swap(d->prevMonthDays, d->currentMonthDays);
        std::swap(d->currentMonthDays, d->nextMonthDays);

        setCurrentDate(d->currentDate.addMonths(1));

        d->monthDays->setCurrentWidget(d->currentMonthDays);
    });
    auto processDateChange = [this](const QDate& _date) {
        setSelectedDate(_date);
        if (d->currentDate.month() > _date.month()) {
            d->prevMonthButton->click();
        } else if (d->currentDate.month() < _date.month()) {
            d->nextMonthButton->click();
        }
        setCurrentDate(_date);

        emit selectedDateChanged(_date);
    };
    connect(d->prevMonthDays, &MonthDaysWidget::selectedDateChanged, this, processDateChange);
    connect(d->currentMonthDays, &MonthDaysWidget::selectedDateChanged, this, processDateChange);
    connect(d->nextMonthDays, &MonthDaysWidget::selectedDateChanged, this, processDateChange);
}

DatePicker::~DatePicker() = default;

void DatePicker::setCurrentDate(const QDate& _date)
{
    d->currentDate = _date;
    d->prevMonthDays->setCurrentDate(_date.addMonths(-1));
    d->currentMonthDays->setCurrentDate(_date);
    d->nextMonthDays->setCurrentDate(_date.addMonths(1));
    d->updateCurrentDate();
}

QDate DatePicker::selectedDate() const
{
    return d->selectedDate;
}

void DatePicker::setSelectedDate(const QDate& _date)
{
    d->selectedDate = _date;
    d->prevMonthDays->setSelectedDate(d->selectedDate);
    d->currentMonthDays->setSelectedDate(d->selectedDate);
    d->nextMonthDays->setSelectedDate(d->selectedDate);
    d->updateCurrentDate();
}

void DatePicker::processBackgroundColorChange()
{
    for (auto widget : std::vector<Widget*>{
             d->monthAndYearLabel,
             d->selectYearButton,
             d->prevMonthButton,
             d->nextMonthButton,
             d->content,
             d->yearsContent,
             d->monthDaysContent,
             d->monthDays,
             d->prevMonthDays,
             d->currentMonthDays,
             d->nextMonthDays,
         }) {
        widget->setBackgroundColor(backgroundColor());
    }
    for (auto widget : std::as_const(d->weekDayTitles)) {
        widget->setBackgroundColor(backgroundColor());
    }
}

void DatePicker::processTextColorChange()
{
    for (auto widget : std::vector<Widget*>{
             d->monthAndYearLabel,
             d->selectYearButton,
             d->prevMonthButton,
             d->nextMonthButton,
             d->content,
             d->yearsContent,
             d->monthDaysContent,
             d->monthDays,
             d->prevMonthDays,
             d->currentMonthDays,
             d->nextMonthDays,
         }) {
        widget->setTextColor(textColor());
    }
    for (auto widget : std::as_const(d->weekDayTitles)) {
        widget->setTextColor(
            ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity()));
    }
}

void DatePicker::updateTranslations()
{
    int dayIndex = 0;
    for (const auto& dayName : {
             tr("Mon"),
             tr("Tue"),
             tr("Wed"),
             tr("Thu"),
             tr("Fri"),
             tr("Sat"),
             tr("Sun"),
         }) {
        d->weekDayTitles[dayIndex]->setText(dayName);
        ++dayIndex;
    }

    d->updateCurrentDate();
}

void DatePicker::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->monthAndYearLabel->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16(),
        Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px12());
    d->monthDaysContent->layout()->setContentsMargins(Ui::DesignSystem::layout().px16(), 0,
                                                      Ui::DesignSystem::layout().px16(),
                                                      Ui::DesignSystem::layout().px8());
    for (auto widget : std::as_const(d->weekDayTitles)) {
        widget->setMinimumSize(Ui::DesignSystem::layout().px(32),
                               Ui::DesignSystem::layout().px(32));
    }
}
