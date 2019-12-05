#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Навигатор списка настроек
 */
class SettingsNavigator : public Widget
{
    Q_OBJECT

public:
    explicit SettingsNavigator(QWidget* _parent = nullptr);
    ~SettingsNavigator() override;

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
