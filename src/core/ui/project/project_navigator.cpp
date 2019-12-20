#include "project_navigator.h"

#include <business_layer/structure_model.h>
#include <business_layer/structure_model_item.h>

#include <domain/document_object.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QUuid>
#include <QVBoxLayout>


namespace Ui
{

class ProjectNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Tree* tree = nullptr;
};

ProjectNavigator::Implementation::Implementation(QWidget* _parent)
    : tree(new Tree(_parent))
{
    tree->setDragDropEnabled(true);
}


// ****


ProjectNavigator::ProjectNavigator(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);
}

void ProjectNavigator::setModel(QAbstractItemModel* _model)
{
    d->tree->setModel(_model);
}

ProjectNavigator::~ProjectNavigator() = default;

void ProjectNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui
