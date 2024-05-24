#include "import_manager.h"

#include <business_layer/import/audioplay/audioplay_fountain_importer.h>
#include <business_layer/import/audioplay/audioplay_import_options.h>
#include <business_layer/import/novel/novel_markdown_importer.h>
#include <business_layer/import/screenplay/screenplay_celtx_importer.h>
#include <business_layer/import/screenplay/screenplay_document_importer.h>
#include <business_layer/import/screenplay/screenplay_fdx_importer.h>
#include <business_layer/import/screenplay/screenplay_fountain_importer.h>
#include <business_layer/import/screenplay/screenplay_import_options.h>
#include <business_layer/import/screenplay/screenplay_kit_scenarist_importer.h>
#include <business_layer/import/screenplay/screenplay_trelby_importer.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/import/import_dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>
#include <utils/logging.h>

#include <QFileDialog>

namespace ManagementLayer {

class ImportManager::Implementation
{
public:
    explicit Implementation(ImportManager* _parent, QWidget* _topLevelWidget);

    /**
     * @brief Показать диалог импорта для заданного файла
     */
    void showImportDialogFor(const QString& _path);

    /**
     * @brief Импортировать данные документа из заданного файла
     */
    void importAudioplay(const BusinessLayer::AudioplayImportOptions& _options);
    void importNovel(const BusinessLayer::ImportOptions& _options);
    void importScreenplay(const BusinessLayer::ScreenplayImportOptions& _options);

    /**
     * @brief Импортировать данные романа из заданного файла
     */

    //
    // Данные
    //

    ImportManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ImportDialog* importDialog = nullptr;
};

ImportManager::Implementation::Implementation(ImportManager* _parent, QWidget* _topLevelWidget)
    : q(_parent)
    , topLevelWidget(_topLevelWidget)
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
                                    tr("Importing from DOC files is not supported. You need to "
                                       "save the file in DOCX format and repeat the import."));
        return;
    }

    if (importDialog == nullptr) {
        importDialog = new Ui::ImportDialog(_path, topLevelWidget);
        connect(importDialog, &Ui::ImportDialog::importRequested, importDialog, [this] {
            const auto importOptions = importDialog->importOptions();
            switch (importOptions.documentType) {
            default:
            case Domain::DocumentObjectType::Undefined: {
                break;
            }
            case Domain::DocumentObjectType::Audioplay: {
                const auto audioplayImportOptions = importDialog->audioplayImportOptions();
                importDialog->hideDialog();
                importAudioplay(audioplayImportOptions);
                break;
            }
            case Domain::DocumentObjectType::ComicBook: {
                break;
            }
            case Domain::DocumentObjectType::Novel: {
                importDialog->hideDialog();
                importNovel(importOptions);
                break;
            }
            case Domain::DocumentObjectType::Screenplay: {
                const auto screenplayImportOptions = importDialog->screenplayImportOptions();
                importDialog->hideDialog();
                importScreenplay(screenplayImportOptions);
                break;
            }
            case Domain::DocumentObjectType::Stageplay: {
                break;
            }
            }
        });
        connect(importDialog, &Ui::ImportDialog::canceled, importDialog,
                &Ui::ImportDialog::hideDialog);
        connect(importDialog, &Ui::ImportDialog::disappeared, importDialog, [this] {
            importDialog->deleteLater();
            importDialog = nullptr;
        });
    }

    importDialog->showDialog();
}

void ImportManager::Implementation::importAudioplay(
    const BusinessLayer::AudioplayImportOptions& _options)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractAudioplayImporter> importer;
    {
        const auto importFilePath = _options.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::fountain())
            || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::AudioplayFountainImporter);
        }
    }

    //
    // Импортируем текст аудиопьес
    //
    const auto audioplays = importer->importAudioplays(_options);
    for (const auto& audioplay : audioplays) {
        const auto screenplayName = !audioplay.name.isEmpty()
            ? audioplay.name
            : QFileInfo(_options.filePath).completeBaseName();
        emit q->audioplayImported(screenplayName, audioplay.titlePage, audioplay.synopsis,
                                  audioplay.text);
    }
}

