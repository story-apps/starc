#include "import_manager.h"

#include <business_layer/import/audioplay/audioplay_fountain_importer.h>
#include <business_layer/import/comic_book/comic_book_fountain_importer.h>
#include <business_layer/import/import_options.h>
#include <business_layer/import/novel/novel_markdown_importer.h>
#include <business_layer/import/screenplay/screenplay_celtx_importer.h>
#include <business_layer/import/screenplay/screenplay_docx_importer.h>
#include <business_layer/import/screenplay/screenplay_fdx_importer.h>
#include <business_layer/import/screenplay/screenplay_fountain_importer.h>
#include <business_layer/import/screenplay/screenplay_kit_scenarist_importer.h>
#include <business_layer/import/screenplay/screenplay_pdf_importer.h>
#include <business_layer/import/screenplay/screenplay_trelby_importer.h>
#include <business_layer/import/stageplay/stageplay_fountain_importer.h>
#include <business_layer/import/text/simple_text_docx_importer.h>
#include <business_layer/import/text/simple_text_markdown_importer.h>
#include <business_layer/import/text/simple_text_pdf_importer.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/import/import_dialog.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>
#include <utils/logging.h>

#include <QFileDialog>
#include <QSet>

namespace ManagementLayer {

class ImportManager::Implementation
{
public:
    explicit Implementation(ImportManager* _parent, QWidget* _topLevelWidget);

    /**
     * @brief Показать диалог импорта для заданных файлов
     */
    void showImportDialogFor(const QStringList& _paths);

