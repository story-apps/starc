#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет календаря со списком дней месяца
 */
class MonthDaysWidget : public Widget
{
    Q_OBJECT

public:
    explicit MonthDaysWidget(QWidget* _parent = nullptr);
    ~MonthDaysWidget() override;

    /**
     * @brief Задать текущую отображаемую дату
     */
    void setCurrentDate(const QDate& _date);

    /**
     * @brief Текущая выбранная дата
     */
    QDate selectedDate() const;
    void setSelectedDate(const QDate& _date);

    /**
     * @brief Сами определяем идеальный размер для виджета
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь выбрал дату
     */
    void selectedDateChanged(const QDate& _date);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Корректируем ширину элементов дат при изменении размера виджета
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Рисуем обводку вокруг числа над которым находится курсор
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Обрабатываем клики
     */
    void mousePressEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
