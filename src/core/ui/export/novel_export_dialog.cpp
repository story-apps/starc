#include "novel_export_dialog.h"

#include <business_layer/export/novel/novel_export_options.h>
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
const QString kGroupKey = "widgets/novel-export-dialog/";
const QString kIncludeTitlePageKey = kGroupKey + "include-title-page";
const QString kIncludeSynopsisKey = kGroupKey + "include-synopsis";
const QString kIncludeOutlineKey = kGroupKey + "include-outline";
const QString kIncludeNovelKey = kGroupKey + "include-novel";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeFootersKey = kGroupKey + "include-footer";
const QString kIncludeInlineNotesKey = kGroupKey + "include-inline-notes";
const QString kIncludeReviewMarksKey = kGroupKey + "include-review-marks";
const QString kScenesToPrintKey = kGroupKey + "scenes-to-print";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kWatermarkColorKey = kGroupKey + "watermark-color";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class NovelExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить формат файла в соответствии со строкой комбобокса
     */
    BusinessLayer::ExportFileFormat currentFileFormat();

    /**
     * @brief Получить список сцен для печати
     */
    QVector<QString> scenesToPrint() const;


    CheckBox* includeTitlePage = nullptr;
    CheckBox* includeSynopsis = nullptr;
    CheckBox* includeOutline = nullptr;
    CheckBox* includeNovel = nullptr;

    ComboBox* fileFormat = nullptr;
    CheckBox* includeFooters = nullptr;
    CheckBox* includeInlineNotes = nullptr;
    CheckBox* includeReviewMarks = nullptr;
    TextField* ornamentalBreak = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

NovelExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeTitlePage(new CheckBox(_parent))
    , includeSynopsis(new CheckBox(_parent))
    , includeOutline(new CheckBox(_parent))
    , includeNovel(new CheckBox(_parent))
    , fileFormat(new ComboBox(_parent))
    , includeFooters(new CheckBox(_parent))
    , includeInlineNotes(new CheckBox(_parent))
    , includeReviewMarks(new CheckBox(_parent))
    , ornamentalBreak(new TextField(_parent))
    , watermark(new TextField(_parent))
    , watermarkColorPopup(new ColorPickerPopup(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({ "PDF", "DOCX", "Markdown" });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));
    ornamentalBreak->setSpellCheckPolicy(SpellCheckPolicy::Manual);
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

BusinessLayer::ExportFileFormat NovelExportDialog::Implementation::currentFileFormat()
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
        return BusinessLayer::ExportFileFormat::Markdown;
    }
    }
}

QVector<QString> NovelExportDialog::Implementation::scenesToPrint() const
{
    QVector<QString> scenesToPrint;
    const auto scenesRanges = ornamentalBreak->text().split(',', Qt::SkipEmptyParts);
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


NovelExportDialog::NovelExportDialog(QWidget* _parent)
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
    leftLayout->addWidget(d->includeOutline);
    leftLayout->addWidget(d->includeNovel);
    leftLayout->addStretch();
    //
    int row = 0;
    int column = 0;
    contentsLayout()->addLayout(leftLayout, row, column++, 6, 1);
    contentsLayout()->addWidget(d->fileFormat, row++, column);
    contentsLayout()->addWidget(d->includeFooters, row++, column);
    contentsLayout()->addWidget(d->includeInlineNotes, row++, column);
    contentsLayout()->addWidget(d->includeReviewMarks, row++, column);
    contentsLayout()->addWidget(d->ornamentalBreak, row++, column);
    contentsLayout()->addWidget(d->watermark, row, column, Qt::AlignTop);
    contentsLayout()->setRowStretch(row++, 1);
    column = 0;
    contentsLayout()->addLayout(d->buttonsLayout, row++, column, 1, 2);
    contentsLayout()->setColumnStretch(1, 1);

    connect(d->includeOutline, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            d->includeNovel->setChecked(false);
        }
    });
    connect(d->includeNovel, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            d->includeOutline->setChecked(false);
        }
    });
    //
    auto updateParametersVisibility = [this] {
        auto isPrintSynopsisVisible = true;
        auto isPrintInlineNotesVisible = true;
        auto isPrintReviewMarksVisible = true;
        auto exportConcreteScenesVisible = true;
        auto isWatermarkVisible = true;
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
        case BusinessLayer::ExportFileFormat::Markdown: {
            isPrintSynopsisVisible = false;
            isPrintInlineNotesVisible = false;
            isPrintReviewMarksVisible = false;
            isWatermarkVisible = false;
            break;
        }
        }

        if (!d->includeOutline->isChecked() && !d->includeNovel->isChecked()) {
            isPrintInlineNotesVisible = false;
            isPrintReviewMarksVisible = false;
            exportConcreteScenesVisible = false;
        }

        d->includeSynopsis->setVisible(isPrintSynopsisVisible);
        d->includeInlineNotes->setVisible(isPrintInlineNotesVisible);
        d->includeReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->ornamentalBreak->setVisible(exportConcreteScenesVisible);
        d->watermark->setVisible(isWatermarkVisible);
    };
    connect(d->includeOutline, &CheckBox::checkedChanged, this, updateParametersVisibility);
    connect(d->includeNovel, &CheckBox::checkedChanged, this, updateParametersVisibility);
    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, updateParametersVisibility);
    //
    auto updateExportEnabled = [this] {
        d->exportButton->setEnabled(
            d->includeTitlePage->isChecked() || d->includeSynopsis->isChecked()
            || d->includeOutline->isChecked() || d->includeNovel->isChecked());
    };
    connect(d->includeTitlePage, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeSynopsis, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeOutline, &CheckBox::checkedChanged, this, updateExportEnabled);
    connect(d->includeNovel, &CheckBox::checkedChanged, this, updateExportEnabled);
    //
    connect(d->watermark, &TextField::trailingIconPressed, this, [this] {
        d->watermarkColorPopup->showPopup(d->watermark, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->watermarkColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->watermark->setTrailingIconColor(_color); });
    //
    connect(d->exportButton, &Button::clicked, this, &NovelExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &NovelExportDialog::canceled);

    updateParametersVisibility();
    updateExportEnabled();

    QSettings settings;
    d->includeTitlePage->setChecked(settings.value(kIncludeTitlePageKey, true).toBool());
    d->includeSynopsis->setChecked(settings.value(kIncludeSynopsisKey, true).toBool());
    d->includeOutline->setChecked(settings.value(kIncludeOutlineKey, false).toBool());
    d->includeNovel->setChecked(settings.value(kIncludeNovelKey, true).toBool());
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeFooters->setChecked(settings.value(kIncludeFootersKey, false).toBool());
    d->includeInlineNotes->setChecked(settings.value(kIncludeInlineNotesKey, false).toBool());
    d->includeReviewMarks->setChecked(settings.value(kIncludeReviewMarksKey, true).toBool());
    d->ornamentalBreak->setText(settings.value(kScenesToPrintKey).toString());
    d->watermark->setText(settings.value(kWatermarkKey).toString());
    d->watermarkColorPopup->setSelectedColor(
        settings.value(kWatermarkColorKey, QColor("#B7B7B7")).value<QColor>());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());

    d->watermark->setTrailingIconColor(d->watermarkColorPopup->selectedColor());
}

