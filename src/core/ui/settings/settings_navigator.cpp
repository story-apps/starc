#include "settings_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
const int kApplicationIndex = 0;
const int kApplicationUserInterfaceIndex = 0;
const int kApplicationSaveAndBackupIndex = 1;
const int kComponentsIndex = 1;
const int kComponentsSimpleTextIndex = 0;
const int kComponentsScreenplayIndex = 1;
const int kShortcutsIndex = 2;
} // namespace


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
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->tree);


    auto createItem = [](const QString& _icon) {
        auto item = new QStandardItem;
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    QStandardItemModel* model = new QStandardItemModel(this);
    auto applicationItem = createItem(u8"\U000f0614");
    applicationItem->appendRow(createItem(u8"\U000f062e"));
    applicationItem->appendRow(createItem(u8"\U000f061b"));
    model->appendRow(applicationItem);
    auto componentsItem = createItem(u8"\U000f09ac");
    componentsItem->appendRow(createItem(u8"\U000F021A"));
    componentsItem->appendRow(createItem(u8"\U000f0bc2"));
    model->appendRow(componentsItem);
    model->appendRow(createItem(u8"\U000f030c"));
    d->tree->setModel(model);
    d->tree->setCurrentIndex(model->index(0, 0));
    d->tree->expandAll();

    connect(d->tree, &Tree::currentIndexChanged, this, [this](const QModelIndex& _index) {
        if (_index.parent().isValid()) {
            switch (_index.parent().row()) {
            case kApplicationIndex: {
                switch (_index.row()) {
                case kApplicationUserInterfaceIndex: {
                    emit applicationUserInterfacePressed();
                    break;
                }
                case kApplicationSaveAndBackupIndex: {
                    emit applicationSaveAndBackupsPressed();
                    break;
                }
                default: {
                    break;
                }
                }
                break;
            }
            case kComponentsIndex: {
                switch (_index.row()) {
                case kComponentsSimpleTextIndex: {
                    emit componentsSimpleTextPressed();
                    break;
                }
                case kComponentsScreenplayIndex: {
                    emit componentsScreenplayPressed();
                    break;
                }
                default: {
                    break;
                }
                }
                break;
            }
            default: {
                break;
            }
            }
        } else {
            switch (_index.row()) {
            case kApplicationIndex: {
                emit applicationPressed();
                break;
            }
            case kComponentsIndex: {
                emit componentsPressed();
                break;
            }
            case kShortcutsIndex: {
                emit shortcutsPressed();
                break;
            }
            default: {
                break;
            }
            }
        }
    });

    designSystemChangeEvent(nullptr);
}

void SettingsNavigator::updateTranslations()
{
    auto model = qobject_cast<QStandardItemModel*>(d->tree->model());
    model->item(kApplicationIndex)->setText(tr("Application settings"));
    model->item(kApplicationIndex)
        ->child(kApplicationUserInterfaceIndex)
        ->setText(tr("User interface"));
    model->item(kApplicationIndex)
        ->child(kApplicationSaveAndBackupIndex)
        ->setText(tr("Save changes & backups"));
    model->item(kComponentsIndex)->setText(tr("Components"));
    model->item(kComponentsIndex)->child(kComponentsSimpleTextIndex)->setText(tr("Simple text"));
    model->item(kComponentsIndex)->child(kComponentsScreenplayIndex)->setText(tr("Screenplay"));
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
