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

    QStandardItemModel* model = new QStandardItemModel(this);
    auto row = new QStandardItem("some text 1");
    row->setData("\uF614", Qt::DecorationRole);
    row->appendRow(new QStandardItem("child 1"));
    model->appendRow(row);
    model->appendRow(new QStandardItem("some text 2"));
    model->appendRow(new QStandardItem("some text 3"));
    model->appendRow(new QStandardItem("some text 4"));
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
