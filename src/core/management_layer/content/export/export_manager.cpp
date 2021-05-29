#include "export_manager.h"

#include <business_layer/export/screenplay/export_options.h>
#include <business_layer/export/screenplay/pdf_exporter.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>

#include <ui/export/export_dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>

#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>

#include <QDesktopServices>
#include <QFileDialog>

namespace ManagementLayer
{

class ExportManager::Implementation
{
public:
    explicit Implementation(ExportManager* _parent, QWidget* _topLevelWidget);

    /**
     * @brief Экспортировать сценарий
     */
    void exportScreenplay(BusinessLayer::AbstractModel* _model);

    //
    // Данные
    //

    ExportManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ExportDialog* exportDialog = nullptr;
};

ExportManager::Implementation::Implementation(ExportManager* _parent, QWidget* _topLevelWidget)
    : q(_parent),
      topLevelWidget(_topLevelWidget)
{

}

void ExportManager::Implementation::exportScreenplay(BusinessLayer::AbstractModel* _model)
{
    if (exportDialog == nullptr) {
        exportDialog = new Ui::ExportDialog(topLevelWidget);
        connect(exportDialog, &Ui::ExportDialog::exportRequested, exportDialog, [this, _model] {
            auto exportOptions = exportDialog->exportOptions();

            //
            // Предоставим пользователю возможность выбрать файл, куда он будет экспортировать
            //
            const auto projectExportFolder
                    = DataStorageLayer::StorageFacade::settingsStorage()->value(
                          DataStorageLayer::kProjectExportFolderKey,
                          DataStorageLayer::SettingsStorage::SettingsPlace::Application)
                      .toString();
            QString exportFilter;
            QString exportExtension;
            switch (exportOptions.fileFormat) {
                default:
                case 0: {
                    exportFilter = DialogHelper::pdfFilter();
                    exportExtension = ExtensionHelper::pdf();
                    break;
                }
                case 1: {
                    exportFilter = DialogHelper::msWordFilter();
                    exportExtension = ExtensionHelper::msOfficeOpenXml();
                    break;
                }
                case 2: {
                    exportFilter = DialogHelper::finalDraftFilter();
                    exportExtension = ExtensionHelper::finalDraft();
                    break;
                }
                case 3: {
                    exportFilter = DialogHelper::fountainFilter();
                    exportExtension = ExtensionHelper::fountain();
                    break;
                }
            }
            auto exportFilePath
                    = QFileDialog::getSaveFileName(topLevelWidget, tr("Choose the file to export"),
                            projectExportFolder, exportFilter);
            if (exportFilePath.isEmpty()) {
                return;
            }
            if (!exportFilePath.endsWith(exportExtension, Qt::CaseInsensitive)) {
                exportFilePath += "." + exportExtension;
            }

            //
            // Если файл был выбран
            //
            exportOptions.filePath = exportFilePath;
            //
            // ... обновим папку, куда в следующий раз он предположительно опять будет экспортировать
            //
            DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                        DataStorageLayer::kProjectExportFolderKey,
                        exportFilePath,
                        DataStorageLayer::SettingsStorage::SettingsPlace::Application);
            //
            // ... и экспортируем документ
            //
            QScopedPointer<BusinessLayer::AbstractExporter> exporter;
            switch (exportOptions.fileFormat) {
                default:
                case 0: {
                    exporter.reset(new BusinessLayer::PdfExporter);
                    break;
                }
                case 1: {
                    break;
                }
                case 2: {
                    break;
                }
                case 3: {
                    break;
                }
            }
            if (exporter.isNull()) {
                return;
            }
            exporter->exportTo(qobject_cast<BusinessLayer::ScreenplayTextModel*>(_model),
                               exportOptions);

            //
            // Если необходимо, откроем экспортированный документ
            //
            if (exportDialog->openDocumentAfetrExport()) {
                QDesktopServices::openUrl(exportOptions.filePath);
            }
            //
            // ... и закрываем диалог экспорта
            //
            exportDialog->hideDialog();
        });
        connect(exportDialog, &Ui::ExportDialog::canceled, exportDialog, &Ui::ExportDialog::hideDialog);
        connect(exportDialog, &Ui::ExportDialog::disappeared, exportDialog, [this] {
            exportDialog->deleteLater();
            exportDialog = nullptr;
        });
    }

    exportDialog->showDialog();
}


// ****


ExportManager::ExportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(this, _parentWidget))
{

}

ExportManager::~ExportManager() = default;

bool ExportManager::canExportDocument(BusinessLayer::AbstractModel* _model) const
{
    if (_model == nullptr
        || _model->document() == nullptr) {
        return false;
    }

    switch (_model->document()->type())
    {
        case Domain::DocumentObjectType::ScreenplayText:
            return true;

        default:
            return false;
    }
}

void ExportManager::exportDocument(BusinessLayer::AbstractModel* _model)
{
    if (!canExportDocument(_model)) {
        return;
    }

    switch (_model->document()->type())
    {
        case Domain::DocumentObjectType::ScreenplayText: {
            d->exportScreenplay(_model);
            break;
        }

        default: break;
    }
}

} // namespace ManagementLayer
