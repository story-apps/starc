#include "export_manager.h"

#include <business_layer/export/audioplay/audioplay_docx_exporter.h>
#include <business_layer/export/audioplay/audioplay_export_options.h>
#include <business_layer/export/audioplay/audioplay_fountain_exporter.h>
#include <business_layer/export/audioplay/audioplay_pdf_exporter.h>
#include <business_layer/export/characters/character_docx_exporter.h>
#include <business_layer/export/characters/character_export_options.h>
#include <business_layer/export/characters/character_pdf_exporter.h>
#include <business_layer/export/characters/characters_docx_exporter.h>
#include <business_layer/export/characters/characters_export_options.h>
#include <business_layer/export/characters/characters_pdf_exporter.h>
#include <business_layer/export/comic_book/comic_book_docx_exporter.h>
#include <business_layer/export/comic_book/comic_book_export_options.h>
#include <business_layer/export/comic_book/comic_book_fountain_exporter.h>
#include <business_layer/export/comic_book/comic_book_pdf_exporter.h>
#include <business_layer/export/export_options.h>
#include <business_layer/export/locations/location_docx_exporter.h>
#include <business_layer/export/locations/location_export_options.h>
#include <business_layer/export/locations/location_pdf_exporter.h>
#include <business_layer/export/locations/locations_docx_exporter.h>
#include <business_layer/export/locations/locations_pdf_exporter.h>
#include <business_layer/export/novel/novel_docx_exporter.h>
#include <business_layer/export/novel/novel_export_options.h>
#include <business_layer/export/novel/novel_markdown_exporter.h>
#include <business_layer/export/novel/novel_pdf_exporter.h>
#include <business_layer/export/screenplay/screenplay_docx_exporter.h>
#include <business_layer/export/screenplay/screenplay_export_options.h>
#include <business_layer/export/screenplay/screenplay_fdx_exporter.h>
#include <business_layer/export/screenplay/screenplay_fountain_exporter.h>
#include <business_layer/export/screenplay/screenplay_pdf_exporter.h>
#include <business_layer/export/simple_text/simple_text_docx_exporter.h>
#include <business_layer/export/simple_text/simple_text_markdown_exporter.h>
#include <business_layer/export/simple_text/simple_text_pdf_exporter.h>
#include <business_layer/export/stageplay/stageplay_docx_exporter.h>
#include <business_layer/export/stageplay/stageplay_export_options.h>
#include <business_layer/export/stageplay/stageplay_fountain_exporter.h>
#include <business_layer/export/stageplay/stageplay_pdf_exporter.h>
#include <business_layer/export/worlds/world_docx_exporter.h>
#include <business_layer/export/worlds/world_export_options.h>
#include <business_layer/export/worlds/world_pdf_exporter.h>
#include <business_layer/export/worlds/worlds_docx_exporter.h>
#include <business_layer/export/worlds/worlds_pdf_exporter.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/worlds/world_model.h>
#include <business_layer/model/worlds/worlds_model.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/export/audioplay_export_dialog.h>
#include <ui/export/character_export_dialog.h>
#include <ui/export/characters_export_dialog.h>
#include <ui/export/comic_book_export_dialog.h>
#include <ui/export/location_export_dialog.h>
#include <ui/export/locations_export_dialog.h>
#include <ui/export/novel_export_dialog.h>
#include <ui/export/screenplay_export_dialog.h>
#include <ui/export/simple_text_export_dialog.h>
#include <ui/export/stageplay_export_dialog.h>
#include <ui/export/world_export_dialog.h>
#include <ui/export/worlds_export_dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>
#include <utils/logging.h>

#include <QDesktopServices>
#include <QFileDialog>


namespace ManagementLayer {

namespace {

/**
 * @brief Ключ для сохранения пути экспортируемого документа
 */
QString exportModelKey(BusinessLayer::AbstractModel* _model)
{
    return QString("%1/%2").arg(DataStorageLayer::kProjectExportFolderKey,
                                _model->document()->uuid().toString());
}

} // namespace

class ExportManager::Implementation
{
public:
    explicit Implementation(ExportManager* _parent, QWidget* _topLevelWidget);

    /**
     * @brief Экспортировать документ
     */
    void exportScreenplays(const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models,
                           int _currentModelIndex);
    void exportScreenplay(BusinessLayer::AbstractModel* _model,
                          const BusinessLayer::ScreenplayExportOptions& _options);
    void exportComicBooks(const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models,
                          int _currentModelIndex);
    void exportAudioplays(const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models,
                          int _currentModelIndex);
    void exportStageplays(const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models,
                          int _currentModelIndex);
    void exportNovels(const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models,
                      int _currentModelIndex);
    //
    void exportSimpleText(BusinessLayer::AbstractModel* _model);
    void exportCharacter(BusinessLayer::AbstractModel* _model);
    void exportCharacters(BusinessLayer::AbstractModel* _model);
    void exportLocation(BusinessLayer::AbstractModel* _model);
    void exportLocations(BusinessLayer::AbstractModel* _model);
    void exportWorld(BusinessLayer::AbstractModel* _model);
    void exportWorlds(BusinessLayer::AbstractModel* _model);

