#pragma once

#include <QGraphicsView>


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

protected:
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};



} // namespace Ui
