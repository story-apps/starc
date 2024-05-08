#include "stageplay_export_dialog.h"

#include <business_layer/export/stageplay/stageplay_export_options.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QEvent>
#include <QGridLayout>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringListModel>


namespace Ui {

namespace {
const QString kGroupKey = "widgets/stageplay-export-dialog/";
const QString kIncludeTitlePageKey = kGroupKey + "include-title-page";
const QString kIncludeSynopsisKey = kGroupKey + "include-synopsis";
const QString kIncludeScriptKey = kGroupKey + "include-script";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeInlineNotesKey = kGroupKey + "include-inline-notes";
const QString kIncludeReviewMarksKey = kGroupKey + "include-review-marks";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kWatermarkColorKey = kGroupKey + "watermark-color";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class StageplayExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить формат файла в соответствии со строкой комбобокса
     */
    BusinessLayer::ExportFileFormat currentFileFormat();


    CheckBox* includeTitlePage = nullptr;
    CheckBox* includeSynopsis = nullptr;
    CheckBox* includeScript = nullptr;

    ComboBox* fileFormat = nullptr;
    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

StageplayExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeTitlePage(new CheckBox(_parent))
    , includeSynopsis(new CheckBox(_parent))
    , includeScript(new CheckBox(_parent))
    , fileFormat(new ComboBox(_parent))
    , includeInlineNotes(new CheckBox(_parent))
    , includeReviewMarks(new CheckBox(_parent))
    , watermark(new TextField(_parent))
    , watermarkColorPopup(new ColorPickerPopup(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({ "PDF", "DOCX", "Fountain" });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));
    watermark->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    watermark->setTrailingIcon(u8"\U000F0765");
    watermarkColorPopup->setColorCanBeDeselected(false);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(exportButton, 0, Qt::AlignVCenter);
}

BusinessLayer::ExportFileFormat StageplayExportDialog::Implementation::currentFileFormat()
{
    switch (fileFormat->currentIndex().row()) {
    default:
    case 0: {
        return BusinessLayer::ExportFileFormat::Pdf;
    }
    case 1: {
        return BusinessLayer::ExportFileFormat::Docx;
    }
    case 2: {
        return BusinessLayer::ExportFileFormat::Fountain;
    }
    }
}


// ****


StageplayExportDialog::StageplayExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    auto leftLayout = new QVBoxLayout;
    leftLayout->setContentsMargins({});
    leftLayout->setSpacing(0);
    leftLayout->addWidget(d->includeTitlePage);
    leftLayout->addWidget(d->includeSynopsis);
    leftLayout->addWidget(d->includeScript);
    leftLayout->addStretch();
    int row = 0;
    int column = 0;
    contentsLayout()->addLayout(leftLayout, row, column++, 4, 1);
    contentsLayout()->addWidget(d->fileFormat, row++, column);
    contentsLayout()->addWidget(d->includeInlineNotes, row++, column);
    contentsLayout()->addWidget(d->includeReviewMarks, row++, column);
    contentsLayout()->addWidget(d->watermark, row++, column, Qt::AlignTop);
    contentsLayout()->setRowStretch(row++, 1);
    column = 0;
    contentsLayout()->addLayout(d->buttonsLayout, row++, column, 1, 2);
    contentsLayout()->setColumnStretch(1, 1);

