#include "settings_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui
{

namespace {
    const int kApplicationIndex = 0;
    const int kApplicationUserInterfaceIndex = 0;
    const int kApplicationSaveAndBackupIndex = 1;
    const int kComponentsIndex = 1;
    const int kShortcutsIndex = 2;
}


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


    auto createItem = [] (const QString& _icon) {
        auto item = new QStandardItem;
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    QStandardItemModel* model = new QStandardItemModel(this);
    auto applicationItem = createItem("\uF614");
    applicationItem->appendRow(createItem("\uf62e"));
    applicationItem->appendRow(createItem("\uf61b"));
    model->appendRow(applicationItem);
    model->appendRow(createItem("\uf9ab"));
    model->appendRow(createItem("\uf30c"));
    d->tree->setModel(model);

    designSystemChangeEvent(nullptr);
}

void SettingsNavigator::updateTranslations()
{
    auto model = qobject_cast<QStandardItemModel*>(d->tree->model());
    model->item(kApplicationIndex)->setText(tr("Application"));
    model->item(kApplicationIndex)->child(kApplicationUserInterfaceIndex)->setText(tr("User interface"));
    model->item(kApplicationIndex)->child(kApplicationSaveAndBackupIndex)->setText(tr("Save changes/backups"));
    model->item(kComponentsIndex)->setText(tr("Components"));
    model->item(kShortcutsIndex)->setText(tr("Shortcuts"));
}

void SettingsNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());
}

SettingsNavigator::~SettingsNavigator() = default;

} // namespace Ui
