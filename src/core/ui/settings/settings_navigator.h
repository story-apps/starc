#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

class SettingsNavigator : public Widget
{
    Q_OBJECT

public:
    explicit SettingsNavigator(QWidget* _parent = nullptr);
    ~SettingsNavigator() override;

protected:
    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
