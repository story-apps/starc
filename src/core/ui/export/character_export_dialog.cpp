#include "character_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QSettings>
#include <QVBoxLayout>


namespace Ui {

namespace {

const QString kFormatKey = "format";
const QString kIncludeStoryInfoKey = "include-story-info";
const QString kIncludePersonalInfoKey = "include-personal-info";
const QString kIncludePhysiqueInfoKey = "include-physique-info";
const QString kIncludeLifeInfoKey = "include-life-info";
const QString kIncludeAttitudeInfoKey = "include-attitude-info";
const QString kIncludeBiographyInfoKey = "include-biography-info";

static const QVector<BusinessLayer::ExportFileFormat> kCharacterExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
});

} // namespace

class CharacterExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* includeStoryInfo = nullptr;
    CheckBox* includePersonalInfo = nullptr;
    CheckBox* includePhysiqueInfo = nullptr;
    CheckBox* includeLifeInfo = nullptr;
    CheckBox* includeAttitudeInfo = nullptr;
    CheckBox* includeBiographyInfo = nullptr;

    BusinessLayer::CharacterExportOptions options;
};

CharacterExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeStoryInfo(new CheckBox(_parent))
    , includePersonalInfo(new CheckBox(_parent))
    , includePhysiqueInfo(new CheckBox(_parent))
    , includeLifeInfo(new CheckBox(_parent))
    , includeAttitudeInfo(new CheckBox(_parent))
    , includeBiographyInfo(new CheckBox(_parent))
{
}


// ****


CharacterExportDialog::CharacterExportDialog(const QString& _uuidKey, QWidget* _parent)
    : DocumentExportDialog(kCharacterExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    int row = 2;
    rightLayout()->insertWidget(row++, d->includeStoryInfo);
    rightLayout()->insertWidget(row++, d->includePersonalInfo);
    rightLayout()->insertWidget(row++, d->includePhysiqueInfo);
    rightLayout()->insertWidget(row++, d->includeLifeInfo);
    rightLayout()->insertWidget(row++, d->includeAttitudeInfo);
    rightLayout()->insertWidget(row++, d->includeBiographyInfo);

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(rightLayout(), row++, column);
    contentsLayout()->addLayout(bottomLayout(), row, column);

    QSettings settings;
    setCurrentFileFormat(settings.value(kFormatKey, 0).toInt());
    d->includeStoryInfo->setChecked(settings.value(uniqueKey(kIncludeStoryInfoKey), true).toBool());
    d->includePersonalInfo->setChecked(
        settings.value(uniqueKey(kIncludePersonalInfoKey), false).toBool());
    d->includePhysiqueInfo->setChecked(
        settings.value(uniqueKey(kIncludePhysiqueInfoKey), false).toBool());
    d->includeLifeInfo->setChecked(settings.value(uniqueKey(kIncludeLifeInfoKey), false).toBool());
    d->includeAttitudeInfo->setChecked(
        settings.value(uniqueKey(kIncludeAttitudeInfoKey), false).toBool());
    d->includeBiographyInfo->setChecked(
        settings.value(uniqueKey(kIncludeBiographyInfoKey), false).toBool());

    updateDialog();
}

CharacterExportDialog::~CharacterExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kFormatKey), currentFileFormatRow());
    settings.setValue(uniqueKey(kIncludeStoryInfoKey), d->includeStoryInfo->isChecked());
    settings.setValue(uniqueKey(kIncludePersonalInfoKey), d->includePersonalInfo->isChecked());
    settings.setValue(uniqueKey(kIncludePhysiqueInfoKey), d->includePhysiqueInfo->isChecked());
    settings.setValue(uniqueKey(kIncludeLifeInfoKey), d->includeLifeInfo->isChecked());
    settings.setValue(uniqueKey(kIncludeAttitudeInfoKey), d->includeAttitudeInfo->isChecked());
    settings.setValue(uniqueKey(kIncludeBiographyInfoKey), d->includeBiographyInfo->isChecked());
}

BusinessLayer::CharacterExportOptions& CharacterExportDialog::exportOptions() const
{
    d->options = DocumentExportDialog::exportOptions();
    d->options.includeStoryInfo
        = d->includeStoryInfo->isVisibleTo(this) && d->includeStoryInfo->isChecked();
    d->options.includePersonalInfo
        = d->includePersonalInfo->isVisibleTo(this) && d->includePersonalInfo->isChecked();
    d->options.includePhysiqueInfo
        = d->includePhysiqueInfo->isVisibleTo(this) && d->includePhysiqueInfo->isChecked();
    d->options.includeLifeInfo
        = d->includeLifeInfo->isVisibleTo(this) && d->includeLifeInfo->isChecked();
    d->options.includeAttitudeInfo
        = d->includeAttitudeInfo->isVisibleTo(this) && d->includeAttitudeInfo->isChecked();
    d->options.includeBiographyInfo
        = d->includeBiographyInfo->isVisibleTo(this) && d->includeBiographyInfo->isChecked();
    return d->options;
}

void CharacterExportDialog::updateTranslations()
{
    DocumentExportDialog::updateTranslations();

    setTitle(tr("Export character"));

    d->includeStoryInfo->setText(tr("Include story info"));
    d->includePersonalInfo->setText(tr("Include personal info"));
    d->includePhysiqueInfo->setText(tr("Include physique info"));
    d->includeLifeInfo->setText(tr("Include life info"));
    d->includeAttitudeInfo->setText(tr("Include attitude info"));
    d->includeBiographyInfo->setText(tr("Include biography"));
}

void CharacterExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    DocumentExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeStoryInfo,
             d->includePersonalInfo,
             d->includePhysiqueInfo,
             d->includeLifeInfo,
             d->includeAttitudeInfo,
             d->includeBiographyInfo,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

} // namespace Ui