void ImportManager::Implementation::importNovel(const BusinessLayer::ImportOptions& _options)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractNovelImporter> importer;
    {
        const auto importFilePath = _options.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::markdown())
            || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::NovelMarkdownImporter);
        }
    }

    //
    // Импортируем текст романа
    //
    const auto novel = importer->importNovels(_options);
    const auto novelName
        = !novel.name.isEmpty() ? novel.name : QFileInfo(_options.filePath).completeBaseName();
    emit q->novelImported(novelName, novel.text);
}

void ImportManager::Implementation::importScreenplay(
    const BusinessLayer::ScreenplayImportOptions& _importOptions)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractScreenplayImporter> importer;
    {
        const auto importFilePath = _importOptions.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::kitScenarist())) {
            importer.reset(new BusinessLayer::ScreenplayKitScenaristImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::finalDraft())
                   || importFilePath.endsWith(ExtensionHelper::finalDraftTemplate())) {
            importer.reset(new BusinessLayer::ScreenplayFdxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::trelby())) {
            importer.reset(new BusinessLayer::ScreenplayTrelbyImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::msOfficeOpenXml())
                   || importFilePath.endsWith(ExtensionHelper::openDocumentXml())) {
            importer.reset(new BusinessLayer::ScreenplayDocumentImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::celtx())) {
            importer.reset(new BusinessLayer::ScreenplayCeltxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::fountain())
                   || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::ScreenplayFountainImporter);
        }
    }

    //
    // Импортируем документы
    //
    const auto documents = importer->importDocuments(_importOptions);
    for (const auto& character : documents.characters) {
        emit q->characterImported(character.name, character.content);
    }
    for (const auto& location : documents.locations) {
        emit q->locationImported(location.name, location.content);
    }

    //
    // Импортируем текст сценариев
    //
    const auto screenplays = importer->importScreenplays(_importOptions);
    for (const auto& screenplay : screenplays) {
        const auto screenplayName = !screenplay.name.isEmpty()
            ? screenplay.name
            : QFileInfo(_importOptions.filePath).completeBaseName();
        emit q->screenplayImported(screenplayName, screenplay.titlePage, screenplay.synopsis,
                                   screenplay.treatment, screenplay.text);
    }
}


// ****


ImportManager::ImportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget))
{
}

ImportManager::~ImportManager() = default;

void ImportManager::import()
{
    Log::info("Importing started");

    //
    // Предоставим пользователю возможность выбрать файл, который он хочет импортировать
    //
    const auto projectImportFolder
        = settingsValue(DataStorageLayer::kProjectImportFolderKey).toString();
    const auto importFilePath
        = QFileDialog::getOpenFileName(d->topLevelWidget, tr("Choose the file to import"),
                                       projectImportFolder, DialogHelper::filtersForImport());
    if (importFilePath.isEmpty()) {
        return;
    }

    //
    // Если файл был выбран
    //
    // ... обновим папку, откуда в следующий раз он предположительно опять будет импортировать
    // проекты
    //
    setSettingsValue(DataStorageLayer::kProjectImportFolderKey, importFilePath);
    //
    // ... и переходим к подготовке импорта
    //
    d->showImportDialogFor(importFilePath);
}

void ImportManager::importScreenplay(const QString& _filePath, bool _importDocuments)
{
    BusinessLayer::ScreenplayImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Screenplay;
    options.importCharacters = _importDocuments;
    options.importLocations = _importDocuments;
    options.importResearch = _importDocuments;
    d->importScreenplay(options);
}

void ImportManager::importNovel(const QString& _filePath)
{
    BusinessLayer::ImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Novel;
    d->importNovel(options);
}

} // namespace ManagementLayer
