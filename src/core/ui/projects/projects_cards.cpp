#include "projects_cards.h"

#include <QResizeEvent>


namespace Ui
{

class ProjectsCards::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

};

ProjectsCards::Implementation::Implementation(QWidget* _parent)
{
}


// ****


ProjectsCards::ProjectsCards(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
}

ProjectsCards::~ProjectsCards() = default;

void ProjectsCards::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);
}

}
