#include "month_days_widget.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QDate>
#include <QPaintEvent>
#include <QPainter>


class MonthDaysWidget::Implementation
{
public:
    explicit Implementation(MonthDaysWidget* _q);

    /**
     * @brief Сформировать дни текущего месяца
     */
    void buildDays();


    MonthDaysWidget* q = nullptr;

    struct DateItem {
        bool operator==(const DateItem& _other) const
        {
            return day == _other.day && rect == _other.rect;
        }

        QDate day;
        QRectF rect;
    };
    QVector<DateItem> days;
    QDate currentDate;
    QDate selectedDate;
};

MonthDaysWidget::Implementation::Implementation(MonthDaysWidget* _q)
    : q(_q)
{
}

void MonthDaysWidget::Implementation::buildDays()
{
    days.clear();

    constexpr int daysInWeek = 7;
    const qreal dayWidth
        = std::max(q->width() / static_cast<qreal>(daysInWeek), Ui::DesignSystem::layout().px(32));
    const QSizeF dayRectSize = { dayWidth, dayWidth };
    int topMargin = 0;
    int leftMargin = 0;
    QDate currentDay(currentDate.year(), currentDate.month(), 1);
    constexpr int weeks = 6;
    for (int week = 0; week < weeks; ++week) {
        for (int dayOfWeek = 0; dayOfWeek < daysInWeek; ++dayOfWeek) {
            QRectF dayRect;
            dayRect.setLeft(leftMargin);
            dayRect.setTop(topMargin);
            dayRect.setSize(dayRectSize);

            //
            // Первая строка
            //
            if (week == 0) {
                //
                // Ещё не дошли до первого дня месяца
                //
                if (currentDay.dayOfWeek() > dayOfWeek + 1) {
                    //
                    // ... оставляем пустым
                    //
                }
                //
                // Наполняем дни
                //
                else {
                    days.append({ currentDay, dayRect });
                    currentDay = currentDay.addDays(1);
                }
            }
            //
            // Остальные строки
            //
            else {
                days.append({ currentDay, dayRect });
                currentDay = currentDay.addDays(1);
            }

            leftMargin += dayRectSize.width();
        }

        topMargin += dayRectSize.height();
        leftMargin = 0;
    }
}


// ****


MonthDaysWidget::MonthDaysWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
}

MonthDaysWidget::~MonthDaysWidget() = default;

void MonthDaysWidget::setCurrentDate(const QDate& _date)
{
    if (d->currentDate.year() == _date.year() && d->currentDate.month() == _date.month()) {
        return;
    }

    d->currentDate = _date;
    d->buildDays();
    update();
}

QDate MonthDaysWidget::selectedDate() const
{
    return d->selectedDate;
}

void MonthDaysWidget::setSelectedDate(const QDate& _date)
{
    d->selectedDate = _date;

    if (d->selectedDate.year() == d->currentDate.year()
        && d->selectedDate.month() == d->currentDate.month()) {
        update();
    }
}

QSize MonthDaysWidget::sizeHint() const
{
    if (d->days.isEmpty()) {
        return {};
    }

    return QSize(
        contentsMargins().left() + d->days.constLast().rect.right() + contentsMargins().right(),
        contentsMargins().top() + d->days.constLast().rect.bottom() + contentsMargins().bottom());
}

void MonthDaysWidget::paintEvent(QPaintEvent* _event)
{
    Widget::paintEvent(_event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(Ui::DesignSystem::font().caption());

    //
    // Рисуем даты
    //
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    for (const auto& date : std::as_const(d->days)) {
        //
        // Текущая дата
        //
        if (date.day == QDate::currentDate()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().accent());
            painter.drawEllipse(date.rect);

            painter.setPen(Ui::DesignSystem::color().onAccent());
            painter.setBrush(Qt::NoBrush);
            const auto adjustSize = Ui::DesignSystem::layout().px2();
            const auto borderRect
                = date.rect.adjusted(adjustSize, adjustSize, -adjustSize, -adjustSize);
            painter.drawText(borderRect, Qt::AlignCenter, QString::number(date.day.day()));
        }
        //
        // Текущая выбранная дата
        //
        else if (date.day == d->selectedDate) {
            painter.setPen(Ui::DesignSystem::color().accent());
            painter.setBrush(Qt::NoBrush);
            painter.drawText(date.rect, Qt::AlignCenter, QString::number(date.day.day()));

            const auto adjustSize = Ui::DesignSystem::layout().px2();
            const auto borderRect
                = date.rect.adjusted(adjustSize, adjustSize, -adjustSize, -adjustSize);
            painter.drawEllipse(borderRect);
        }
        //
        // Остальные даты
        //
        else {
            painter.setPen(ColorHelper::transparent(textColor(),
                                                    date.day.month() == d->currentDate.month()
                                                        ? 1.0
                                                        : Ui::DesignSystem::inactiveItemOpacity()));
            painter.setBrush(Qt::NoBrush);
            painter.drawText(date.rect, Qt::AlignCenter, QString::number(date.day.day()));

            //
            // Под мышкой
            //
            if (date.rect.toRect().contains(mousePos)) {
                const auto adjustSize = Ui::DesignSystem::layout().px2();
                const auto borderRect
                    = date.rect.adjusted(adjustSize, adjustSize, -adjustSize, -adjustSize);
                painter.drawEllipse(borderRect);
            }
        }
    }
}

void MonthDaysWidget::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->buildDays();
}

void MonthDaysWidget::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

    update();
}

void MonthDaysWidget::mousePressEvent(QMouseEvent* _event)
{
    for (const auto& color : std::as_const(d->days)) {
        if (!color.rect.contains(_event->pos())) {
            continue;
        }

        d->selectedDate = color.day;

        emit selectedDateChanged(d->selectedDate);

        update();

        break;
    }
}
