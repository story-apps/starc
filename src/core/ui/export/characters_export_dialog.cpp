#include "characters_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QSettings>
#include <QVBoxLayout>


namespace Ui {

namespace {
const QString kCharactersKey = "characters";
const QString kFormatKey = "format";
const QString kIncludeAgeKey = "include-age";
const QString kIncludeGenderKey = "include-gender";

static const QVector<BusinessLayer::ExportFileFormat> kCharactersExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
});

} // namespace

class CharactersExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* includeAge = nullptr;
    CheckBox* includeGender = nullptr;

    BusinessLayer::CharactersExportOptions options;
};

CharactersExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeAge(new CheckBox(_parent))
    , includeGender(new CheckBox(_parent))
{
}


// ****


CharactersExportDialog::CharactersExportDialog(const QString& _uuidKey, QWidget* _parent)
    : DocumentsExportDialog(kCharactersExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    int row = 3;
    rightLayout()->insertWidget(row++, d->includeAge);
    rightLayout()->insertWidget(row++, d->includeGender);

    QSettings settings;
    setCurrentFileFormat(settings.value(settingsKey(kFormatKey), 0).toInt());
    d->includeAge->setChecked(settings.value(settingsKey(kIncludeAgeKey), true).toBool());
    d->includeGender->setChecked(settings.value(settingsKey(kIncludeGenderKey), true).toBool());

    updateDialog();
}

CharactersExportDialog::~CharactersExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kCharactersKey), checkedDocuments());
    settings.setValue(settingsKey(kFormatKey), currentFileFormatRow());
    settings.setValue(settingsKey(kIncludeAgeKey), d->includeAge->isChecked());
    settings.setValue(settingsKey(kIncludeGenderKey), d->includeGender->isChecked());
}

BusinessLayer::CharactersExportOptions& CharactersExportDialog::exportOptions() const
{
    d->options = DocumentsExportDialog::exportOptions();

    d->options.includeAge = d->includeAge->isVisibleTo(this) && d->includeAge->isChecked();
    d->options.includeGender = d->includeGender->isVisibleTo(this) && d->includeGender->isChecked();
    return d->options;
}

void CharactersExportDialog::updateTranslations()
{
    DocumentsExportDialog::updateTranslations();

    setTitle(tr("Export characters"));

    d->includeAge->setText(tr("Include age"));
    d->includeGender->setText(tr("Include gender"));
}

void CharactersExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    DocumentsExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeAge,
             d->includeGender,
         }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }
}

QString CharactersExportDialog::documentsKey() const
{
    return kCharactersKey;
}

} // namespace Ui
