#include "settings_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui
{

class SettingsNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Tree* tree = nullptr;
};

SettingsNavigator::Implementation::Implementation(QWidget* _parent)
    : tree(new Tree(_parent))
{

}


// ****


SettingsNavigator::SettingsNavigator(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);


    auto createItem = [] (const QString& _name, const QString& _icon) {
        auto item = new QStandardItem(_name);
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    QStandardItemModel* model = new QStandardItemModel(this);
    auto applicationItem = createItem("Application", "\uF614");
    applicationItem->appendRow(createItem("User interface", "\uf62e"));
    applicationItem->appendRow(createItem("Save changes/backups", "\uf61b"));
    model->appendRow(applicationItem);
    model->appendRow(createItem("Components", "\uf9ab"));
    model->appendRow(createItem("Shortcuts", "\uf30c"));
    d->tree->setModel(model);

    designSystemChangeEvent(nullptr);
}

void SettingsNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());
}

SettingsNavigator::~SettingsNavigator() = default;

} // namespace Ui
