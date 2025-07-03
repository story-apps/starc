#include "worlds_export_dialog.h"

#include <QSettings>


namespace Ui {

namespace {

const QLatin1String kWorldsKey("worlds");
const QLatin1String kFormatKey("format");

static const QVector<BusinessLayer::ExportFileFormat> kWorldsExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
});

} // namespace


// ****


WorldsExportDialog::WorldsExportDialog(const QString& _uuidKey, QWidget* _parent)
    : DocumentsExportDialog(kWorldsExportFormats, _uuidKey, _parent)
{
    hideStoryRole();

    QSettings settings;
    setCurrentFileFormat(settings.value(settingsKey(kFormatKey), 0).toInt());

    updateDialog();
}

WorldsExportDialog::~WorldsExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kWorldsKey), checkedDocuments());
    settings.setValue(settingsKey(kFormatKey), currentFileFormatRow());
}

BusinessLayer::DocumentsExportOptions& WorldsExportDialog::exportOptions() const
{
    return DocumentsExportDialog::exportOptions();
}

void WorldsExportDialog::updateTranslations()
{
    DocumentsExportDialog::updateTranslations();

    setTitle(tr("Export worlds"));
}

QString WorldsExportDialog::documentsKey() const
{
    return kWorldsKey;
}

} // namespace Ui
