#include "screenplay_export_dialog.h"

#include <business_layer/export/screenplay/screenplay_export_options.h>
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
const QString kGroupKey = "widgets/screenplay-export-dialog/";
const QString kIncludeTitlePageKey = kGroupKey + "include-title-page";
const QString kIncludeSynopsisKey = kGroupKey + "include-synopsis";
const QString kIncludeTreatmentKey = kGroupKey + "include-treatment";
const QString kIncludeScreenplayKey = kGroupKey + "include-screenplay";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeSequencesKey = kGroupKey + "include-sequences";
const QString kIncludeInlineNotesKey = kGroupKey + "include-inline-notes";
const QString kIncludeReviewMarksKey = kGroupKey + "include-review-marks";
const QString kScenesToPrintKey = kGroupKey + "scenes-to-print";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class ScreenplayExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список сцен для печати
     */
    QVector<QString> scenesToPrint() const;


    CheckBox* includeTitlePage = nullptr;
    CheckBox* includeSynopsis = nullptr;
    CheckBox* includeTreatment = nullptr;
    CheckBox* includeScreenplay = nullptr;

    ComboBox* fileFormat = nullptr;
    CheckBox* includeSequences = nullptr;
    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;
    TextField* exportConcreteScenes = nullptr;
    TextField* watermark = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

ScreenplayExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeTitlePage(new CheckBox(_parent))
    , includeSynopsis(new CheckBox(_parent))
    , includeTreatment(new CheckBox(_parent))
    , includeScreenplay(new CheckBox(_parent))
    , fileFormat(new ComboBox(_parent))
    , includeSequences(new CheckBox(_parent))
    , includeInlineNotes(new CheckBox(_parent))
    , includeReviewMarks(new CheckBox(_parent))
    , exportConcreteScenes(new TextField(_parent))
    , watermark(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    using namespace BusinessLayer;

    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({ "PDF", "DOCX", "FDX", "Fountain" });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));

    exportConcreteScenes->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    watermark->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(exportButton, 0, Qt::AlignVCenter);
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
            scenesToPrint.append(scenesRange);
        }
    }
    return scenesToPrint;
}


// ****


ScreenplayExportDialog::ScreenplayExportDialog(QWidget* _parent)
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
    leftLayout->addWidget(d->includeTreatment);
    leftLayout->addWidget(d->includeScreenplay);
    leftLayout->addStretch();
    //
    int row = 0;
    int column = 0;
    contentsLayout()->addLayout(leftLayout, row, column++, 6, 1);
    contentsLayout()->addWidget(d->fileFormat, row++, column);
    contentsLayout()->addWidget(d->includeSequences, row++, column);
    contentsLayout()->addWidget(d->includeInlineNotes, row++, column);
    contentsLayout()->addWidget(d->includeReviewMarks, row++, column);
    contentsLayout()->addWidget(d->exportConcreteScenes, row++, column);
    contentsLayout()->addWidget(d->watermark, row, column, Qt::AlignTop);
    contentsLayout()->setRowStretch(row++, 1);
    column = 0;
    contentsLayout()->addLayout(d->buttonsLayout, row++, column, 1, 2);
    contentsLayout()->setColumnStretch(1, 1);

    connect(d->includeTreatment, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            d->includeScreenplay->setChecked(false);
        }
    });
    connect(d->includeScreenplay, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            d->includeTreatment->setChecked(false);
        }
    });
    //
    auto updateParametersVisibility = [this] {
        auto isPrintFoldersVisible = true;
        auto isPrintInlineNotesVisible = true;
        auto isPrintReviewMarksVisible = true;
        auto exportConcreteScenesVisible = true;
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

            //
            // FDX
            //
        case 2: {
            isPrintReviewMarksVisible = false;
            isWatermarkVisible = false;
            break;
        }

            //
            // Fountain
            //
        case 3: {
            isWatermarkVisible = false;
            break;
        }
        }

        if (!d->includeTreatment->isChecked() && !d->includeScreenplay->isChecked()) {
            isPrintFoldersVisible = false;
            isPrintInlineNotesVisible = false;
            isPrintReviewMarksVisible = false;
            exportConcreteScenesVisible = false;
        }

        d->includeSequences->setVisible(isPrintFoldersVisible);
        d->includeInlineNotes->setVisible(isPrintInlineNotesVisible);
        d->includeReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->exportConcreteScenes->setVisible(exportConcreteScenesVisible);
        d->watermark->setVisible(isWatermarkVisible);
    };
    connect(d->includeTreatment, &CheckBox::checkedChanged, this, updateParametersVisibility);
    connect(d->includeScreenplay, &CheckBox::checkedChanged, this, updateParametersVisibility);
    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, updateParametersVisibility);
    //
    auto updateExportEnabled = [this] {
        d->exportButton->setEnabled(
            d->includeTitlePage->isChecked() || d->includeSynopsis->isChecked()
            || d->includeTreatment->isChecked() || d->includeScreenplay->isChecked());
    };
    connect(d->includeTitlePage, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeSynopsis, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeTreatment, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeScreenplay, &CheckBox::checkedChanged, this, updateExportEnabled);
    //
    connect(d->exportButton, &Button::clicked, this, &ScreenplayExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &ScreenplayExportDialog::canceled);

    updateParametersVisibility();
    updateExportEnabled();

    QSettings settings;
    d->includeTitlePage->setChecked(settings.value(kIncludeTitlePageKey, true).toBool());
    d->includeSynopsis->setChecked(settings.value(kIncludeSynopsisKey, true).toBool());
    d->includeTreatment->setChecked(settings.value(kIncludeTreatmentKey, false).toBool());
    d->includeScreenplay->setChecked(settings.value(kIncludeScreenplayKey, true).toBool());
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeSequences->setChecked(settings.value(kIncludeSequencesKey, true).toBool());
    d->includeInlineNotes->setChecked(settings.value(kIncludeInlineNotesKey, false).toBool());
    d->includeReviewMarks->setChecked(settings.value(kIncludeReviewMarksKey, true).toBool());
    d->exportConcreteScenes->setText(settings.value(kScenesToPrintKey).toString());
    d->watermark->setText(settings.value(kWatermarkKey).toString());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());
}

