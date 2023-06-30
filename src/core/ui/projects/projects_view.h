#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace BusinessLayer {
class ProjectsModelProjectItem;
class ProjectsModel;
} // namespace BusinessLayer


namespace Ui {

/**
 * @brief Представление списка проектов
 */
class ProjectsView : public StackWidget
{
    Q_OBJECT

public:
    explicit ProjectsView(QWidget* _parent = nullptr);
    ~ProjectsView() override;

    /**
     * @brief Задать список проектов
     */
    void setProjects(BusinessLayer::ProjectsModel* _projects);

    /**
     * @brief Показать страницу без проектов
     */
    void showEmptyPage();

    /**
     * @brief Показать страницу со списком проектов
     */
    void showProjectsPage();

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
     * @brief Пользователь хочет открыть выбранный проект
     */
    void openProjectRequested(BusinessLayer::ProjectsModelProjectItem* _project);

    /**
     * @brief Запрос на отображение контекстного меню проекта
     */
    void projectContextMenuRequested(BusinessLayer::ProjectsModelProjectItem* _project);

protected:
    /**
     * @brief Переопределяем для корректировки положения тулбара действий над проектами
     */
    void resizeEvent(QResizeEvent* _event) override;

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