    /**
     * @brief Импортировать данные документа из заданного файла
     */
    void importSimpleText(const BusinessLayer::ImportOptions& _options);
    void importAudioplay(const BusinessLayer::ImportOptions& _options);
    void importComicBook(const BusinessLayer::ImportOptions& _options);
    void importNovel(const BusinessLayer::ImportOptions& _options);
    void importScreenplay(const BusinessLayer::ImportOptions& _options);
    void importStageplay(const BusinessLayer::ImportOptions& _options);
    void importPresentation(const BusinessLayer::ImportOptions& _options);

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

void ImportManager::Implementation::showImportDialogFor(const QStringList& _paths)
{
    //
    // Формат MS DOC не поддерживается, он отображается только для того, чтобы пользователи
    // не теряли свои файлы
    //
    QStringList docFiles;
    QStringList otherFiles;
    QStringList filesToImport;
    const QSet<QString> supportedFileTypes = {
        ExtensionHelper::kitScenarist(),
        ExtensionHelper::finalDraft(),
        ExtensionHelper::finalDraftTemplate(),
        ExtensionHelper::trelby(),
        ExtensionHelper::msOfficeOpenXml(),
        ExtensionHelper::openDocumentXml(),
        ExtensionHelper::fountain(),
        ExtensionHelper::celtx(),
        ExtensionHelper::markdown(),
        ExtensionHelper::plainText(),
        ExtensionHelper::pdf(),
    };
    for (const auto& path : _paths) {
        const auto fileExtension = path.split('.').constLast().toLower();
        if (fileExtension == ExtensionHelper::msOfficeBinary()) {
            docFiles.append(path);
        } else if (supportedFileTypes.contains(fileExtension)) {
            filesToImport.append(path);
        } else {
            otherFiles.append(path);
        }
    }

    //
    // Покажем сообщение об ошибке импорта, если не было ни одного поддерживаемого файла
    //
    if (filesToImport.isEmpty()) {
        QString errorMessage;
        if (!docFiles.isEmpty()) {
            errorMessage.append(tr("Importing from DOC files is not supported. You need to save "
                                   "the file in DOCX format and repeat the import."));
        }
        if (!otherFiles.isEmpty()) {
            QString unsupporterExtensions;
            for (const auto& path : std::as_const(otherFiles)) {
                if (const auto fileExtension = path.split('.').constLast().toLower();
                    !unsupporterExtensions.contains(fileExtension)) {
                    if (!unsupporterExtensions.isEmpty()) {
                        unsupporterExtensions.append(", ");
                    }
                    unsupporterExtensions.append(fileExtension);
                }
            }

            if (!errorMessage.isEmpty()) {
                errorMessage.append("\n\n");
            }
            errorMessage.append(tr("Importing from %1 files is not supported.")
                                    .arg(unsupporterExtensions.toUpper()));
        }
        StandardDialog::information(topLevelWidget, tr("File format not supported"), errorMessage);
        return;
    }

    //
    // Создаем диалог импорта
    //
    if (importDialog == nullptr) {
        importDialog = new Ui::ImportDialog(filesToImport, topLevelWidget);
        connect(importDialog, &Ui::ImportDialog::importRequested, importDialog, [this] {
            const auto optionsList = importDialog->importOptions();
            importDialog->hideDialog();
            for (const auto& importOptions : optionsList) {
                switch (importOptions.documentType) {
                default:
                case Domain::DocumentObjectType::Undefined: {
                    break;
                }
                case Domain::DocumentObjectType::SimpleText: {
                    importSimpleText(importOptions);
                    break;
                }
                case Domain::DocumentObjectType::Audioplay: {
                    importAudioplay(importOptions);
                    break;
                }
                case Domain::DocumentObjectType::ComicBook: {
                    importComicBook(importOptions);
                    break;
                }
                case Domain::DocumentObjectType::Novel: {
                    importNovel(importOptions);
                    break;
                }
                case Domain::DocumentObjectType::Screenplay: {
                    importScreenplay(importOptions);
                    break;
                }
                case Domain::DocumentObjectType::Stageplay: {
                    importStageplay(importOptions);
                    break;
                }
                case Domain::DocumentObjectType::Presentation: {
                    importPresentation(importOptions);
                    break;
                }
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

    //
    // Прежде чем показать диалог импорта, выведем предупреждение, если нужно
    //
    if ((!docFiles.isEmpty() || !otherFiles.isEmpty()) && !filesToImport.isEmpty()) {
        QString errorMessage;
        auto filesList = [](const QStringList& _files) {
            QString filesList;
            for (const auto& file : _files) {
                QFileInfo fileInfo(file);
                filesList += fileInfo.fileName() + "\n";
            }
            return filesList;
        };
        if (!docFiles.isEmpty()) {
            errorMessage.append(tr("Importing from DOC files is not supported. You need to "
                                   "save the file in DOCX format and repeat the import.\n\nThe "
                                   "following files will not be imported:\n")
                                + filesList(docFiles));
        }
        if (!otherFiles.isEmpty()) {
            QString unsupporterExtensions;
            for (const auto& path : std::as_const(otherFiles)) {
                if (const auto fileExtension = path.split('.').constLast().toLower();
                    !unsupporterExtensions.contains(fileExtension)) {
                    if (!unsupporterExtensions.isEmpty()) {
                        unsupporterExtensions.append(", ");
                    }
                    unsupporterExtensions.append(fileExtension);
                }
            }

            if (!errorMessage.isEmpty()) {
                errorMessage.append("\n\n");
            }
            errorMessage.append(
                tr("Importing from %1 files is not supported.\n\nThe following files "
                   "will not be imported:\n")
                    .arg(unsupporterExtensions.toUpper())
                + filesList(otherFiles));
        }
        auto dialog = new Dialog(topLevelWidget);
        dialog->setContentMaximumWidth(Ui::DesignSystem::dialog().maximumWidth());
        dialog->showDialog(tr("File format not supported"), errorMessage,
                           { { 0, StandardDialog::generateOkTerm(), Dialog::RejectButton } });
        //
        // После того, как сообщение о проблемах импорта будет закрыто, запускаем импорт файлов,
        // которые поддерживаются приложением
        //
        QObject::connect(dialog, &Dialog::finished, dialog, [this, dialog]() {
            dialog->hideDialog();
            importDialog->showDialog();
        });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    }
    //
    // А если все файлы могут быть импортированы, то сразу перейдём к процессу импорта
    //
    else {
        importDialog->showDialog();
    }
}

void ImportManager::Implementation::importSimpleText(const BusinessLayer::ImportOptions& _options)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractSimpleTextImporter> importer;
    {
        const auto importFilePath = _options.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::msOfficeOpenXml())
            || importFilePath.endsWith(ExtensionHelper::openDocumentXml())) {
            importer.reset(new BusinessLayer::SimpleTextDocxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::fountain())
                   || importFilePath.endsWith(ExtensionHelper::markdown())
                   || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::SimpleTextMarkdownImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::pdf())) {
            importer.reset(new BusinessLayer::SimpleTextPdfImporter);
        }
    }

    //
    // Импортируем текстовый документ
    //
    const auto document = importer->importSimpleText(_options);
    const auto documentName = !document.name.isEmpty()
        ? document.name
        : QFileInfo(_options.filePath).completeBaseName();
    emit q->simpleTextImported(documentName, document.text);
}

void ImportManager::Implementation::importAudioplay(const BusinessLayer::ImportOptions& _options)
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
    // Импортируем персонажей
    //
    const auto documents = importer->importDocuments(_options);
    for (const auto& character : documents.characters) {
        emit q->characterImported(character.name, character.content);
    }

    //
    // Импортируем текст аудиопьесы
    //
    const auto audioplay = importer->importAudioplay(_options);
    const auto audioplayName = !audioplay.name.isEmpty()
        ? audioplay.name
        : QFileInfo(_options.filePath).completeBaseName();
    emit q->audioplayImported(audioplayName, audioplay.titlePage, audioplay.text);
}

void ImportManager::Implementation::importComicBook(const BusinessLayer::ImportOptions& _options)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractComicBookImporter> importer;
    {
        const auto importFilePath = _options.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::fountain())
            || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::ComicBookFountainImporter);
        }
    }

    //
    // Импортируем персонажей
    //
    const auto documents = importer->importDocuments(_options);
    for (const auto& character : documents.characters) {
        emit q->characterImported(character.name, character.content);
    }

    //
    // Импортируем текст комикса
    //
    const auto comicbook = importer->importComicBook(_options);
    const auto comicbookName = !comicbook.name.isEmpty()
        ? comicbook.name
        : QFileInfo(_options.filePath).completeBaseName();
    emit q->comicbookImported(comicbookName, comicbook.titlePage, comicbook.text);
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
    const auto novel = importer->importNovel(_options);
    const auto novelName
        = !novel.name.isEmpty() ? novel.name : QFileInfo(_options.filePath).completeBaseName();
    emit q->novelImported(novelName, novel.text);
}

