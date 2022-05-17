#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace Ui {

/**
 * @brief Навигатор списка настроек
 */
class SettingsNavigator : public StackWidget
{
    Q_OBJECT

public:
    explicit SettingsNavigator(QWidget* _parent = nullptr);
    ~SettingsNavigator() override;

    /**
     * @brief Показать основную страницу
     */
    void showDefaultPage();

signals:
    /**
     * @brief Пользователь хочет перейти в отображению заданных настроек
     */
    void applicationPressed();
    void applicationUserInterfacePressed();
    void applicationSaveAndBackupsPressed();
    void applicationTextEditingPressed();
    void componentsPressed();
    void componentsSimpleTextPressed();
    void componentsScreenplayPressed();
    void componentsComicBookPressed();
    void componentsAudioplayPressed();
    void componentsStageplayPressed();
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
