#include "screenplay_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/text_field/text_field.h>

#include <QSettings>
#include <QVBoxLayout>

namespace Ui {

namespace {

const QString kFormatKey = "format";
const QString kIncludeTreatmentKey = "include-treatment";
const QString kIncludeSequencesKey = "include-sequences";
const QString kHighlightCharacterKey = "highlight-character";
const QString kHighlightCharacterWithDialogueKey = "highlight-character-with-dialogue";
const QString kScenesToPrintKey = "scenes-to-print";

static const QVector<BusinessLayer::ExportFileFormat> kScreenplayExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
    BusinessLayer::ExportFileFormat::Fdx,
    BusinessLayer::ExportFileFormat::Fountain,
});

} // namespace


class ScreenplayExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список сцен для печати
     */
    QVector<QString> scenesToPrint() const;


    CheckBox* includeTreatment = nullptr;

    CheckBox* includeSequences = nullptr;
    CheckBox* highlightCharacters = nullptr;
    CheckBox* highlightCharactersWithDialogue = nullptr;
    TextField* exportConcreteScenes = nullptr;

    BusinessLayer::ScreenplayExportOptions options;
};

ScreenplayExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeTreatment(new CheckBox(_parent))
    , includeSequences(new CheckBox(_parent))
    , highlightCharacters(new CheckBox(_parent))
    , highlightCharactersWithDialogue(new CheckBox(_parent))
    , exportConcreteScenes(new TextField(_parent))
{
    exportConcreteScenes->setSpellCheckPolicy(SpellCheckPolicy::Manual);
}

QVector<QString> ScreenplayExportDialog::Implementation::scenesToPrint() const
{
    QVector<QString> scenesToPrint;
    const auto scenesRanges = exportConcreteScenes->text().split(',', Qt::SkipEmptyParts);
    for (const auto& scenesRange : scenesRanges) {
        if (scenesRange.contains('-')) {
            const auto range = scenesRange.split('-', Qt::SkipEmptyParts);
            if (range.size() == 2) {
                int fromScene = range.constFirst().toInt();
                int toScene = range.constLast().toInt();
                if (fromScene > toScene) {
                    std::swap(fromScene, toScene);
                }
                for (int scene = fromScene; scene <= toScene; ++scene) {
                    scenesToPrint.append(QString::number(scene));
                }
            } else if (!range.isEmpty()) {
                scenesToPrint.append(range.constFirst());
            }
        } else {
            scenesToPrint.append(scenesRange.simplified());
        }
    }
    return scenesToPrint;
}


// ****


ScreenplayExportDialog::ScreenplayExportDialog(const QString& _uuidKey, QWidget* _parent)
    : ScriptExportDialog(kScreenplayExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    leftLayout()->insertWidget(2, d->includeTreatment);

    rightLayout()->insertWidget(1, d->includeSequences);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->highlightCharacters);
        layout->addWidget(d->highlightCharactersWithDialogue);
        layout->addStretch();
        rightLayout()->insertLayout(4, layout);
    }
    rightLayout()->insertWidget(5, d->exportConcreteScenes);

    QSettings settings;
    setCurrentFileFormat(settings.value(uniqueKey(kFormatKey), 0).toInt());
    d->includeTreatment->setChecked(
        settings.value(uniqueKey(kIncludeTreatmentKey), false).toBool());
    d->includeSequences->setChecked(settings.value(uniqueKey(kIncludeSequencesKey), true).toBool());
    d->highlightCharacters->setChecked(
        settings.value(uniqueKey(kHighlightCharacterKey), false).toBool());
    d->highlightCharactersWithDialogue->setChecked(
        settings.value(uniqueKey(kHighlightCharacterWithDialogueKey), false).toBool());
    d->exportConcreteScenes->setText(settings.value(uniqueKey(kScenesToPrintKey)).toString());

    connect(d->includeTreatment, &CheckBox::checkedChanged, this,
            &ScreenplayExportDialog::updateDialog);

    connect(d->includeTreatment, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            setIncludeScript(false);
        }
    });
    connect(d->highlightCharacters, &CheckBox::checkedChanged, d->highlightCharactersWithDialogue,
            &CheckBox::setEnabled);

    updateDialog();
}

