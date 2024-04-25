#pragma once

#include <ui/widgets/widget/widget.h>

#include <corelib_global.h>


/**
 * @brief Виджет календаря
 */
class DatePicker : public Widget
{
    Q_OBJECT

public:
    explicit DatePicker(QWidget* _parent = nullptr);
    ~DatePicker() override;

    /**
     * @brief Задать текущий отображаемый месяц
     */
    void setCurrentDate(const QDate& _date);

    /**
     * @brief Текущий выбранный цвет
     */
    QDate selectedDate() const;

    /**
     * @brief Задать текущий выбранный цвет
     */
    void setSelectedDate(const QDate& _date);

signals:
    /**
     * @brief Пользователь выбрал дату
     */
    void selectedDateChanged(const QDate& _date);

protected:
    /**
     * @brief Реализуем реакцию на задание кастомного цвета
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Настроить внешний вид в соответствии с дизайн системой
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
