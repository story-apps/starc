#include "shortcuts_manager.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeySequence>


namespace ManagementLayer {

enum class ShortcutType {
    Import,
    Export,
};

class ShortcutsManager::Implementation
{
public:
    QVector<QString> shortcutNames;
    QHash<QString, ShortcutType> shortcutNamesToKeys;
};

ShortcutsManager::ShortcutsManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ShortcutsManager::~ShortcutsManager() = default;

QVector<QString> ShortcutsManager::shortcutNames() const
{
    return d->shortcutNames;
}

QKeySequence ShortcutsManager::shortcutByName(const QString& _name) const
{
    switch (d->shortcutNamesToKeys[_name]) {
    case ShortcutType::Import: {
        return importShortcut();
    }

    case ShortcutType::Export: {
        return currentDocumentExportShortcut();
    }

    default: {
        return {};
    }
    }
}

void ShortcutsManager::setShortcutByName(const QString& _name, const QKeySequence& _shortcut)
{
    switch (d->shortcutNamesToKeys[_name]) {
    case ShortcutType::Import: {
        setImportShortcut(_shortcut);
        break;
    }

    case ShortcutType::Export: {
        setExportShortcut(_shortcut);
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }
}

QKeySequence ShortcutsManager::importShortcut() const
{
    return settingsValue(DataStorageLayer::kApplicationShortcutsImportKey).toString();
}

void ShortcutsManager::setImportShortcut(const QKeySequence& _shortcut)
{
    setSettingsValue(DataStorageLayer::kApplicationShortcutsImportKey, _shortcut.toString());
    emit importShortcutChanged(_shortcut);
}

QKeySequence ShortcutsManager::currentDocumentExportShortcut() const
{
    return settingsValue(DataStorageLayer::kApplicationShortcutsCurrentDocumentExportKey)
        .toString();
}

void ShortcutsManager::setExportShortcut(const QKeySequence& _shortcut)
{
    setSettingsValue(DataStorageLayer::kApplicationShortcutsCurrentDocumentExportKey,
                     _shortcut.toString());
    emit exportShortcutChanged(_shortcut);
}

void ShortcutsManager::updateTranslations()
{
    d->shortcutNames.clear();
    d->shortcutNamesToKeys.clear();
    auto addShortcutInfo = [this](const QString& _shortcutName, ShortcutType _shortcutType) {
        d->shortcutNames.append(_shortcutName);
        d->shortcutNamesToKeys[_shortcutName] = _shortcutType;
    };
    addShortcutInfo(tr("Import"), ShortcutType::Import);
    addShortcutInfo(tr("Export"), ShortcutType::Export);
}

} // namespace ManagementLayer
