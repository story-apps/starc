#include "audioplay_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QHBoxLayout>
#include <QSettings>

namespace Ui {

namespace {

const QString kFormatKey = "format";
const QString kHighlightCharacterKey = "highlight-character";
const QString kHighlightCharacterWithDialogueKey = "highlight-character-with-dialogue";

static const QVector<BusinessLayer::ExportFileFormat> kAudioplayExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
    BusinessLayer::ExportFileFormat::Fountain,
});

} // namespace


class AudioplayExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* highlightCharacters = nullptr;
    CheckBox* highlightCharactersWithDialogue = nullptr;

    BusinessLayer::AudioplayExportOptions options;
};

AudioplayExportDialog::Implementation::Implementation(QWidget* _parent)
    : highlightCharacters(new CheckBox(_parent))
    , highlightCharactersWithDialogue(new CheckBox(_parent))
{
}


// ****


AudioplayExportDialog::AudioplayExportDialog(const QString& _uuidKey, QWidget* _parent)
    : ScriptExportDialog(kAudioplayExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->highlightCharacters);
        layout->addWidget(d->highlightCharactersWithDialogue);
        layout->addStretch();
        rightLayout()->insertLayout(4, layout);
    }

    QSettings settings;
    setCurrentFileFormat(settings.value(uniqueKey(kFormatKey), 0).toInt());
    d->highlightCharacters->setChecked(
        settings.value(uniqueKey(kHighlightCharacterKey), false).toBool());
    d->highlightCharactersWithDialogue->setChecked(
        settings.value(uniqueKey(kHighlightCharacterWithDialogueKey), false).toBool());

    connect(d->highlightCharacters, &CheckBox::checkedChanged, d->highlightCharactersWithDialogue,
            &CheckBox::setEnabled);

    updateDialog();
}

AudioplayExportDialog::~AudioplayExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kFormatKey), currentFileFormatRow());
    settings.setValue(uniqueKey(kHighlightCharacterKey), d->highlightCharacters->isChecked());
    settings.setValue(uniqueKey(kHighlightCharacterWithDialogueKey),
                      d->highlightCharactersWithDialogue->isChecked());
}

BusinessLayer::AudioplayExportOptions& AudioplayExportDialog::exportOptions() const
{
    d->options = ScriptExportDialog::exportOptions();
    d->options.highlightCharacters
        = d->highlightCharacters->isVisibleTo(this) && d->highlightCharacters->isChecked();
    d->options.highlightCharactersWithDialogues
        = d->highlightCharactersWithDialogue->isVisibleTo(this)
        && d->highlightCharactersWithDialogue->isChecked();
    return d->options;
}

void AudioplayExportDialog::updateTranslations()
{
    ScriptExportDialog::updateTranslations();
    setTitle(tr("Export audioplay"));
    d->highlightCharacters->setText(tr("Highlight characters"));
    d->highlightCharactersWithDialogue->setText(tr("with dialogues"));
}

void AudioplayExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    ScriptExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->highlightCharacters,
             d->highlightCharactersWithDialogue,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void AudioplayExportDialog::updateParametersVisibility() const
{
    ScriptExportDialog::updateParametersVisibility();
    d->highlightCharacters->setVisible(isRightLayoutVisible());
    d->highlightCharactersWithDialogue->setVisible(isRightLayoutVisible());
    d->highlightCharactersWithDialogue->setEnabled(d->highlightCharacters->isChecked());
}


} // namespace Ui
