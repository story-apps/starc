#include "comic_book_export_dialog.h"

#include <QSettings>


namespace Ui {

namespace {

const QString kFormatKey = "format";

static const QVector<BusinessLayer::ExportFileFormat> kComicBookExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
    BusinessLayer::ExportFileFormat::Fountain,
});

} // namespace


class ComicBookExportDialog::Implementation
{
public:
    explicit Implementation() = default;

    BusinessLayer::ComicBookExportOptions options;
};


// ****


ComicBookExportDialog::ComicBookExportDialog(const QString& _uuidKey, QWidget* _parent)
    : ScriptExportDialog(kComicBookExportFormats, _uuidKey, _parent)
    , d(new Implementation())
{
    QSettings settings;
    setCurrentFileFormat(settings.value(uniqueKey(kFormatKey), 0).toInt());
    updateDialog();
}

ComicBookExportDialog::~ComicBookExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kFormatKey), currentFileFormatRow());
}

BusinessLayer::ComicBookExportOptions& ComicBookExportDialog::exportOptions() const
{
    d->options = ScriptExportDialog::exportOptions();
    return d->options;
}

void ComicBookExportDialog::updateTranslations()
{
    ScriptExportDialog::updateTranslations();
    setTitle(tr("Export comic book"));
}

} // namespace Ui