void ImportManager::Implementation::importScreenplay(
    const BusinessLayer::ImportOptions& _importOptions)
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
            importer.reset(new BusinessLayer::ScreenplayDocxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::celtx())) {
            importer.reset(new BusinessLayer::ScreenplayCeltxImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::fountain())
                   || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::ScreenplayFountainImporter);
        } else if (importFilePath.endsWith(ExtensionHelper::pdf())) {
            importer.reset(new BusinessLayer::ScreenplayPdfImporter);
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
    for (const auto& document : documents.research) {
        emit q->documentImported(document);
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

void ImportManager::Implementation::importStageplay(const BusinessLayer::ImportOptions& _options)
{
    //
    // Определим нужный импортер
    //
    QScopedPointer<BusinessLayer::AbstractStageplayImporter> importer;
    {
        const auto importFilePath = _options.filePath.toLower();
        if (importFilePath.endsWith(ExtensionHelper::fountain())
            || importFilePath.endsWith(ExtensionHelper::plainText())) {
            importer.reset(new BusinessLayer::StageplayFountainImporter);
        }
    }

    //
    // Импортируем персонажей
    //
    const auto documents = importer->importDocuments(_options);
    for (const auto& character : documents.characters) {
        emit q->characterImported(character.name, character.content);
    }

    //
    // Импортируем текст пьесы
    //
    const auto stageplay = importer->importStageplay(_options);
    const auto stageplayName = !stageplay.name.isEmpty()
        ? stageplay.name
        : QFileInfo(_options.filePath).completeBaseName();
    emit q->stageplayImported(stageplayName, stageplay.titlePage, stageplay.text);
}

void ImportManager::Implementation::importPresentation(const BusinessLayer::ImportOptions& _options)
{
    //
    // NOTE: Пока мы не умеем рендерить презентации локально, делаем вид, что мы это сделали, хотя
    // по факту презентация будет рендериться в картинки на стороне нашего сервиса и вся магия по
    // работе с ним находится внутри плагина презентаций
    //
    // Поэтому тут делаем вид, типа мы всё импортировали :)
    //
    emit q->presentationImported(
        _options.documentUuid, QFileInfo(_options.filePath).completeBaseName(), _options.filePath);
}


// ****


ImportManager::ImportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget))
{
}