    //
    // Данные
    //

    ExportManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ScreenplayExportDialog* screenplayExportDialog = nullptr;
    Ui::ComicBookExportDialog* comicBookExportDialog = nullptr;
    Ui::AudioplayExportDialog* audioplayExportDialog = nullptr;
    Ui::StageplayExportDialog* stageplayExportDialog = nullptr;
    Ui::NovelExportDialog* novelExportDialog = nullptr;
    Ui::SimpleTextExportDialog* simpleTextExportDialog = nullptr;
    Ui::CharacterExportDialog* characterExportDialog = nullptr;
    Ui::CharactersExportDialog* charactersExportDialog = nullptr;
    Ui::LocationExportDialog* locationExportDialog = nullptr;
    Ui::LocationsExportDialog* locationsExportDialog = nullptr;
    Ui::WorldExportDialog* worldExportDialog = nullptr;
    Ui::WorldsExportDialog* worldsExportDialog = nullptr;
};

ExportManager::Implementation::Implementation(ExportManager* _parent, QWidget* _topLevelWidget)
    : q(_parent)
    , topLevelWidget(_topLevelWidget)
{
}

void ExportManager::Implementation::exportScreenplays(
    const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models, int _currentModelIndex)
{
    using namespace BusinessLayer;

    if (screenplayExportDialog == nullptr) {
        const auto currentVersion = _models.constFirst().second;
        screenplayExportDialog = new Ui::ScreenplayExportDialog(
            currentVersion->document()->uuid().toString(), topLevelWidget);
        connect(screenplayExportDialog, &Ui::ScreenplayExportDialog::exportRequested,
                screenplayExportDialog, [this, _models] {
                    auto exportOptions = screenplayExportDialog->exportOptions();

                    //
                    // Предоставим пользователю возможность выбрать файл, куда он будет
                    // экспортировать
                    //
                    QString exportFilter;
                    QString exportExtension;
                    switch (exportOptions.fileFormat) {
                    default:
                    case ExportFileFormat::Pdf: {
                        exportFilter = DialogHelper::pdfFilter();
                        exportExtension = ExtensionHelper::pdf();
                        break;
                    }
                    case ExportFileFormat::Docx: {
                        exportFilter = DialogHelper::msWordFilter();
                        exportExtension = ExtensionHelper::msOfficeOpenXml();
                        break;
                    }
                    case ExportFileFormat::Fdx: {
                        exportFilter = DialogHelper::finalDraftFilter();
                        exportExtension = ExtensionHelper::finalDraft();
                        break;
                    }
                    case ExportFileFormat::Fountain: {
                        exportFilter = DialogHelper::fountainFilter();
                        exportExtension = ExtensionHelper::fountain();
                        break;
                    }
                    }
                    auto model = _models.at(screenplayExportDialog->selectedVersion()).second;
                    const auto screenplayTextModel
                        = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    const auto projectExportFolder
                        = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                    auto modelExportFile = QString("%1/%2.%3")
                                               .arg(projectExportFolder,
                                                    screenplayTextModel->informationModel()->name(),
                                                    exportExtension);
                    modelExportFile
                        = settingsValue(exportModelKey(screenplayTextModel), modelExportFile)
                              .toString();
                    if (!modelExportFile.endsWith(exportExtension)) {
                        const auto dotIndex = modelExportFile.lastIndexOf('.');
                        if (dotIndex == -1) {
                            modelExportFile += '.';
                        } else {
                            modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                        }
                        modelExportFile += exportExtension;
                    }
                    auto exportFilePath = QFileDialog::getSaveFileName(
                        topLevelWidget, tr("Choose the file to export"), modelExportFile,
                        exportFilter);
                    if (exportFilePath.isEmpty()) {
                        return;
                    }

                    //
                    // Сохраним файл, в который экспортировали данную модель
                    //
                    setSettingsValue(exportModelKey(screenplayTextModel), exportFilePath);

                    //
                    // Если файл был выбран
                    //
                    exportOptions.filePath = exportFilePath;
                    exportScreenplay(screenplayTextModel, exportOptions);

                    //
                    // Если необходимо, откроем экспортированный документ
                    //
                    if (screenplayExportDialog->openDocumentAfterExport()) {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                    }
                    //
                    // ... и закрываем диалог экспорта
                    //
                    screenplayExportDialog->hideDialog();
                });
        connect(screenplayExportDialog, &Ui::ScreenplayExportDialog::canceled,
                screenplayExportDialog, &Ui::ScreenplayExportDialog::hideDialog);
        connect(screenplayExportDialog, &Ui::ScreenplayExportDialog::disappeared,
                screenplayExportDialog, [this] {
                    screenplayExportDialog->deleteLater();
                    screenplayExportDialog = nullptr;
                });
    }

    QVector<QString> versions;
    for (const auto& version : _models) {
        versions.append(version.first);
    }
    screenplayExportDialog->setVersions(versions, _currentModelIndex);
    screenplayExportDialog->showDialog();
}

void ExportManager::Implementation::exportScreenplay(
    BusinessLayer::AbstractModel* _model, const BusinessLayer::ScreenplayExportOptions& _options)
{
    using namespace BusinessLayer;

    ScreenplayExportOptions exportOptions = _options;

    //
    // ... проверяем возможность записи в файл
    //
    QFile file(exportOptions.filePath);
    const bool canWrite = file.open(QIODevice::WriteOnly);
    file.close();
    if (!canWrite) {
        //
        // ... предупреждаем
        //
        QString errorMessage;
        const QFileInfo fileInfo(exportOptions.filePath);
        if (fileInfo.exists()) {
            errorMessage = tr("Can't write to file. Looks like it's opened by another "
                              "application. Please close it and retry the export.");
        } else {
            errorMessage = tr("Can't write to file. Check permissions to write in the "
                              "chosen folder or choose another folder.");
        }
        StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
        return;
    }

    //
    // ... донастроим параметры экспорта
    //
    const auto screenplayTextModel = qobject_cast<ScreenplayTextModel*>(_model);
    const auto screenplayInformation = screenplayTextModel->informationModel();
    exportOptions.templateId = screenplayInformation->templateId();
    exportOptions.showScenesNumbers = screenplayInformation->showSceneNumbers();
    exportOptions.showScenesNumbersOnLeft = screenplayInformation->showSceneNumbersOnLeft();
    exportOptions.showScenesNumbersOnRight = screenplayInformation->showSceneNumbersOnRight();
    exportOptions.showDialoguesNumbers = screenplayInformation->showDialoguesNumbers();
    exportOptions.header = screenplayInformation->header();
    exportOptions.printHeaderOnTitlePage = screenplayInformation->printHeaderOnTitlePage();
    exportOptions.footer = screenplayInformation->footer();
    exportOptions.printFooterOnTitlePage = screenplayInformation->printFooterOnTitlePage();
    //
    // ... при необходимости подсветить персонажей, делаем это
    //
    if (exportOptions.highlightCharacters) {
        const auto charactersModel = screenplayTextModel->charactersList();
        for (int row = 0; row < charactersModel->rowCount(); ++row) {
            const auto characterName = charactersModel->index(row, 0).data().toString();
            const auto character = screenplayTextModel->character(characterName);
            if (character != nullptr && character->color().isValid()) {
                exportOptions.highlightCharactersList.insert(character->name(), character->color());
            }
        }
    }
    //
    // ... обновим папку, куда в следующий раз он предположительно опять будет
    //     экспортировать
    //
    setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                     QFileInfo(exportOptions.filePath).dir().absolutePath());
    //
    // ... и экспортируем документ
    //
    QScopedPointer<ScreenplayExporter> exporter;
    switch (exportOptions.fileFormat) {
    default:
    case ExportFileFormat::Pdf: {
        exporter.reset(new ScreenplayPdfExporter);
        break;
    }
    case ExportFileFormat::Docx: {
        exporter.reset(new ScreenplayDocxExporter);
        break;
    }
    case ExportFileFormat::Fdx: {
        exporter.reset(new ScreenplayFdxExporter);
        break;
    }
    case ExportFileFormat::Fountain: {
        exporter.reset(new ScreenplayFountainExporter);
        break;
    }
    }
    if (exporter.isNull()) {
        return;
    }
    exporter->exportTo(screenplayTextModel, exportOptions);
}

