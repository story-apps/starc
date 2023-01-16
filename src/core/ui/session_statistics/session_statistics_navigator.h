#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Навигатор списка настроек
 */
class SessionStatisticsNavigator : public Widget
{
    Q_OBJECT

public:
    explicit SessionStatisticsNavigator(QWidget* _parent = nullptr);
    ~SessionStatisticsNavigator() override;

    /**
     * @brief Задать информацию о текущей сессии
     */
    void setCurrentSessionDetails(int _words, const QDateTime& _startedAt);

    /**
     * @brief Задать экстремумы 30 последних дней
     */
    void set30DaysOverviewDetails(std::chrono::seconds _minSessionDuration,
                                  std::chrono::seconds _maxSessionDuration, int _minWords,
                                  int _maxWords);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
