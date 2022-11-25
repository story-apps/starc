#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Виджет с информацией о счётчиках
 */
class CountersInfoWidget : public Widget
{
    Q_OBJECT

public:
    explicit CountersInfoWidget(QWidget* _parent = nullptr);
    ~CountersInfoWidget() override;

    /**
     * @brief Задать счётчики
     */
    void setCounters(const QVector<QString>& _counters);

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