void ExportManager::Implementation::exportComicBooks(
    const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models, int _currentModelIndex)
{
    using namespace BusinessLayer;

    if (comicBookExportDialog == nullptr) {
        const auto currentVersion = _models.constFirst().second;
        comicBookExportDialog = new Ui::ComicBookExportDialog(
            currentVersion->document()->uuid().toString(), topLevelWidget);
        connect(
            comicBookExportDialog, &Ui::ComicBookExportDialog::exportRequested,
            comicBookExportDialog, [this, _models] {
                auto exportOptions = comicBookExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет
                // экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                case ExportFileFormat::Fountain: {
                    exportFilter = DialogHelper::fountainFilter();
                    exportExtension = ExtensionHelper::fountain();
                    break;
                }
                }
                auto model = _models.at(comicBookExportDialog->selectedVersion()).second;
                const auto comicBookTextModel
                    = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, comicBookTextModel->informationModel()->name(),
                               exportExtension);
                modelExportFile
                    = settingsValue(exportModelKey(comicBookTextModel), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(comicBookTextModel), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }
                //
                // ... донастроим параметры экспорта
                //
                const auto comicBookInformation = comicBookTextModel->informationModel();
                exportOptions.templateId = comicBookInformation->templateId();
                exportOptions.header = comicBookInformation->header();
                exportOptions.footer = comicBookInformation->footer();
                //
                // ... при необходимости подсветить персонажей, делаем это
                //
                if (exportOptions.highlightCharacters) {
                    const auto charactersModel = comicBookTextModel->charactersList();
                    for (int row = 0; row < charactersModel->rowCount(); ++row) {
                        const auto characterName = charactersModel->index(row, 0).data().toString();
                        const auto character = comicBookTextModel->character(characterName);
                        if (character != nullptr && character->color().isValid()) {
                            exportOptions.highlightCharactersList.insert(character->name(),
                                                                         character->color());
                        }
                    }
                }
                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::ComicBookExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::ComicBookPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::ComicBookDocxExporter);
                    break;
                }
                case ExportFileFormat::Fountain: {
                    exporter.reset(new BusinessLayer::ComicBookFountainExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(comicBookTextModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (comicBookExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                comicBookExportDialog->hideDialog();
            });
        connect(comicBookExportDialog, &Ui::ComicBookExportDialog::canceled, comicBookExportDialog,
                &Ui::ComicBookExportDialog::hideDialog);
        connect(comicBookExportDialog, &Ui::ComicBookExportDialog::disappeared,
                comicBookExportDialog, [this] {
                    comicBookExportDialog->deleteLater();
                    comicBookExportDialog = nullptr;
                });
    }

    QVector<QString> versions;
    for (const auto& version : _models) {
        versions.append(version.first);
    }
    comicBookExportDialog->setVersions(versions, _currentModelIndex);
    comicBookExportDialog->showDialog();
}

