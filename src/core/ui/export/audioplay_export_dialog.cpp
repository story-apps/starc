#include "audioplay_export_dialog.h"

#include <business_layer/export/audioplay/audioplay_export_options.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>

#include <QEvent>
#include <QGridLayout>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringListModel>


namespace Ui {

namespace {
const QString kGroupKey = "widgets/audioplay-export-dialog/";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeTitlePageKey = kGroupKey + "include-title-page";
const QString kIncludeInlineNotesKey = kGroupKey + "include-inline-notes";
const QString kIncludeReviewMarksKey = kGroupKey + "include-review-marks";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class AudioplayExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    ComboBox* fileFormat = nullptr;
    CheckBox* includeTitlePage = nullptr;
    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;
    TextField* watermark = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

AudioplayExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent))
    , includeTitlePage(new CheckBox(_parent))
    , includeInlineNotes(new CheckBox(_parent))
    , includeReviewMarks(new CheckBox(_parent))
    , watermark(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    using namespace BusinessLayer;

    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({ "PDF", "DOCX" });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));

    watermark->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(exportButton, 0, Qt::AlignVCenter);
}


// ****


AudioplayExportDialog::AudioplayExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->fileFormat, row++, 0);
    contentsLayout()->addWidget(d->includeTitlePage, row++, 0);
    contentsLayout()->addWidget(d->includeInlineNotes, row++, 0);
    contentsLayout()->addWidget(d->includeReviewMarks, row++, 0);
    contentsLayout()->addWidget(d->watermark, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, [this] {
        auto isPrintFoldersVisible = true;
        auto isPrintInlineNotesVisible = true;
        auto isPrintReviewMarksVisible = true;
        auto isWatermarkVisible = true;
        switch (d->fileFormat->currentIndex().row()) {
        //
        // PDF
        //
        default:
        case 0: {
            //
            // ... всё видимое
            //
            break;
        }

        //
        // DOCX
        //
        case 1: {
            isWatermarkVisible = false;
            break;
        }
        }
        d->includeInlineNotes->setVisible(isPrintInlineNotesVisible);
        d->includeReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->watermark->setVisible(isWatermarkVisible);
    });
    //
    connect(d->exportButton, &Button::clicked, this, &AudioplayExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &AudioplayExportDialog::canceled);

    QSettings settings;
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeTitlePage->setChecked(settings.value(kIncludeTitlePageKey, true).toBool());
    d->includeInlineNotes->setChecked(settings.value(kIncludeInlineNotesKey, false).toBool());
    d->includeReviewMarks->setChecked(settings.value(kIncludeReviewMarksKey, true).toBool());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

AudioplayExportDialog::~AudioplayExportDialog()
{
    QSettings settings;
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeTitlePageKey, d->includeTitlePage->isChecked());
    settings.setValue(kIncludeInlineNotesKey, d->includeInlineNotes->isChecked());
    settings.setValue(kIncludeReviewMarksKey, d->includeReviewMarks->isChecked());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::AudioplayExportOptions AudioplayExportDialog::exportOptions() const
{
    BusinessLayer::AudioplayExportOptions options;
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeTiltePage = d->includeTitlePage->isChecked();
    options.includeInlineNotes = d->includeInlineNotes->isChecked();
    options.includeReviewMarks = d->includeReviewMarks->isChecked();
    options.watermark = d->watermark->text();
    options.watermarkColor = QColor(100, 100, 100, 30);
    return options;
}

bool AudioplayExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* AudioplayExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* AudioplayExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void AudioplayExportDialog::updateTranslations()
{
    setTitle(tr("Export audioplay"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeTitlePage->setText(tr("Include title page"));
    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void AudioplayExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    for (auto textField : std::vector<TextField*>{
             d->fileFormat,
             d->watermark,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->fileFormat,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }

    for (auto checkBox : {
             d->includeTitlePage,
             d->includeInlineNotes,
             d->includeReviewMarks,
             d->openDocumentAfterExport,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->exportButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0, Ui::DesignSystem::layout().px24(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px12())
                                             .toMargins());
}

} // namespace Ui
