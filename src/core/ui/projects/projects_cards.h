#pragma once

#include <QGraphicsView>

namespace Domain {
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
    void projectPressed(const Domain::Project& _project);
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
    void setProjects(Domain::ProjectsModel* _projects);

protected:
    /**
     * @brief Переопределяем, чтобы корректировать область сцены, в зависимости от размера
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};



} // namespace Ui