void ExportManager::Implementation::exportAudioplays(
    const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models, int _currentModelIndex)
{
    using namespace BusinessLayer;

    if (audioplayExportDialog == nullptr) {
        const auto currentVersion = _models.constFirst().second;
        audioplayExportDialog = new Ui::AudioplayExportDialog(
            currentVersion->document()->uuid().toString(), topLevelWidget);
        connect(
            audioplayExportDialog, &Ui::AudioplayExportDialog::exportRequested,
            audioplayExportDialog, [this, _models] {
                auto exportOptions = audioplayExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                case ExportFileFormat::Fountain: {
                    exportFilter = DialogHelper::fountainFilter();
                    exportExtension = ExtensionHelper::fountain();
                    break;
                }
                }
                auto model = _models.at(audioplayExportDialog->selectedVersion()).second;
                const auto audioplayTextModel
                    = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, audioplayTextModel->informationModel()->name(),
                               exportExtension);
                modelExportFile
                    = settingsValue(exportModelKey(audioplayTextModel), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(audioplayTextModel), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... донастроим параметры экспорта
                //
                const auto audioplayInformation = audioplayTextModel->informationModel();
                exportOptions.templateId = audioplayInformation->templateId();
                exportOptions.showBlockNumbers = audioplayInformation->showBlockNumbers();
                exportOptions.header = audioplayInformation->header();
                exportOptions.printHeaderOnTitlePage
                    = audioplayInformation->printHeaderOnTitlePage();
                exportOptions.footer = audioplayInformation->footer();
                exportOptions.printFooterOnTitlePage
                    = audioplayInformation->printFooterOnTitlePage();
                //
                // ... при необходимости подсветить персонажей, делаем это
                //
                if (exportOptions.highlightCharacters) {
                    const auto charactersModel = audioplayTextModel->charactersList();
                    for (int row = 0; row < charactersModel->rowCount(); ++row) {
                        const auto characterName = charactersModel->index(row, 0).data().toString();
                        const auto character = audioplayTextModel->character(characterName);
                        if (character != nullptr && character->color().isValid()) {
                            exportOptions.highlightCharactersList.insert(character->name(),
                                                                         character->color());
                        }
                    }
                }
                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::AudioplayPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::AudioplayDocxExporter);
                    break;
                }
                case ExportFileFormat::Fountain: {
                    exporter.reset(new BusinessLayer::AudioplayFountainExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(audioplayTextModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (audioplayExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                audioplayExportDialog->hideDialog();
            });
        connect(audioplayExportDialog, &Ui::AudioplayExportDialog::canceled, audioplayExportDialog,
                &Ui::AudioplayExportDialog::hideDialog);
        connect(audioplayExportDialog, &Ui::AudioplayExportDialog::disappeared,
                audioplayExportDialog, [this] {
                    audioplayExportDialog->deleteLater();
                    audioplayExportDialog = nullptr;
                });
    }

    QVector<QString> versions;
    for (const auto& version : _models) {
        versions.append(version.first);
    }
    audioplayExportDialog->setVersions(versions, _currentModelIndex);
    audioplayExportDialog->showDialog();
}

void ExportManager::Implementation::exportStageplays(
    const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models, int _currentModelIndex)
{
    using namespace BusinessLayer;

    if (stageplayExportDialog == nullptr) {
        const auto currentVersion = _models.constFirst().second;
        stageplayExportDialog = new Ui::StageplayExportDialog(
            currentVersion->document()->uuid().toString(), topLevelWidget);
        connect(
            stageplayExportDialog, &Ui::StageplayExportDialog::exportRequested,
            stageplayExportDialog, [this, _models] {
                auto exportOptions = stageplayExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                case ExportFileFormat::Fountain: {
                    exportFilter = DialogHelper::fountainFilter();
                    exportExtension = ExtensionHelper::fountain();
                    break;
                }
                }
                auto model = _models.at(stageplayExportDialog->selectedVersion()).second;
                const auto stageplayTextModel
                    = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, stageplayTextModel->informationModel()->name(),
                               exportExtension);
                modelExportFile
                    = settingsValue(exportModelKey(stageplayTextModel), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(stageplayTextModel), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... донастроим параметры экспорта
                //
                const auto stageplayInformation = stageplayTextModel->informationModel();
                exportOptions.templateId = stageplayInformation->templateId();
                exportOptions.header = stageplayInformation->header();
                exportOptions.printHeaderOnTitlePage
                    = stageplayInformation->printHeaderOnTitlePage();
                exportOptions.footer = stageplayInformation->footer();
                exportOptions.printFooterOnTitlePage
                    = stageplayInformation->printFooterOnTitlePage();
                //
                // ... при необходимости подсветить персонажей, делаем это
                //
                if (exportOptions.highlightCharacters) {
                    const auto charactersModel = stageplayTextModel->charactersList();
                    for (int row = 0; row < charactersModel->rowCount(); ++row) {
                        const auto characterName = charactersModel->index(row, 0).data().toString();
                        const auto character = stageplayTextModel->character(characterName);
                        if (character != nullptr && character->color().isValid()) {
                            exportOptions.highlightCharactersList.insert(character->name(),
                                                                         character->color());
                        }
                    }
                }
                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::StageplayPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::StageplayDocxExporter);
                    break;
                }
                case ExportFileFormat::Fountain: {
                    exporter.reset(new BusinessLayer::StageplayFountainExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(stageplayTextModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (stageplayExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                stageplayExportDialog->hideDialog();
            });
        connect(stageplayExportDialog, &Ui::StageplayExportDialog::canceled, stageplayExportDialog,
                &Ui::StageplayExportDialog::hideDialog);
        connect(stageplayExportDialog, &Ui::StageplayExportDialog::disappeared,
                stageplayExportDialog, [this] {
                    stageplayExportDialog->deleteLater();
                    stageplayExportDialog = nullptr;
                });
    }

    QVector<QString> versions;
    for (const auto& version : _models) {
        versions.append(version.first);
    }
    stageplayExportDialog->setVersions(versions, _currentModelIndex);
    stageplayExportDialog->showDialog();
}

void ExportManager::Implementation::exportNovels(
    const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models, int _currentModelIndex)
{
    using namespace BusinessLayer;

    if (novelExportDialog == nullptr) {
        const auto currentVersion = _models.constFirst().second;
        novelExportDialog = new Ui::NovelExportDialog(currentVersion->document()->uuid().toString(),
                                                      topLevelWidget);
        connect(
            novelExportDialog, &Ui::NovelExportDialog::exportRequested, novelExportDialog,
            [this, _models] {
                auto exportOptions = novelExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                case ExportFileFormat::Markdown: {
                    exportFilter = DialogHelper::markdownFilter();
                    exportExtension = ExtensionHelper::markdown();
                    break;
                }
                }
                auto model = _models.at(novelExportDialog->selectedVersion()).second;
                const auto novelTextModel = qobject_cast<BusinessLayer::NovelTextModel*>(model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, novelTextModel->informationModel()->name(),
                               exportExtension);
                modelExportFile
                    = settingsValue(exportModelKey(novelTextModel), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(novelTextModel), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... донастроим параметры экспорта
                //
                const auto novelInformation = novelTextModel->informationModel();
                exportOptions.templateId = novelInformation->templateId();
                exportOptions.header = novelInformation->header();
                exportOptions.printHeaderOnTitlePage = novelInformation->printHeaderOnTitlePage();
                exportOptions.footer = novelInformation->footer();
                exportOptions.printFooterOnTitlePage = novelInformation->printFooterOnTitlePage();
                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::NovelPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::NovelDocxExporter);
                    break;
                }
                case ExportFileFormat::Markdown: {
                    exporter.reset(new BusinessLayer::NovelMarkdownExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(novelTextModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (novelExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                novelExportDialog->hideDialog();
            });
        connect(novelExportDialog, &Ui::NovelExportDialog::canceled, novelExportDialog,
                &Ui::NovelExportDialog::hideDialog);
        connect(novelExportDialog, &Ui::NovelExportDialog::disappeared, novelExportDialog, [this] {
            novelExportDialog->deleteLater();
            novelExportDialog = nullptr;
        });
    }

    QVector<QString> versions;
    for (const auto& version : _models) {
        versions.append(version.first);
    }
    novelExportDialog->setVersions(versions, _currentModelIndex);
    novelExportDialog->showDialog();
}

void ExportManager::Implementation::exportSimpleText(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (simpleTextExportDialog == nullptr) {
        simpleTextExportDialog
            = new Ui::SimpleTextExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            simpleTextExportDialog, &Ui::SimpleTextExportDialog::exportRequested,
            simpleTextExportDialog, [this, _model] {
                auto exportOptions = simpleTextExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                case ExportFileFormat::Markdown: {
                    exportFilter = DialogHelper::markdownFilter();
                    exportExtension = ExtensionHelper::markdown();
                    break;
                }
                }
                const auto simpleTextModel = qobject_cast<BusinessLayer::SimpleTextModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, simpleTextModel->name(), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... донастроим параметры экспорта
                //
                exportOptions.templateId = TemplatesFacade::simpleTextTemplate().id();
                exportOptions.includeTitlePage = false;
                exportOptions.includeSynopsis = false;
                exportOptions.includeText = true;
                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::SimpleTextPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::SimpleTextDocxExporter);
                    break;
                }
                case ExportFileFormat::Markdown: {
                    exporter.reset(new BusinessLayer::SimpleTextMarkdownExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(simpleTextModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (simpleTextExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                simpleTextExportDialog->hideDialog();
            });
        connect(simpleTextExportDialog, &Ui::SimpleTextExportDialog::canceled,
                simpleTextExportDialog, &Ui::SimpleTextExportDialog::hideDialog);
        connect(simpleTextExportDialog, &Ui::SimpleTextExportDialog::disappeared,
                simpleTextExportDialog, [this] {
                    simpleTextExportDialog->deleteLater();
                    simpleTextExportDialog = nullptr;
                });
    }

    simpleTextExportDialog->showDialog();
}

void ExportManager::Implementation::exportCharacter(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (characterExportDialog == nullptr) {
        characterExportDialog
            = new Ui::CharacterExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            characterExportDialog, &Ui::CharacterExportDialog::exportRequested,
            characterExportDialog, [this, _model] {
                auto exportOptions = characterExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                }
                const auto characterModel = qobject_cast<BusinessLayer::CharacterModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, characterModel->name(), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::CharacterPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::CharacterDocxExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(characterModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (characterExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                characterExportDialog->hideDialog();
            });
        connect(characterExportDialog, &Ui::CharacterExportDialog::canceled, characterExportDialog,
                &Ui::CharacterExportDialog::hideDialog);
        connect(characterExportDialog, &Ui::CharacterExportDialog::disappeared,
                characterExportDialog, [this] {
                    characterExportDialog->deleteLater();
                    characterExportDialog = nullptr;
                });
    }

    characterExportDialog->showDialog();
}

void ExportManager::Implementation::exportCharacters(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (charactersExportDialog == nullptr) {
        charactersExportDialog
            = new Ui::CharactersExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            charactersExportDialog, &Ui::CharactersExportDialog::exportRequested,
            charactersExportDialog, [this, _model] {
                auto exportOptions = charactersExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                }
                const auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, tr("Characters"), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::CharactersPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::CharactersDocxExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(charactersModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (charactersExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                charactersExportDialog->hideDialog();
            });
        connect(charactersExportDialog, &Ui::CharactersExportDialog::canceled,
                charactersExportDialog, &Ui::CharactersExportDialog::hideDialog);
        connect(charactersExportDialog, &Ui::CharactersExportDialog::disappeared,
                charactersExportDialog, [this] {
                    charactersExportDialog->deleteLater();
                    charactersExportDialog = nullptr;
                });
    }

    charactersExportDialog->setModel(_model);
    charactersExportDialog->showDialog();
}

void ExportManager::Implementation::exportLocation(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (locationExportDialog == nullptr) {
        locationExportDialog
            = new Ui::LocationExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            locationExportDialog, &Ui::LocationExportDialog::exportRequested, locationExportDialog,
            [this, _model] {
                auto exportOptions = locationExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                }
                const auto locationModel = qobject_cast<BusinessLayer::LocationModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, locationModel->name(), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::LocationPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::LocationDocxExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(locationModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (locationExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                locationExportDialog->hideDialog();
            });
        connect(locationExportDialog, &Ui::LocationExportDialog::canceled, locationExportDialog,
                &Ui::LocationExportDialog::hideDialog);
        connect(locationExportDialog, &Ui::LocationExportDialog::disappeared, locationExportDialog,
                [this] {
                    locationExportDialog->deleteLater();
                    locationExportDialog = nullptr;
                });
    }

    locationExportDialog->showDialog();
}

void ExportManager::Implementation::exportLocations(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (locationsExportDialog == nullptr) {
        locationsExportDialog
            = new Ui::LocationsExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            locationsExportDialog, &Ui::LocationsExportDialog::exportRequested,
            locationsExportDialog, [this, _model] {
                auto exportOptions = locationsExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                }
                const auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, tr("Locations"), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::LocationsPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::LocationsDocxExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(locationsModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (locationsExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                locationsExportDialog->hideDialog();
            });
        connect(locationsExportDialog, &Ui::LocationsExportDialog::canceled, locationsExportDialog,
                &Ui::LocationsExportDialog::hideDialog);
        connect(locationsExportDialog, &Ui::LocationsExportDialog::disappeared,
                locationsExportDialog, [this] {
                    locationsExportDialog->deleteLater();
                    locationsExportDialog = nullptr;
                });
    }

    locationsExportDialog->setModel(_model);
    locationsExportDialog->showDialog();
}

void ExportManager::Implementation::exportWorld(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (worldExportDialog == nullptr) {
        worldExportDialog
            = new Ui::WorldExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            worldExportDialog, &Ui::WorldExportDialog::exportRequested, worldExportDialog,
            [this, _model] {
                auto exportOptions = worldExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                }
                const auto worldModel = qobject_cast<BusinessLayer::WorldModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3")
                          .arg(projectExportFolder, worldModel->name(), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::WorldPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::WorldDocxExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(worldModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (worldExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                worldExportDialog->hideDialog();
            });
        connect(worldExportDialog, &Ui::WorldExportDialog::canceled, worldExportDialog,
                &Ui::WorldExportDialog::hideDialog);
        connect(worldExportDialog, &Ui::WorldExportDialog::disappeared, worldExportDialog, [this] {
            worldExportDialog->deleteLater();
            worldExportDialog = nullptr;
        });
    }

    worldExportDialog->showDialog();
}

void ExportManager::Implementation::exportWorlds(BusinessLayer::AbstractModel* _model)
{
    using namespace BusinessLayer;

    if (worldsExportDialog == nullptr) {
        worldsExportDialog
            = new Ui::WorldsExportDialog(_model->document()->uuid().toString(), topLevelWidget);
        connect(
            worldsExportDialog, &Ui::WorldsExportDialog::exportRequested, worldsExportDialog,
            [this, _model] {
                auto exportOptions = worldsExportDialog->exportOptions();

                //
                // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
                //
                QString exportFilter;
                QString exportExtension;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case ExportFileFormat::Docx: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                }
                const auto worldsModel = qobject_cast<BusinessLayer::WorldsModel*>(_model);
                const auto projectExportFolder
                    = settingsValue(DataStorageLayer::kProjectExportFolderKey).toString();
                auto modelExportFile
                    = QString("%1/%2.%3").arg(projectExportFolder, tr("Worlds"), exportExtension);
                modelExportFile = settingsValue(exportModelKey(_model), modelExportFile).toString();
                if (!modelExportFile.endsWith(exportExtension)) {
                    const auto dotIndex = modelExportFile.lastIndexOf('.');
                    if (dotIndex == -1) {
                        modelExportFile += '.';
                    } else {
                        modelExportFile = modelExportFile.mid(0, dotIndex + 1);
                    }
                    modelExportFile += exportExtension;
                }
                auto exportFilePath = QFileDialog::getSaveFileName(
                    topLevelWidget, tr("Choose the file to export"), modelExportFile, exportFilter);
                if (exportFilePath.isEmpty()) {
                    return;
                }

                //
                // Сохраним файл, в который экспортировали данную модель
                //
                setSettingsValue(exportModelKey(_model), exportFilePath);

                //
                // Если файл был выбран
                //
                exportOptions.filePath = exportFilePath;
                //
                // ... проверяем возможность записи в файл
                //
                QFile file(exportFilePath);
                const bool canWrite = file.open(QIODevice::WriteOnly);
                file.close();
                if (!canWrite) {
                    //
                    // ... предупреждаем
                    //
                    QString errorMessage;
                    const QFileInfo fileInfo(exportFilePath);
                    if (fileInfo.exists()) {
                        errorMessage = tr("Can't write to file. Looks like it's opened by another "
                                          "application. Please close it and retry the export.");
                    } else {
                        errorMessage = tr("Can't write to file. Check permissions to write in the "
                                          "chosen folder or choose another folder.");
                    }
                    StandardDialog::information(topLevelWidget, tr("Export error"), errorMessage);
                    return;
                }

                //
                // ... обновим папку, куда в следующий раз он предположительно опять будет
                //     экспортировать
                //
                setSettingsValue(DataStorageLayer::kProjectExportFolderKey,
                                 QFileInfo(exportFilePath).dir().absolutePath());
                //
                // ... и экспортируем документ
                //
                QScopedPointer<BusinessLayer::AbstractExporter> exporter;
                switch (exportOptions.fileFormat) {
                default:
                case ExportFileFormat::Pdf: {
                    exporter.reset(new BusinessLayer::WorldsPdfExporter);
                    break;
                }
                case ExportFileFormat::Docx: {
                    exporter.reset(new BusinessLayer::WorldsDocxExporter);
                    break;
                }
                }
                if (exporter.isNull()) {
                    return;
                }
                exporter->exportTo(worldsModel, exportOptions);

                //
                // Если необходимо, откроем экспортированный документ
                //
                if (worldsExportDialog->openDocumentAfterExport()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(exportOptions.filePath));
                }
                //
                // ... и закрываем диалог экспорта
                //
                worldsExportDialog->hideDialog();
            });
        connect(worldsExportDialog, &Ui::WorldsExportDialog::canceled, worldsExportDialog,
                &Ui::WorldsExportDialog::hideDialog);
        connect(worldsExportDialog, &Ui::WorldsExportDialog::disappeared, worldsExportDialog,
                [this] {
                    worldsExportDialog->deleteLater();
                    worldsExportDialog = nullptr;
                });
    }

    worldsExportDialog->setModel(_model);
    worldsExportDialog->showDialog();
}


// ****


ExportManager::ExportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget))
{
}

ExportManager::~ExportManager() = default;

bool ExportManager::canExportDocument(BusinessLayer::AbstractModel* _model) const
{
    if (_model == nullptr || _model->document() == nullptr) {
        return false;
    }

    switch (_model->document()->type()) {
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::AudioplayTitlePage:
    case Domain::DocumentObjectType::AudioplaySynopsis:
    case Domain::DocumentObjectType::AudioplayText:
    //
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::ComicBookTitlePage:
    case Domain::DocumentObjectType::ComicBookSynopsis:
    case Domain::DocumentObjectType::ComicBookText:
    //
    case Domain::DocumentObjectType::Screenplay:
    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis:
    case Domain::DocumentObjectType::ScreenplayTreatment:
    case Domain::DocumentObjectType::ScreenplayText:
    case Domain::DocumentObjectType::ScreenplayStatistics:
    //
    case Domain::DocumentObjectType::Stageplay:
    case Domain::DocumentObjectType::StageplayTitlePage:
    case Domain::DocumentObjectType::StageplaySynopsis:
    case Domain::DocumentObjectType::StageplayText:
    //
    case Domain::DocumentObjectType::Novel:
    case Domain::DocumentObjectType::NovelTitlePage:
    case Domain::DocumentObjectType::NovelSynopsis:
    case Domain::DocumentObjectType::NovelOutline:
    case Domain::DocumentObjectType::NovelText:
    case Domain::DocumentObjectType::NovelStatistics:
    //
    case Domain::DocumentObjectType::SimpleText:
    //
    case Domain::DocumentObjectType::Character:
    case Domain::DocumentObjectType::Characters:
    case Domain::DocumentObjectType::Location:
    case Domain::DocumentObjectType::Locations:
    case Domain::DocumentObjectType::World:
    case Domain::DocumentObjectType::Worlds:
        return true;

    default:
        return false;
    }
}

void ExportManager::exportDocument(
    const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models, int _currentModelIndex)
{
    if (_models.isEmpty()) {
        return;
    }

    auto firstModel = _models.constFirst().second;
    if (!canExportDocument(firstModel)) {
        return;
    }

    Log::info("Exporting started. Export document of type %1",
              Domain::mimeTypeFor(firstModel->document()->type()).constData());

    switch (firstModel->document()->type()) {
    case Domain::DocumentObjectType::ScreenplayText: {
        d->exportScreenplays(_models, _currentModelIndex);
        break;
    }

    case Domain::DocumentObjectType::ComicBookText: {
        d->exportComicBooks(_models, _currentModelIndex);
        break;
    }

    case Domain::DocumentObjectType::AudioplayText: {
        d->exportAudioplays(_models, _currentModelIndex);
        break;
    }

    case Domain::DocumentObjectType::StageplayText: {
        d->exportStageplays(_models, _currentModelIndex);
        break;
    }

    case Domain::DocumentObjectType::NovelText: {
        d->exportNovels(_models, _currentModelIndex);
        break;
    }

    case Domain::DocumentObjectType::SimpleText: {
        d->exportSimpleText(firstModel);
        break;
    }

    case Domain::DocumentObjectType::Character: {
        d->exportCharacter(firstModel);
        break;
    }

    case Domain::DocumentObjectType::Characters: {
        d->exportCharacters(firstModel);
        break;
    }

    case Domain::DocumentObjectType::Location: {
        d->exportLocation(firstModel);
        break;
    }

    case Domain::DocumentObjectType::Locations: {
        d->exportLocations(firstModel);
        break;
    }

    case Domain::DocumentObjectType::World: {
        d->exportWorld(firstModel);
        break;
    }

    case Domain::DocumentObjectType::Worlds: {
        d->exportWorlds(firstModel);
        break;
    }

    default:
        break;
    }
}

void ExportManager::exportDocument(BusinessLayer::AbstractModel* _model, const QString& _filePath)
{
    if (!canExportDocument(_model)) {
        return;
    }

    Log::info("Exporting started. Export document of type %1 to file %2",
              Domain::mimeTypeFor(_model->document()->type()).constData(), _filePath);

    switch (_model->document()->type()) {
    case Domain::DocumentObjectType::ScreenplayText: {
        BusinessLayer::ScreenplayExportOptions options;
        options.filePath = _filePath;
        options.fileFormat = BusinessLayer::ExportFileFormat::Fountain;
        options.includeFolders = true;
        options.includeInlineNotes = true;
        options.includeReviewMarks = true;
        options.includeTitlePage = true;
        options.includeText = true;
        d->exportScreenplay(_model, options);
        break;
    }

    default:
        break;
    }
}

} // namespace ManagementLayer
