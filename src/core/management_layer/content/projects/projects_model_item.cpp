#include "projects_model_item.h"

#include <QVariant>

namespace BusinessLayer {

ProjectsModelItem::ProjectsModelItem()
    : AbstractModelItem()
{
}

ProjectsModelItemType ProjectsModelItem::type() const
{
    return ProjectsModelItemType::Root;
}

QVariant ProjectsModelItem::data(int _role) const
{
    Q_UNUSED(_role)
    return {};
}

ProjectsModelItem* ProjectsModelItem::parent() const
{
    return static_cast<ProjectsModelItem*>(AbstractModelItem::parent());
}

} // namespace BusinessLayer
