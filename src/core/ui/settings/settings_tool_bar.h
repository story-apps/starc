#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace Ui {

/**
 * @brief Панель инструментов настроек
 */
class SettingsToolBar : public StackWidget
{
    Q_OBJECT

public:
    explicit SettingsToolBar(QWidget* _parent = nullptr);
    ~SettingsToolBar() override;

    /**
     * @brief Показать основную страницу
     */
    void showDefaultPage();

signals:
    /**
     * @brief Пользователь хочет выйти из личного кабинета
     */
    void backPressed();

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
    QScopedArrayPointer<Implementation> d;
};

} // namespace Ui
