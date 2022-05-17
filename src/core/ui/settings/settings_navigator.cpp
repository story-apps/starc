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
const int kApplicationTextEditingIndex = 2;
const int kComponentsIndex = 1;
const int kComponentsSimpleTextIndex = 0;
const int kComponentsScreenplayIndex = 1;
const int kComponentsComicBookIndex = 2;
const int kComponentsAudioplayIndex = 3;
const int kComponentsStageplayIndex = 4;
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
    auto createItem = [](const QString& _icon) {
        auto item = new QStandardItem;
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    QStandardItemModel* model = new QStandardItemModel(tree);
    auto applicationItem = createItem(u8"\U000f0614");
    applicationItem->appendRow(createItem(u8"\U000f062e"));
    applicationItem->appendRow(createItem(u8"\U000f061b"));
    applicationItem->appendRow(createItem(u8"\U000F05E7"));
    model->appendRow(applicationItem);
    auto componentsItem = createItem(u8"\U000f09ac");
    componentsItem->appendRow(createItem(u8"\U000F021A"));
    componentsItem->appendRow(createItem(u8"\U000F0FCE"));
    componentsItem->appendRow(createItem(u8"\U000F056E"));
    componentsItem->appendRow(createItem(u8"\U000F02CB"));
    componentsItem->appendRow(createItem(u8"\U000F0D02"));
    model->appendRow(componentsItem);
    model->appendRow(createItem(u8"\U000f030c"));
    tree->setModel(model);
    tree->setCurrentIndex(model->index(0, 0));
    tree->expandAll();
}


// ****


SettingsNavigator::SettingsNavigator(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    showDefaultPage();

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
                case kApplicationTextEditingIndex: {
                    emit applicationTextEditingPressed();
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
                case kComponentsComicBookIndex: {
                    emit componentsComicBookPressed();
                    break;
                }
                case kComponentsAudioplayIndex: {
                    emit componentsAudioplayPressed();
                    break;
                }
                case kComponentsStageplayIndex: {
                    emit componentsStageplayPressed();
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

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void SettingsNavigator::showDefaultPage()
{
    setCurrentWidget(d->tree);
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
    model->item(kApplicationIndex)
        ->child(kApplicationTextEditingIndex)
        ->setText(tr("Text editing"));
    model->item(kComponentsIndex)->setText(tr("Components"));
    model->item(kComponentsIndex)->child(kComponentsSimpleTextIndex)->setText(tr("Simple text"));
    model->item(kComponentsIndex)->child(kComponentsScreenplayIndex)->setText(tr("Screenplay"));
    model->item(kComponentsIndex)->child(kComponentsComicBookIndex)->setText(tr("Comic book"));
    model->item(kComponentsIndex)->child(kComponentsAudioplayIndex)->setText(tr("Audioplay"));
    model->item(kComponentsIndex)->child(kComponentsStageplayIndex)->setText(tr("Stageplay"));
    model->item(kShortcutsIndex)->setText(tr("Shortcuts"));
}

void SettingsNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());
}

SettingsNavigator::~SettingsNavigator() = default;

} // namespace Ui