    auto updateParametersVisibility = [this] {
        auto isPrintInlineNotesVisible = true;
        auto isPrintReviewMarksVisible = true;
        auto isWatermarkVisible = true;
        auto isPrintSynopsisVisible = true;
        switch (d->currentFileFormat()) {
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
        case BusinessLayer::ExportFileFormat::Fountain: {
            isWatermarkVisible = false;
            isPrintSynopsisVisible = false;
            break;
        }
        }
        if (!d->includeScript->isChecked()) {
            isPrintInlineNotesVisible = false;
            isPrintReviewMarksVisible = false;
        }
        d->includeInlineNotes->setVisible(isPrintInlineNotesVisible);
        d->includeReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->watermark->setVisible(isWatermarkVisible);
        d->includeSynopsis->setVisible(isPrintSynopsisVisible);
    };
    connect(d->includeScript, &CheckBox::checkedChanged, this, updateParametersVisibility);
    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, updateParametersVisibility);
    //
    auto updateExportEnabled = [this] {
        d->exportButton->setEnabled(d->includeTitlePage->isChecked()
                                    || d->includeSynopsis->isChecked()
                                    || d->includeScript->isChecked());
    };
    connect(d->includeTitlePage, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeSynopsis, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeScript, &CheckBox::checkedChanged, this, updateExportEnabled);
    //
    connect(d->watermark, &TextField::trailingIconPressed, this, [this] {
        d->watermarkColorPopup->showPopup(d->watermark, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->watermarkColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->watermark->setTrailingIconColor(_color); });
    //
    connect(d->exportButton, &Button::clicked, this, &StageplayExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &StageplayExportDialog::canceled);

    updateParametersVisibility();
    updateExportEnabled();

    QSettings settings;
    d->includeTitlePage->setChecked(settings.value(kIncludeTitlePageKey, true).toBool());
    d->includeSynopsis->setChecked(settings.value(kIncludeSynopsisKey, true).toBool());
    d->includeScript->setChecked(settings.value(kIncludeScriptKey, false).toBool());
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeInlineNotes->setChecked(settings.value(kIncludeInlineNotesKey, false).toBool());
    d->includeReviewMarks->setChecked(settings.value(kIncludeReviewMarksKey, true).toBool());
    d->watermark->setText(settings.value(kWatermarkKey).toString());
    d->watermarkColorPopup->setSelectedColor(
        settings.value(kWatermarkColorKey, QColor("#B7B7B7")).value<QColor>());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());

    d->watermark->setTrailingIconColor(d->watermarkColorPopup->selectedColor());
}

StageplayExportDialog::~StageplayExportDialog()
{
    QSettings settings;
    settings.setValue(kIncludeTitlePageKey, d->includeTitlePage->isChecked());
    settings.setValue(kIncludeSynopsisKey, d->includeSynopsis->isChecked());
    settings.setValue(kIncludeScriptKey, d->includeScript->isChecked());
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeInlineNotesKey, d->includeInlineNotes->isChecked());
    settings.setValue(kIncludeReviewMarksKey, d->includeReviewMarks->isChecked());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kWatermarkColorKey, d->watermarkColorPopup->selectedColor());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::StageplayExportOptions StageplayExportDialog::exportOptions() const
{
    BusinessLayer::StageplayExportOptions options;
    options.fileFormat = d->currentFileFormat();
    options.includeTiltePage = d->includeTitlePage->isChecked();
    options.includeSynopsis
        = d->includeSynopsis->isVisibleTo(this) ? d->includeSynopsis->isChecked() : false;
    options.includeText = d->includeScript->isChecked();
    options.includeInlineNotes = d->includeInlineNotes->isChecked();
    options.includeReviewMarks = d->includeReviewMarks->isChecked();
    options.watermark = d->watermark->text();
    options.watermarkColor = ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3);
    return options;
}

bool StageplayExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* StageplayExportDialog::focusedWidgetAfterShow() const
{
    return d->includeTitlePage;
}

QWidget* StageplayExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void StageplayExportDialog::updateTranslations()
{
    setTitle(tr("Export stageplay"));

    d->includeTitlePage->setText(tr("Title page"));
    d->includeSynopsis->setText(tr("Synopsis"));
    d->includeScript->setText(tr("Script"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void StageplayExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(topLevelWidget()->width() * 0.7);

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
             d->includeSynopsis,
             d->includeScript,
             d->includeInlineNotes,
             d->includeReviewMarks,
             d->openDocumentAfterExport,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->exportButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    d->watermarkColorPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->watermarkColorPopup->setTextColor(Ui::DesignSystem::color().onBackground());

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0, Ui::DesignSystem::layout().px24(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px12())
                                             .toMargins());
}

} // namespace Ui
