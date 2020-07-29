#include "import_manager.h"

#include <business_layer/import/celtx_importer.h>
#include <business_layer/import/fdx_importer.h>
#include <business_layer/import/import_options.h>
#include <business_layer/import/kit_scenarist_importer.h>
#include <business_layer/import/trelby_importer.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/import/import_dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>

#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>

#include <QFileDialog>


namespace ManagementLayer
{

class ImportManager::Implementation
{
public:
    explicit Implementation(ImportManager* _parent, QWidget* _topLevelWidget);

    /**
     * @brief Показать диалог импорта для заданного файла
     */
    void showImportDialogFor(const QString& _path);

    /**
     * @brief Импортировать данные из заданного файла
     */
    void import(const BusinessLayer::ImportOptions& _options);

    //
    // Данные
    //

    ImportManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ImportDialog* importDialog = nullptr;
};

ImportManager::Implementation::Implementation(ImportManager* _parent, QWidget* _topLevelWidget)
    : q(_parent),
      topLevelWidget(_topLevelWidget)
{
}

void ImportManager::Implementation::showImportDialogFor(const QString& _path)
{
    //
    // Формат MS DOC не поддерживается, он отображается только для того, чтобы пользователи
    // не теряли свои файлы
    //
    if (_path.toLower().endsWith(ExtensionHelper::msOfficeBinary())) {
        StandardDialog::information(topLevelWidget, tr("File format not supported"),
            tr("Importing from DOC files is not supported. You need to save the file in DOCX format and repeat the import."));
        return;
    }

    if (importDialog == nullptr) {
        importDialog = new Ui::ImportDialog(_path, topLevelWidget);
        connect(importDialog, &Ui::ImportDialog::importRequested, importDialog, [this] {
            importDialog->hideDialog();
            import(importDialog->importOptions());
        });
        connect(importDialog, &Ui::ImportDialog::canceled, importDialog, &Ui::ImportDialog::hideDialog);
        connect(importDialog, &Ui::ImportDialog::disappeared, importDialog, [this] {
            importDialog->deleteLater();
            importDialog = nullptr;
        });
    }

    importDialog->showDialog();
}

void ImportManager::Implementation::import(const BusinessLayer::ImportOptions& _options)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractImporter> importer;
    {
        const auto importFilePath = _options.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::starc())) {

        } else if (importFilePath.endsWith(ExtensionHelper::kitScenarist())) {
            importer.reset(new BusinessLayer::KitScenaristImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::finalDraft())
                   || importFilePath.endsWith(ExtensionHelper::finalDraftTemplate())) {
            importer.reset(new BusinessLayer::FdxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::trelby())) {
            importer.reset(new BusinessLayer::TrelbyImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::msOfficeBinary())) {

        } else if (importFilePath.endsWith(ExtensionHelper::msOfficeOpenXml())) {

        } else if (importFilePath.endsWith(ExtensionHelper::openDocumentXml())) {

        } else if (importFilePath.endsWith(ExtensionHelper::fountain())) {

        } else if (importFilePath.endsWith(ExtensionHelper::celtx())) {
            importer.reset(new BusinessLayer::CeltxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::plainText())) {

        }
    }

    //
    // Импортируем документы
    //
    const auto documents = importer->importDocuments(_options);
    for (const auto& character : documents.characters) {
        emit q->characterImported(character.name, character.content);
    }
    for (const auto& location : documents.locations) {
        emit q->locationImported(location.name, location.content);
    }

    //
    // Импортируем текст сценариев
    //
    const auto screenplays = importer->importScreenplays(_options);
    for (const auto& screenplay : screenplays) {
        emit q->screenplayImported(screenplay.name, screenplay.titlePage, screenplay.synopsis,
                                   screenplay.treatment, screenplay.text);
    }
}


// ****


ImportManager::ImportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(this, _parentWidget))
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
