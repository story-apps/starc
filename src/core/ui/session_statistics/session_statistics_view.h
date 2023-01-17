#pragma once

#include <ui/widgets/widget/widget.h>

namespace BusinessLayer {
struct Plot;
}

namespace Ui {

/**
 * @brief Представление настроек
 */
class SessionStatisticsView : public Widget
{
    Q_OBJECT

public:
    explicit SessionStatisticsView(QWidget* _parent = nullptr);
    ~SessionStatisticsView() override;

    /**
     * @brief Настроить возможность отображения детальной статистики
     */
    void setAbleToShowDetails(bool _able);

    /**
     * @brief Показывать каждое устройство по отдельности
     */
    bool showDevices() const;

    /**
     * @brief Отобразить график
     */
    void setPlot(const BusinessLayer::Plot& _plot);

signals:
    /**
     * @brief Пользователь поменял настройки для отображения представления
     */
    void viewPreferencesChanged();

protected:
    /**
     * @brief Переопределяем для обновления размеров графика и таблицы
     */
    void resizeEvent(QResizeEvent* _event) override;

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
