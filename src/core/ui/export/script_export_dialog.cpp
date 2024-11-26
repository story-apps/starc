#include "script_export_dialog.h"

#include <business_layer/export/export_options.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>

#include <QEvent>
#include <QGridLayout>
#include <QSettings>
#include <QStringListModel>


namespace Ui {
namespace {
const QLatin1String kIncludeTitlePageKey("include-title-page");
const QLatin1String kIncludeSynopsisKey("include-synopsis");
const QLatin1String kIncludeTextKey("include-text");
const QLatin1String kVersionKey("version");
const QLatin1String kIncludeInlineNotesKey("include-inline-notes");
const QLatin1String kIncludeReviewMarksKey("include-review-marks");
} // namespace

class ScriptExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    CheckBox* includeTitlePage = nullptr;
    CheckBox* includeSynopsis = nullptr;
    CheckBox* includeText = nullptr;

    ComboBox* version = nullptr;
    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;

    BusinessLayer::ExportOptions options;
};

ScriptExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeTitlePage(new CheckBox(_parent))
    , includeSynopsis(new CheckBox(_parent))
    , includeText(new CheckBox(_parent))
    , version(new ComboBox(_parent))
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
    rightLayout()->insertWidget(row++, d->version);
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
    d->includeTitlePage->setChecked(
        settings.value(settingsKey(kIncludeTitlePageKey), true).toBool());
    d->includeSynopsis->setChecked(settings.value(settingsKey(kIncludeSynopsisKey), true).toBool());
    d->includeText->setChecked(settings.value(settingsKey(kIncludeTextKey), true).toBool());
    d->includeInlineNotes->setChecked(
        settings.value(settingsKey(kIncludeInlineNotesKey), false).toBool());
    d->includeReviewMarks->setChecked(
        settings.value(settingsKey(kIncludeReviewMarksKey), true).toBool());

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
    settings.setValue(settingsKey(kIncludeTitlePageKey), d->includeTitlePage->isChecked());
    settings.setValue(settingsKey(kIncludeSynopsisKey), d->includeSynopsis->isChecked());
    settings.setValue(settingsKey(kIncludeTextKey), d->includeText->isChecked());
    settings.setValue(settingsKey(kVersionKey), d->version->currentIndex().row());
    settings.setValue(settingsKey(kIncludeInlineNotesKey), d->includeInlineNotes->isChecked());
    settings.setValue(settingsKey(kIncludeReviewMarksKey), d->includeReviewMarks->isChecked());
}

void ScriptExportDialog::setVersions(const QVector<QString>& _versions, int _currentVersionIndex)
{
    QStringListModel* model = nullptr;
    const bool isInitialSetup = d->version->model() == nullptr;
    if (isInitialSetup) {
        model = new QStringListModel(d->version);
        d->version->setModel(model);
    } else {
        model = qobject_cast<QStringListModel*>(d->version->model());
    }

    const int invalidIndex = -1;
    int versionRow = 0;
    if (!isInitialSetup) {
        versionRow = selectedVersion();
    }
    model->setStringList({ _versions.begin(), _versions.end() });
    if (_currentVersionIndex != invalidIndex) {
        //
        // Если задан индекс напрямую, то используем его
        //
        versionRow = _currentVersionIndex;
    } else if (isInitialSetup) {
        //
        // Когда версии были заданы в первый раз для диалога восстановим прошлое значение выбранной
        // версии
        //
        versionRow = QSettings().value(settingsKey(kVersionKey), 0).toInt();
    }
    const auto versionIndex = d->version->model()->index(versionRow, 0);
    d->version->setCurrentIndex(versionIndex);
    d->version->setVisible(_versions.size() > 1);
}

int ScriptExportDialog::selectedVersion() const
{
    return d->version->currentIndex().row();
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

void ScriptExportDialog::updateTranslations()
{
    AbstractExportDialog::updateTranslations();

    d->includeTitlePage->setText(tr("Title page"));
    d->includeSynopsis->setText(tr("Synopsis"));
    d->includeText->setText(tr("Text"));

    d->version->setLabel(tr("Draft"));
    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks / revisions"));
}

void ScriptExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractExportDialog::designSystemChangeEvent(_event);

    for (auto combobox : {
             d->version,
         }) {
        combobox->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        combobox->setTextColor(Ui::DesignSystem::color().onBackground());
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }

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

} // namespace Ui
