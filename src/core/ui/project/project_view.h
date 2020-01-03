#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace Ui
{

/**
 * @brief Представление проекта
 */
class ProjectView : public StackWidget
{
    Q_OBJECT
public:
    explicit ProjectView(QWidget* _parent = nullptr);
    ~ProjectView() override;

    /**
     * @brief Показать умолчальную страницу
     */
    void showDefaultPage();

signals:
    /**
     * @brief Пользователь нажал кнопку создания нового проекта
     */
    void createNewItemPressed();

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
