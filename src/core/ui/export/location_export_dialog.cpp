#include "location_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QSettings>
#include <QVBoxLayout>


namespace Ui {

namespace {

const QString kFormatKey = "format";
const QString kIncludeSenseInfoKey = "include-sense-info";
const QString kIncludeGeographyInfoKey = "include-geography-info";
const QString kIncludeBackgroundInfoKey = "include-background-info";

static const QVector<BusinessLayer::ExportFileFormat> kLocationExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
});

} // namespace

class LocationExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* includeSenseInfo = nullptr;
    CheckBox* includeGeographyInfo = nullptr;
    CheckBox* includeBackgroundInfo = nullptr;

    BusinessLayer::LocationExportOptions options;
};

LocationExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeSenseInfo(new CheckBox(_parent))
    , includeGeographyInfo(new CheckBox(_parent))
    , includeBackgroundInfo(new CheckBox(_parent))
{
}


// ****


LocationExportDialog::LocationExportDialog(const QString& _uuidKey, QWidget* _parent)
    : DocumentExportDialog(kLocationExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    int row = 2;
    rightLayout()->insertWidget(row++, d->includeSenseInfo);
    rightLayout()->insertWidget(row++, d->includeGeographyInfo);
    rightLayout()->insertWidget(row++, d->includeBackgroundInfo);

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(rightLayout(), row++, column);
    contentsLayout()->addLayout(bottomLayout(), row, column);

    QSettings settings;
    setCurrentFileFormat(settings.value(uniqueKey(kFormatKey), 0).toInt());
    d->includeSenseInfo->setChecked(settings.value(uniqueKey(kIncludeSenseInfoKey), true).toBool());
    d->includeGeographyInfo->setChecked(
        settings.value(uniqueKey(kIncludeGeographyInfoKey), false).toBool());
    d->includeBackgroundInfo->setChecked(
        settings.value(uniqueKey(kIncludeBackgroundInfoKey), false).toBool());

    updateDialog();
}

LocationExportDialog::~LocationExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kFormatKey), currentFileFormatRow());
    settings.setValue(uniqueKey(kIncludeSenseInfoKey), d->includeSenseInfo->isChecked());
    settings.setValue(uniqueKey(kIncludeGeographyInfoKey), d->includeGeographyInfo->isChecked());
    settings.setValue(uniqueKey(kIncludeBackgroundInfoKey), d->includeBackgroundInfo->isChecked());
}

BusinessLayer::LocationExportOptions& LocationExportDialog::exportOptions() const
{
    d->options = DocumentExportDialog::exportOptions();

    d->options.includeSenseInfo
        = d->includeSenseInfo->isVisibleTo(this) && d->includeSenseInfo->isChecked();
    d->options.includeGeographyInfo
        = d->includeGeographyInfo->isVisibleTo(this) && d->includeGeographyInfo->isChecked();
    d->options.includeBackgroundInfo
        = d->includeBackgroundInfo->isVisibleTo(this) && d->includeBackgroundInfo->isChecked();
    return d->options;
}

void LocationExportDialog::updateTranslations()
{
    DocumentExportDialog::updateTranslations();

    setTitle(tr("Export location"));

    d->includeSenseInfo->setText(tr("Include sense info"));
    d->includeGeographyInfo->setText(tr("Include geography info"));
    d->includeBackgroundInfo->setText(tr("Include background info"));
}

void LocationExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    DocumentExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeSenseInfo,
             d->includeGeographyInfo,
             d->includeBackgroundInfo,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

} // namespace Ui