ImportManager::~ImportManager() = default;

void ImportManager::import(const QVector<QString>& _files)
{
    Log::info("Importing started");

    QStringList importFilePaths(_files.begin(), _files.end());

    if (importFilePaths.isEmpty()) {
        //
        // Предоставим пользователю возможность выбрать файл, который он хочет импортировать
        //
        const auto projectImportFolder
            = settingsValue(DataStorageLayer::kProjectImportFolderKey).toString();
        importFilePaths
            = QFileDialog::getOpenFileNames(d->topLevelWidget, tr("Choose files to import"),
                                            projectImportFolder, DialogHelper::filtersForImport());
        if (importFilePaths.isEmpty()) {
            return;
        }

        //
        // Если файлы были выбраны обновим папку, откуда в следующий раз он предположительно опять
        // будет импортировать проекты
        //
        setSettingsValue(DataStorageLayer::kProjectImportFolderKey, importFilePaths.last());
    }

    //
    // Переходим к подготовке импорта
    //
    d->showImportDialogFor(importFilePaths);
}

void ImportManager::importScreenplay(const QString& _filePath, bool _importDocuments)
{
    BusinessLayer::ImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Screenplay;
    options.importCharacters = _importDocuments;
    options.importLocations = _importDocuments;
    options.importResearch = _importDocuments;
    d->importScreenplay(options);
}

void ImportManager::importComicBook(const QString& _filePath, bool _importDocuments)
{
    BusinessLayer::ImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Screenplay;
    options.importCharacters = _importDocuments;
    d->importComicBook(options);
}

void ImportManager::importAudioplay(const QString& _filePath, bool _importDocuments)
{
    BusinessLayer::ImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Screenplay;
    options.importCharacters = _importDocuments;
    d->importAudioplay(options);
}

void ImportManager::importStageplay(const QString& _filePath, bool _importDocuments)
{
    BusinessLayer::ImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Screenplay;
    options.importCharacters = _importDocuments;
    d->importStageplay(options);
}

void ImportManager::importNovel(const QString& _filePath)
{
    BusinessLayer::ImportOptions options;
    options.filePath = _filePath;
    options.documentType = Domain::DocumentObjectType::Novel;
    d->importNovel(options);
}

void ImportManager::importToDocument(const QString& _filePath, const QUuid& _documentUuid,
                                     Domain::DocumentObjectType _type)
{
    switch (_type) {
    case Domain::DocumentObjectType::ScreenplayText: {
        const bool importDocuments = false;
        importScreenplay(_filePath, importDocuments);
        break;
    }
    case Domain::DocumentObjectType::ComicBookText: {
        const bool importDocuments = false;
        importComicBook(_filePath, importDocuments);
        break;
    }
    case Domain::DocumentObjectType::AudioplayText: {
        const bool importDocuments = false;
        importAudioplay(_filePath, importDocuments);
        break;
    }
    case Domain::DocumentObjectType::StageplayText: {
        const bool importDocuments = false;
        importStageplay(_filePath, importDocuments);
        break;
    }
    case Domain::DocumentObjectType::NovelText: {
        importNovel(_filePath);
        break;
    }

    case Domain::DocumentObjectType::Presentation: {
        BusinessLayer::ImportOptions options;
        options.documentUuid = _documentUuid;
        options.filePath = _filePath;
        d->importPresentation(options);
        break;
    }

    default: {
        break;
    }
    }
}

} // namespace ManagementLayer
