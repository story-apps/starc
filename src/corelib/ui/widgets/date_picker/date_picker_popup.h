#pragma once

#include <ui/widgets/card/card.h>


/**
 * @brief Виджет всплывающего окошка для выбора даты
 */
class CORE_LIBRARY_EXPORT DatePickerPopup : public Card
{
    Q_OBJECT

public:
    explicit DatePickerPopup(QWidget* _parent = nullptr);
    ~DatePickerPopup() override;

    /**
     * @brief Задать необходимость закрытия попапа после выборе даты
     */
    void setAutoHideOnSelection(bool _autoHide);

    /**
     * @brief Задать текущий отображаемый месяц
     */
    void setCurrentDate(const QDate& _date);

    /**
     * @brief Выбранная пользователем дата
     */
    QDate selectedDate() const;

    /**
     * @brief Задать выбранную пользователем дату
     */
    void setSelectedDate(const QDate& _date);

    /**
     * @brief Показан ли попап в данный момент
     */
    bool isPopupShown() const;

    /**
     * @brief Показать попап выбора даты
     */
    void showPopup(QWidget* _parent, Qt::Alignment _alignment = Qt::AlignBottom | Qt::AlignHCenter);

    /**
     * @brief Скрыть попап выбора даты
     */
    void hidePopup();

signals:
    /**
     * @brief Пользователь выбрал дату
     */
    void selectedDateChanged(const QDate& _date);

protected:
    /**
     * @brief Прокидываем изменение цвета в дочерние виджеты
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

    /**
     * @brief Следим за событиям потери фокуса в виджете, к которому привязан попап
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
