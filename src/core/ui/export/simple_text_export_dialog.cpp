#include "simple_text_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QGridLayout>
#include <QSettings>


namespace Ui {

namespace {

const QString kFormatKey = "format";
const QString kIncludeInlineNotesKey = "include-inline-notes";
const QString kIncludeReviewMarksKey = "include-review-marks";

static const QVector<BusinessLayer::ExportFileFormat> kSimpleTextExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
    BusinessLayer::ExportFileFormat::Markdown,
});

} // namespace

class SimpleTextExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;

    BusinessLayer::ExportOptions options;
};

SimpleTextExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeInlineNotes(new CheckBox(_parent))
    , includeReviewMarks(new CheckBox(_parent))
{
}


// ****


SimpleTextExportDialog::SimpleTextExportDialog(const QString& _uuidKey, QWidget* _parent)
    : AbstractExportDialog(kSimpleTextExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    int row = 1;
    rightLayout()->insertWidget(row++, d->includeInlineNotes);
    rightLayout()->insertWidget(row++, d->includeReviewMarks);

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(rightLayout(), row++, column, 1, 2);
    contentsLayout()->addLayout(bottomLayout(), row, column, 1, 2);

    QSettings settings;
    setCurrentFileFormat(settings.value(kFormatKey, 0).toInt());
    updateDialog();
    d->includeInlineNotes->setChecked(
        settings.value(settingsKey(kIncludeInlineNotesKey), false).toBool());
    d->includeReviewMarks->setChecked(
        settings.value(settingsKey(kIncludeReviewMarksKey), true).toBool());
}

SimpleTextExportDialog::~SimpleTextExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kFormatKey), currentFileFormatRow());
    settings.setValue(settingsKey(kIncludeInlineNotesKey), d->includeInlineNotes->isChecked());
    settings.setValue(settingsKey(kIncludeReviewMarksKey), d->includeReviewMarks->isChecked());
}

BusinessLayer::ExportOptions& SimpleTextExportDialog::exportOptions() const
{
    d->options = AbstractExportDialog::exportOptions();
    d->options.includeInlineNotes
        = d->includeInlineNotes->isVisibleTo(this) && d->includeInlineNotes->isChecked();
    d->options.includeReviewMarks
        = d->includeReviewMarks->isVisibleTo(this) && d->includeReviewMarks->isChecked();
    return d->options;
}

void SimpleTextExportDialog::updateTranslations()
{
    AbstractExportDialog::updateTranslations();
    setTitle(tr("Export text document"));
    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks"));
}

void SimpleTextExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeInlineNotes,
             d->includeReviewMarks,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void SimpleTextExportDialog::updateParametersVisibility() const
{
    auto isPrintInlineNotesVisible = true;
    auto isPrintReviewMarksVisible = true;
    auto isWatermarkVisible = true;
    switch (currentFileFormat()) {
    default:
    case BusinessLayer::ExportFileFormat::Pdf: {
        //
        // ... всё видимое
        //
        break;
    }
    case BusinessLayer::ExportFileFormat::Docx: {
        isWatermarkVisible = false;
        break;
    }
    case BusinessLayer::ExportFileFormat::Markdown: {
        isPrintInlineNotesVisible = false;
        isPrintReviewMarksVisible = false;
        isWatermarkVisible = false;
        break;
    }
    }
    d->includeInlineNotes->setVisible(isPrintInlineNotesVisible);
    d->includeReviewMarks->setVisible(isPrintReviewMarksVisible);
    setWatermarkVisible(isWatermarkVisible);
}

} // namespace Ui
