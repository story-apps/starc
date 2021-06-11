#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Навигатор списка проектов
 */
class ProjectsNavigator : public Widget
{
    Q_OBJECT

public:
    explicit ProjectsNavigator(QWidget* _parent = nullptr);
    ~ProjectsNavigator() override;

signals:
    /**
     * @brief Пользователь нажал кнопку создания истории
     */
    void createProjectPressed();

    /**
     * @brief Пользователь нажал кнопку открытия истории
     */
    void openProjectPressed();

    /**
     * @brief Пользователь нажал кнопку отображения справки
     */
    void helpPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
