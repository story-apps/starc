#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui {

/**
 * @brief Панель инструментов редактора шаблона сценария
 */
class ScreenplayTemplateViewToolBar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ScreenplayTemplateViewToolBar(QWidget* _parent = nullptr);
    ~ScreenplayTemplateViewToolBar() override;

signals:
    /**
     * @brief Пользователь нажал кнопку сохранения шаблона
     */
    void savePressed();

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