ScreenplayExportDialog::~ScreenplayExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kFormatKey), currentFileFormatRow());
    settings.setValue(uniqueKey(kIncludeTreatmentKey), d->includeTreatment->isChecked());
    settings.setValue(uniqueKey(kIncludeSequencesKey), d->includeSequences->isChecked());
    settings.setValue(uniqueKey(kScenesToPrintKey), d->exportConcreteScenes->text());
    settings.setValue(uniqueKey(kHighlightCharacterKey), d->highlightCharacters->isChecked());
    settings.setValue(uniqueKey(kHighlightCharacterWithDialogueKey),
                      d->highlightCharactersWithDialogue->isChecked());
}

BusinessLayer::ScreenplayExportOptions& ScreenplayExportDialog::exportOptions() const
{
    d->options = ScriptExportDialog::exportOptions();

    d->options.includeTreatment
        = d->includeTreatment->isVisibleTo(this) && d->includeTreatment->isChecked();
    d->options.includeFolders
        = d->includeSequences->isVisibleTo(this) && d->includeSequences->isChecked();
    d->options.highlightCharacters
        = d->highlightCharacters->isVisibleTo(this) && d->highlightCharacters->isChecked();
    d->options.highlightCharactersWithDialogues
        = d->highlightCharactersWithDialogue->isVisibleTo(this)
        && d->highlightCharactersWithDialogue->isChecked();
    d->options.exportScenes = d->scenesToPrint();
    return d->options;
}

void ScreenplayExportDialog::updateTranslations()
{
    ScriptExportDialog::updateTranslations();

    setTitle(tr("Export screenplay"));
    d->includeTreatment->setText(tr("Treatment"));
    d->includeSequences->setText(tr("Include sequences headers and footers"));
    d->highlightCharacters->setText(tr("Highlight characters"));
    d->highlightCharactersWithDialogue->setText(tr("with dialogues"));
    d->exportConcreteScenes->setLabel(tr("Export concrete scenes"));
    d->exportConcreteScenes->setHelper(tr("Keep empty, if you want to print all scenes"));
}

void ScreenplayExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    ScriptExportDialog::designSystemChangeEvent(_event);

    d->exportConcreteScenes->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->exportConcreteScenes->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto checkBox : {
             d->includeTreatment,
             d->includeSequences,
             d->highlightCharacters,
             d->highlightCharactersWithDialogue,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void ScreenplayExportDialog::updateParametersVisibility() const
{
    ScriptExportDialog::updateParametersVisibility();
    d->includeSequences->setVisible(isRightLayoutVisible());
    d->highlightCharacters->setVisible(isRightLayoutVisible());
    d->highlightCharactersWithDialogue->setVisible(isRightLayoutVisible());
    d->highlightCharactersWithDialogue->setEnabled(d->highlightCharacters->isChecked());
    d->exportConcreteScenes->setVisible(isRightLayoutVisible());
}

bool ScreenplayExportDialog::isRightLayoutVisible() const
{
    return ScriptExportDialog::isRightLayoutVisible()
        || (d->includeTreatment->isVisibleTo(this) && d->includeTreatment->isChecked());
}

void ScreenplayExportDialog::processIncludeTextChanged(bool _checked) const
{
    if (_checked) {
        d->includeTreatment->setChecked(false);
    }
}

bool ScreenplayExportDialog::isExportEnabled() const
{
    return ScriptExportDialog::isExportEnabled()
        || (d->includeTreatment->isVisibleTo(this) && d->includeTreatment->isChecked());
}

bool ScreenplayExportDialog::shouldIncludeText() const
{
    return (d->includeTreatment->isVisibleTo(this) && d->includeTreatment->isChecked())
        || ScriptExportDialog::shouldIncludeText();
}

} // namespace Ui
