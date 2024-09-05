#include "script_export_dialog.h"

#include <business_layer/export/export_options.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QEvent>
#include <QGridLayout>
#include <QSettings>


namespace Ui {
namespace {
const QString kIncludeTitlePageKey = "include-title-page";
const QString kIncludeSynopsisKey = "include-synopsis";
const QString kIncludeTextKey = "include-text";
const QString kIncludeInlineNotesKey = "include-inline-notes";
const QString kIncludeReviewMarksKey = "include-review-marks";
} // namespace

class ScriptExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    CheckBox* includeTitlePage = nullptr;
    CheckBox* includeSynopsis = nullptr;
    CheckBox* includeText = nullptr;

    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;

    BusinessLayer::ExportOptions options;
};

ScriptExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeTitlePage(new CheckBox(_parent))
    , includeSynopsis(new CheckBox(_parent))
    , includeText(new CheckBox(_parent))
    , includeInlineNotes(new CheckBox(_parent))
    , includeReviewMarks(new CheckBox(_parent))
{
}


// ****


ScriptExportDialog::ScriptExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                       const QString& _uuidKey, QWidget* _parent)
    : AbstractExportDialog(_formats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    leftLayout()->addWidget(d->includeTitlePage);
    leftLayout()->addWidget(d->includeSynopsis);
    leftLayout()->addWidget(d->includeText);
    leftLayout()->addStretch();

    int row = 1;

    rightLayout()->insertWidget(row++, d->includeInlineNotes);
    rightLayout()->insertWidget(row++, d->includeReviewMarks);

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(leftLayout(), row, column++);
    contentsLayout()->addLayout(rightLayout(), row++, column);

    column = 0;
    contentsLayout()->addLayout(bottomLayout(), row, column, 1, 2);
    contentsLayout()->setColumnStretch(1, 1);

    QSettings settings;
    d->includeTitlePage->setChecked(settings.value(uniqueKey(kIncludeTitlePageKey), true).toBool());
    d->includeSynopsis->setChecked(settings.value(uniqueKey(kIncludeSynopsisKey), true).toBool());
    d->includeText->setChecked(settings.value(uniqueKey(kIncludeTextKey), true).toBool());
    d->includeInlineNotes->setChecked(
        settings.value(uniqueKey(kIncludeInlineNotesKey), false).toBool());
    d->includeReviewMarks->setChecked(
        settings.value(uniqueKey(kIncludeReviewMarksKey), true).toBool());

    connect(d->includeText, &CheckBox::checkedChanged, this,
            &ScriptExportDialog::processIncludeTextChanged);
    //
    connect(d->includeTitlePage, &CheckBox::checkedChanged, this,
            &ScriptExportDialog::updateDialog);
    connect(d->includeSynopsis, &CheckBox::checkedChanged, this, &ScriptExportDialog::updateDialog);
    connect(d->includeText, &CheckBox::checkedChanged, this, &ScriptExportDialog::updateDialog);

    updateDialog();
}

ScriptExportDialog::~ScriptExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kIncludeTitlePageKey), d->includeTitlePage->isChecked());
    settings.setValue(uniqueKey(kIncludeSynopsisKey), d->includeSynopsis->isChecked());
    settings.setValue(uniqueKey(kIncludeTextKey), d->includeText->isChecked());
    settings.setValue(uniqueKey(kIncludeInlineNotesKey), d->includeInlineNotes->isChecked());
    settings.setValue(uniqueKey(kIncludeReviewMarksKey), d->includeReviewMarks->isChecked());
}

BusinessLayer::ExportOptions& ScriptExportDialog::exportOptions() const
{
    d->options = AbstractExportDialog::exportOptions();
    d->options.includeTitlePage
        = d->includeTitlePage->isVisibleTo(this) && d->includeTitlePage->isChecked();
    d->options.includeSynopsis
        = d->includeSynopsis->isVisibleTo(this) && d->includeSynopsis->isChecked();
    d->options.includeText = shouldIncludeText();
    d->options.includeInlineNotes
        = d->includeInlineNotes->isVisibleTo(this) && d->includeInlineNotes->isChecked();
    d->options.includeReviewMarks
        = d->includeReviewMarks->isVisibleTo(this) && d->includeReviewMarks->isChecked();
    return d->options;
}

void ScriptExportDialog::updateTranslations()
{
    AbstractExportDialog::updateTranslations();

    d->includeTitlePage->setText(tr("Title page"));
    d->includeSynopsis->setText(tr("Synopsis"));
    d->includeText->setText(tr("Text"));

    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks"));
}

void ScriptExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeTitlePage,
             d->includeSynopsis,
             d->includeText,
             d->includeInlineNotes,
             d->includeReviewMarks,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void ScriptExportDialog::updateParametersVisibility() const
{
    auto isPrintSynopsisVisible = true;
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
    case BusinessLayer::ExportFileFormat::Fdx: {
        isPrintSynopsisVisible = false;
        isPrintReviewMarksVisible = false;
        isWatermarkVisible = false;
        break;
    }
    case BusinessLayer::ExportFileFormat::Fountain: {
        isPrintSynopsisVisible = false;
        isWatermarkVisible = false;
        break;
    }
    case BusinessLayer::ExportFileFormat::Markdown: {
        isPrintSynopsisVisible = false;
        isPrintInlineNotesVisible = false;
        isPrintReviewMarksVisible = false;
        isWatermarkVisible = false;
        break;
    }
    }

    d->includeSynopsis->setVisible(isPrintSynopsisVisible);
    if (!isRightLayoutVisible()) {
        isPrintInlineNotesVisible = false;
        isPrintReviewMarksVisible = false;
    }

    d->includeInlineNotes->setVisible(isPrintInlineNotesVisible);
    d->includeReviewMarks->setVisible(isPrintReviewMarksVisible);
    setWatermarkVisible(isWatermarkVisible);
}

bool ScriptExportDialog::isExportEnabled() const
{
    return (d->includeTitlePage->isVisibleTo(this) && d->includeTitlePage->isChecked())
        || (d->includeSynopsis->isVisibleTo(this) && d->includeSynopsis->isChecked())
        || (d->includeText->isVisibleTo(this) && d->includeText->isChecked());
}

bool ScriptExportDialog::isRightLayoutVisible() const
{
    return d->includeText->isVisibleTo(this) && d->includeText->isChecked();
}

void ScriptExportDialog::setIncludeScript(bool _checked) const
{
    d->includeText->setChecked(_checked);
}

bool ScriptExportDialog::shouldIncludeText() const
{
    return d->includeText->isVisibleTo(this) && d->includeText->isChecked();
}

void ScriptExportDialog::processIncludeTextChanged(bool _checked) const
{
    Q_UNUSED(_checked)
}

QWidget* ScriptExportDialog::focusedWidgetAfterShow() const
{
    return d->includeTitlePage;
}

} // namespace Ui
