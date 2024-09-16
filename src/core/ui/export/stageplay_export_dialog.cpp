#include "stageplay_export_dialog.h"

#include <QSettings>


namespace Ui {

namespace {

const QString kFormatKey = "format";

static const QVector<BusinessLayer::ExportFileFormat> kStageplayExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
    BusinessLayer::ExportFileFormat::Fountain,
});

} // namespace

class StageplayExportDialog::Implementation
{
public:
    explicit Implementation() = default;

    BusinessLayer::StageplayExportOptions options;
};


// ****


StageplayExportDialog::StageplayExportDialog(const QString& _uuidKey, QWidget* _parent)
    : ScriptExportDialog(kStageplayExportFormats, _uuidKey, _parent)
    , d(new Implementation())
{
    QSettings settings;
    setCurrentFileFormat(settings.value(settingsKey(kFormatKey), 0).toInt());
    updateDialog();
}

StageplayExportDialog::~StageplayExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kFormatKey), currentFileFormatRow());
}

BusinessLayer::StageplayExportOptions& StageplayExportDialog::exportOptions() const
{
    d->options = ScriptExportDialog::exportOptions();
    return d->options;
}

void StageplayExportDialog::updateTranslations()
{
    ScriptExportDialog::updateTranslations();
    setTitle(tr("Export stageplay"));
}

} // namespace Ui
