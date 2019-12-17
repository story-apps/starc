#pragma once

#include <QGraphicsView>

namespace ManagementLayer {
    class Project;
    class ProjectsModel;
}


namespace Ui
{

/**
 * @brief Сцена для отображения списка проектов
 */
class ProjectsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit ProjectsScene(QObject* _parent = nullptr);

signals:
    /**
     * @brief Пользователь выбрал проект
     */
    void projectPressed(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет перенести проект в облако
     */
    void moveProjectToCloudRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет скрыть проект
     */
    void hideProjectRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет изменить название проекта
     */
    void changeProjectNameRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет удалить проект
     */
    void removeProjectRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Требуется переупорядочить карточки проектов, т.к. заданная карточка была перенесена
     */
    void reorderProjectCardRequested(QGraphicsItem* _movedCard);

protected:
    /**
     * @brief Реализуем переупорядочивание при перетаскивании карточки
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent* _event) override;
};

/**
 * @brief Представление модели со списком проектов
 */
class ProjectsCards : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ProjectsCards(QWidget* _parent = nullptr);
    ~ProjectsCards() override;

    /**
     * @brief Задать цвет фона
     */
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Задать модель проектов
     */
    void setProjects(ManagementLayer::ProjectsModel* _projects);

signals:
    /**
     * @brief Запрос на отображение (когда появились проекты)
     */
    void showRequested();

    /**
     * @brief Запрос на скрытие (когда не осталось проектов)
     */
    void hideRequested();

    /**
     * @brief Пользователь хочет открыть выбранный проект
     */
    void openProjectRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет перенести проект в облако
     */
    void moveProjectToCloudRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет скрыть проект
     */
    void hideProjectRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет изменить название проекта
     */
    void changeProjectNameRequested(const ManagementLayer::Project& _project);

    /**
     * @brief Пользователь хочет удалить проект
     */
    void removeProjectRequested(const ManagementLayer::Project& _project);

protected:
    /**
     * @brief Перенастраиваем виджет при обновлении дизайн системы
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы корректировать область сцены, в зависимости от размера
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    /**
     * @brief Уведомить клиентов, если нужно изменить видимость списка проектов
     */
    void notifyVisibleChange();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};



} // namespace Ui