NovelExportDialog::~NovelExportDialog()
{
    QSettings settings;
    settings.setValue(kIncludeTitlePageKey, d->includeTitlePage->isChecked());
    settings.setValue(kIncludeSynopsisKey, d->includeSynopsis->isChecked());
    settings.setValue(kIncludeOutlineKey, d->includeOutline->isChecked());
    settings.setValue(kIncludeNovelKey, d->includeNovel->isChecked());
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeFootersKey, d->includeFooters->isChecked());
    settings.setValue(kIncludeInlineNotesKey, d->includeInlineNotes->isChecked());
    settings.setValue(kIncludeReviewMarksKey, d->includeReviewMarks->isChecked());
    settings.setValue(kScenesToPrintKey, d->ornamentalBreak->text());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kWatermarkColorKey, d->watermarkColorPopup->selectedColor());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::NovelExportOptions NovelExportDialog::exportOptions() const
{
    BusinessLayer::NovelExportOptions options;
    options.fileFormat = d->currentFileFormat();
    options.includeTitlePage
        = d->includeTitlePage->isVisibleTo(this) && d->includeTitlePage->isChecked();
    options.includeSynopsis
        = d->includeSynopsis->isVisibleTo(this) && d->includeSynopsis->isChecked();
    options.includeText = (d->includeOutline->isVisibleTo(this) && d->includeOutline->isChecked())
        || (d->includeNovel->isVisibleTo(this) && d->includeNovel->isChecked());
    options.includeOutline = d->includeOutline->isVisibleTo(this) && d->includeOutline->isChecked();
    options.includeFolders = true;
    options.includeFooters = d->includeFooters->isVisibleTo(this) && d->includeFooters->isChecked();
    options.includeInlineNotes
        = d->includeInlineNotes->isVisibleTo(this) && d->includeInlineNotes->isChecked();
    options.includeReviewMarks
        = d->includeReviewMarks->isVisibleTo(this) && d->includeReviewMarks->isChecked();
    options.ornamentalBreak = d->watermark->isVisibleTo(this) ? d->ornamentalBreak->text() : "";
    options.watermark = d->watermark->isVisibleTo(this) ? d->watermark->text() : "";
    options.watermarkColor = d->watermarkColorPopup->isVisibleTo(this)
        ? ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3)
        : QColor();
    return options;
}

bool NovelExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* NovelExportDialog::focusedWidgetAfterShow() const
{
    return d->includeTitlePage;
}

QWidget* NovelExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void NovelExportDialog::updateTranslations()
{
    setTitle(tr("Export novel"));

    d->includeTitlePage->setText(tr("Title page"));
    d->includeSynopsis->setText(tr("Synopsis"));
    d->includeOutline->setText(tr("Outline"));
    d->includeNovel->setText(tr("Novel"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeFooters->setText(tr("Include parts & chapters footers"));
    d->includeInlineNotes->setText(tr("Include inline notes"));
    d->includeReviewMarks->setText(tr("Include review marks"));
    d->ornamentalBreak->setLabel(tr("Scenes' ornamental break"));
    d->ornamentalBreak->setHelper(tr("Keep empty, if you want to print scene headings instead"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void NovelExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(topLevelWidget()->width() * 0.7);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    for (auto textField : std::vector<TextField*>{
             d->fileFormat,
             d->ornamentalBreak,
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
             d->includeOutline,
             d->includeNovel,
             d->includeFooters,
             d->includeInlineNotes,
             d->includeReviewMarks,
             d->openDocumentAfterExport,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->watermarkColorPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->watermarkColorPopup->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto button : { d->exportButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0, Ui::DesignSystem::layout().px24(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px12())
                                             .toMargins());
}

} // namespace Ui
