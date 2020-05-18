#include "import_manager.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/import/import_dialog.h>

#include <utils/helpers/dialog_helper.h>

#include <QFileDialog>


namespace ManagementLayer
{

class ImportManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Показать диалог импорта для заданного файла
     */
    void showImportDialogFor(const QString& _path);


    QWidget* topLevelWidget = nullptr;

    Ui::ImportDialog* importDialog = nullptr;
};

ImportManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent)
{
}

void ImportManager::Implementation::showImportDialogFor(const QString& _path)
{
    if (importDialog == nullptr) {
        importDialog = new Ui::ImportDialog(_path, topLevelWidget);
        connect(importDialog, &Ui::ImportDialog::canceled, importDialog, &Ui::ImportDialog::hideDialog);
        connect(importDialog, &Ui::ImportDialog::disappeared, importDialog, [this] {
            importDialog->deleteLater();
            importDialog = nullptr;
        });
    }

    importDialog->showDialog();
}


// ****


ImportManager::ImportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{

}

ImportManager::~ImportManager() = default;

void ImportManager::import()
{
    //
    // Предоставим пользователю возможность выбрать файл, который он хочет импортировать
    //
    const auto projectOpenFolder
            = DataStorageLayer::StorageFacade::settingsStorage()->value(
                  DataStorageLayer::kProjectImportFolderKey,
                  DataStorageLayer::SettingsStorage::SettingsPlace::Application)
              .toString();
    const auto importFilePath
            = QFileDialog::getOpenFileName(d->topLevelWidget, tr("Choose the file to import"),
                    projectOpenFolder, DialogHelper::importFilters());
    if (importFilePath.isEmpty()) {
        return;
    }

    //
    // Если файл был выбран
    //
    // ... обновим папку, откуда в следующий раз он предположительно опять будет открывать проекты
    //
    DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                DataStorageLayer::kProjectImportFolderKey,
                importFilePath,
                DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    //
    // ... и переходим к подготовке импорта
    //
    d->showImportDialogFor(importFilePath);
}

} // namespace ManagementLayer
