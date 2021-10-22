#include "screenplay_export_dialog.h"

#include <business_layer/export/screenplay/screenplay_export_options.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>

#include <QEvent>
#include <QGridLayout>
#include <QStandardItemModel>
#include <QStringListModel>


namespace Ui {

class ScreenplayExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список сцен для печати
     */
    QVector<QString> scenesToPrint() const;


    ComboBox* fileFormat = nullptr;
    CheckBox* printTitlePage = nullptr;
    CheckBox* printFolders = nullptr;
    CheckBox* printInlineNotes = nullptr;
    CheckBox* printReviewMarks = nullptr;
    TextField* printScenes = nullptr;
    TextField* watermark = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

ScreenplayExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent))
    , printTitlePage(new CheckBox(_parent))
    , printFolders(new CheckBox(_parent))
    , printInlineNotes(new CheckBox(_parent))
    , printReviewMarks(new CheckBox(_parent))
    , printScenes(new TextField(_parent))
    , watermark(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    using namespace BusinessLayer;

    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({ "PDF", "DOCX" /*, "FDX", "Fontain" */ });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));

    for (auto checkBox :
         { printTitlePage, printFolders, printReviewMarks, openDocumentAfterExport }) {
        checkBox->setChecked(true);
    }

    printScenes->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    watermark->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(exportButton);
}

QVector<QString> ScreenplayExportDialog::Implementation::scenesToPrint() const
{
    QVector<QString> scenes;
    const auto scenesRanges = printScenes->text().split(',', Qt::SkipEmptyParts);
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
                    scenes.append(QString::number(scene));
                }
            } else if (!range.isEmpty()) {
                scenes.append(range.constFirst());
            }
        } else {
            scenes.append(scenesRange);
        }
    }
    return scenes;
}


// ****


ScreenplayExportDialog::ScreenplayExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->fileFormat, row++, 0);
    contentsLayout()->addWidget(d->printTitlePage, row++, 0);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->printFolders);
        layout->addWidget(d->printInlineNotes);
        layout->addStretch();
        contentsLayout()->addLayout(layout, row++, 0);
    }
    contentsLayout()->addWidget(d->printReviewMarks, row++, 0);
    contentsLayout()->addWidget(d->printScenes, row++, 0);
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

        //
        // FDX
        //
        case 2: {
            isPrintReviewMarksVisible = false;
            isWatermarkVisible = false;
            break;
        }

        //
        // Foumtain
        //
        case 3: {
            isPrintFoldersVisible = false;
            isPrintInlineNotesVisible = false;
            isPrintReviewMarksVisible = false;
            isWatermarkVisible = false;
            break;
        }
        }
        d->printFolders->setVisible(isPrintFoldersVisible);
        d->printInlineNotes->setVisible(isPrintInlineNotesVisible);
        d->printReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->watermark->setVisible(isWatermarkVisible);
    });

    connect(d->exportButton, &Button::clicked, this, &ScreenplayExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &ScreenplayExportDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayExportDialog::~ScreenplayExportDialog() = default;

BusinessLayer::ScreenplayExportOptions ScreenplayExportDialog::exportOptions() const
{
    BusinessLayer::ScreenplayExportOptions options;
    options.fileFormat = static_cast<BusinessLayer::ScreenplayExportFileFormat>(
        d->fileFormat->currentIndex().row());
    options.includeTiltePage = d->printTitlePage->isChecked();
    options.includeFolders = d->printFolders->isChecked();
    options.includeInlineNotes = d->printInlineNotes->isChecked();
    options.includeReviewMarks = d->printReviewMarks->isChecked();
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
    return d->fileFormat;
}

QWidget* ScreenplayExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void ScreenplayExportDialog::updateTranslations()
{
    setTitle(tr("Export screenplay"));

    d->fileFormat->setLabel(tr("Format"));
    d->printTitlePage->setText(tr("Include title page"));
    d->printFolders->setText(tr("Include sequences headers and footers"));
    d->printInlineNotes->setText(tr("Include inline notes"));
    d->printReviewMarks->setText(tr("Include review marks"));
    d->printScenes->setLabel(tr("Export concrete scenes"));
    d->printScenes->setHelper(tr("Keep empty, if you want to print all scenes"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void ScreenplayExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    for (auto textField : std::vector<TextField*>{
             d->fileFormat,
             d->printScenes,
             d->watermark,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox : {
             d->printTitlePage,
             d->printFolders,
             d->printInlineNotes,
             d->printReviewMarks,
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
                                                   Ui::DesignSystem::layout().px8())
                                             .toMargins());
}

} // namespace Ui
