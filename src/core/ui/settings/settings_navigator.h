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

signals:
    /**
     * @brief Пользователь хочет перейти в отображению заданных настроек
     */
    void applicationPressed();
    void applicationUserInterfacePressed();
    void applicationSaveAndBackupsPressed();
    void componentsPressed();
    void componentsScreenplayPressed();
    void shortcutsPressed();

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
