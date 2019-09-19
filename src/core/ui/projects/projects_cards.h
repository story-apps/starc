#pragma once

#include <QGraphicsView>

namespace Domain {
    class ProjectsModel;
}


namespace Ui
{

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
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};



} // namespace Ui