ScreenplayExportDialog::~ScreenplayExportDialog()
{
    QSettings settings;
    settings.setValue(kIncludeTitlePageKey, d->includeTitlePage->isChecked());
    settings.setValue(kIncludeSynopsisKey, d->includeSynopsis->isChecked());
    settings.setValue(kIncludeTreatmentKey, d->includeTreatment->isChecked());
    settings.setValue(kIncludeScreenplayKey, d->includeScreenplay->isChecked());
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeSequencesKey, d->includeSequences->isChecked());
    settings.setValue(kIncludeInlineNotesKey, d->includeInlineNotes->isChecked());
    settings.setValue(kIncludeReviewMarksKey, d->includeReviewMarks->isChecked());
    settings.setValue(kScenesToPrintKey, d->exportConcreteScenes->text());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::ScreenplayExportOptions ScreenplayExportDialog::exportOptions() const
{
    BusinessLayer::ScreenplayExportOptions options;
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeTiltePage = d->includeTitlePage->isChecked();
    options.includeSynopsis = d->includeSynopsis->isChecked();
    options.includeScript = d->includeTreatment->isChecked() || d->includeScreenplay->isChecked();
    options.includeTreatment = d->includeTreatment->isChecked();
    options.includeFolders = d->includeSequences->isChecked();
    options.includeInlineNotes = d->includeInlineNotes->isChecked();
    options.includeReviewMarks = d->includeReviewMarks->isChecked();
    options.exportScenes = d->scenesToPrint();
    options.watermark = d->watermark->text();
    options.watermarkColor = QColor(100, 100, 100, 30);
    return options;
}

bool ScreenplayExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* ScreenplayExportDialog::focusedWidgetAfterShow() const
{
    return d->includeTitlePage;
}

QWidget* ScreenplayExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void ScreenplayExportDialog::updateTranslations()
{
    setTitle(tr("Export screenplay"));

    d->includeTitlePage->setText(tr("Title page"));
    d->includeSynopsis->setText(tr("Synopsis"));
    d->includeTreatment->setText(tr("Treatment"));
    d->includeScreenplay->setText(tr("Screenplay"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeSequences->setText(tr("Include sequences headers and footers"));
    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks"));
    d->exportConcreteScenes->setLabel(tr("Export concrete scenes"));
    d->exportConcreteScenes->setHelper(tr("Keep empty, if you want to print all scenes"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void ScreenplayExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(topLevelWidget()->width() * 0.7);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    for (auto textField : std::vector<TextField*>{
             d->fileFormat,
             d->exportConcreteScenes,
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
             d->includeTreatment,
             d->includeScreenplay,
             d->includeSequences,
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
