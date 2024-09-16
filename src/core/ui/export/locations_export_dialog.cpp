#include "locations_export_dialog.h"

#include <QSettings>


namespace Ui {

namespace {

const QString kLocationsKey = "locations";
const QString kFormatKey = "format";

static const QVector<BusinessLayer::ExportFileFormat> kLocationsExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
});

} // namespace


// ****


LocationsExportDialog::LocationsExportDialog(const QString& _uuidKey, QWidget* _parent)
    : DocumentsExportDialog(kLocationsExportFormats, _uuidKey, _parent)
{
    QSettings settings;
    setCurrentFileFormat(settings.value(settingsKey(kFormatKey), 0).toInt());

    updateDialog();
}

LocationsExportDialog::~LocationsExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kLocationsKey), checkedDocuments());
    settings.setValue(settingsKey(kFormatKey), currentFileFormatRow());
}

BusinessLayer::DocumentsExportOptions& LocationsExportDialog::exportOptions() const
{
    return DocumentsExportDialog::exportOptions();
}

void LocationsExportDialog::updateTranslations()
{
    DocumentsExportDialog::updateTranslations();

    setTitle(tr("Export locations"));
}

QString LocationsExportDialog::documentsKey() const
{
    return kLocationsKey;
}

} // namespace Ui
