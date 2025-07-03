#include "world_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QSettings>
#include <QVBoxLayout>


namespace Ui {

namespace {

const QLatin1String kFormatKey("format");
const QLatin1String kIncludeWorldDescriptionInfoKey("include-world-description-info");
const QLatin1String kIncludeNatureInfoKey("include-nature-info");
const QLatin1String kIncludeCultureInfoKey("include-culture-info");
const QLatin1String kIncludeSystemInfoKey("include-system-info");
const QLatin1String kIncludePoliticsInfoKey("include-politics-info");
const QLatin1String kIncludeMagicInfoKey("include-magic-info");

static const QVector<BusinessLayer::ExportFileFormat> kWorldExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
});

} // namespace

class WorldExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* includeWorldDescriptionInfo = nullptr;
    CheckBox* includeNatureInfo = nullptr;
    CheckBox* includeCultureInfo = nullptr;
    CheckBox* includeSystemInfo = nullptr;
    CheckBox* includePoliticsInfo = nullptr;
    CheckBox* includeMagicInfo = nullptr;

    BusinessLayer::WorldExportOptions options;
};

WorldExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeWorldDescriptionInfo(new CheckBox(_parent))
    , includeNatureInfo(new CheckBox(_parent))
    , includeCultureInfo(new CheckBox(_parent))
    , includeSystemInfo(new CheckBox(_parent))
    , includePoliticsInfo(new CheckBox(_parent))
    , includeMagicInfo(new CheckBox(_parent))
{
}


// ****


WorldExportDialog::WorldExportDialog(const QString& _uuidKey, QWidget* _parent)
    : DocumentExportDialog(kWorldExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    int row = 2;
    rightLayout()->insertWidget(row++, d->includeWorldDescriptionInfo);
    rightLayout()->insertWidget(row++, d->includeNatureInfo);
    rightLayout()->insertWidget(row++, d->includeCultureInfo);
    rightLayout()->insertWidget(row++, d->includeSystemInfo);
    rightLayout()->insertWidget(row++, d->includePoliticsInfo);
    rightLayout()->insertWidget(row++, d->includeMagicInfo);

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(rightLayout(), row++, column);
    contentsLayout()->addLayout(bottomLayout(), row, column);

    QSettings settings;
    setCurrentFileFormat(settings.value(settingsKey(kFormatKey), 0).toInt());
    d->includeWorldDescriptionInfo->setChecked(
        settings.value(settingsKey(kIncludeWorldDescriptionInfoKey), true).toBool());
    d->includeNatureInfo->setChecked(
        settings.value(settingsKey(kIncludeNatureInfoKey), true).toBool());
    d->includeCultureInfo->setChecked(
        settings.value(settingsKey(kIncludeCultureInfoKey), true).toBool());
    d->includeSystemInfo->setChecked(
        settings.value(settingsKey(kIncludeSystemInfoKey), true).toBool());
    d->includePoliticsInfo->setChecked(
        settings.value(settingsKey(kIncludePoliticsInfoKey), true).toBool());
    d->includeMagicInfo->setChecked(
        settings.value(settingsKey(kIncludeMagicInfoKey), true).toBool());

    updateDialog();
}

WorldExportDialog::~WorldExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kFormatKey), currentFileFormatRow());
    settings.setValue(settingsKey(kIncludeWorldDescriptionInfoKey),
                      d->includeWorldDescriptionInfo->isChecked());
    settings.setValue(settingsKey(kIncludeNatureInfoKey), d->includeNatureInfo->isChecked());
    settings.setValue(settingsKey(kIncludeCultureInfoKey), d->includeCultureInfo->isChecked());
    settings.setValue(settingsKey(kIncludeSystemInfoKey), d->includeSystemInfo->isChecked());
    settings.setValue(settingsKey(kIncludePoliticsInfoKey), d->includePoliticsInfo->isChecked());
    settings.setValue(settingsKey(kIncludeMagicInfoKey), d->includeMagicInfo->isChecked());
}

BusinessLayer::WorldExportOptions& WorldExportDialog::exportOptions() const
{
    d->options = DocumentExportDialog::exportOptions();

    d->options.includeWorldDescriptionInfo = d->includeWorldDescriptionInfo->isVisibleTo(this)
        && d->includeWorldDescriptionInfo->isChecked();
    d->options.includeNatureInfo
        = d->includeNatureInfo->isVisibleTo(this) && d->includeNatureInfo->isChecked();
    d->options.includeCultureInfo
        = d->includeCultureInfo->isVisibleTo(this) && d->includeCultureInfo->isChecked();
    d->options.includeSystemInfo
        = d->includeSystemInfo->isVisibleTo(this) && d->includeSystemInfo->isChecked();
    d->options.includePoliticsInfo
        = d->includePoliticsInfo->isVisibleTo(this) && d->includePoliticsInfo->isChecked();
    d->options.includeMagicInfo
        = d->includeMagicInfo->isVisibleTo(this) && d->includeMagicInfo->isChecked();
    return d->options;
}

void WorldExportDialog::updateTranslations()
{
    DocumentExportDialog::updateTranslations();

    setTitle(tr("Export World"));

    d->includeWorldDescriptionInfo->setText(tr("Include world description info"));
    d->includeNatureInfo->setText(tr("Include nature info"));
    d->includeCultureInfo->setText(tr("Include culture info"));
    d->includeSystemInfo->setText(tr("Include system info"));
    d->includePoliticsInfo->setText(tr("Include politics info"));
    d->includeMagicInfo->setText(tr("Include magic info"));
}

void WorldExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    DocumentExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeWorldDescriptionInfo,
             d->includeNatureInfo,
             d->includeCultureInfo,
             d->includeSystemInfo,
             d->includePoliticsInfo,
             d->includeMagicInfo,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

} // namespace Ui
