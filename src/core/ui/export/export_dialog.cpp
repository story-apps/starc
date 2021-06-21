#include "export_dialog.h"

#include <business_layer/export/screenplay/export_options.h>
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

class ExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    ComboBox* fileFormat = nullptr;
    ComboBox* screenplayTemplate = nullptr;
    CheckBox* printTitlePage = nullptr;
    CheckBox* printFolders = nullptr;
    CheckBox* printInlineNotes = nullptr;
    CheckBox* printSceneNumbers = nullptr;
    CheckBox* printSceneNumbersOnLeft = nullptr;
    CheckBox* printSceneNumbersOnRight = nullptr;
    CheckBox* printDialoguesNumbers = nullptr;
    CheckBox* printReviewMarks = nullptr;
    TextField* watermark = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

ExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent))
    , screenplayTemplate(new ComboBox(_parent))
    , printTitlePage(new CheckBox(_parent))
    , printFolders(new CheckBox(_parent))
    , printInlineNotes(new CheckBox(_parent))
    , printSceneNumbers(new CheckBox(_parent))
    , printSceneNumbersOnLeft(new CheckBox(_parent))
    , printSceneNumbersOnRight(new CheckBox(_parent))
    , printDialoguesNumbers(new CheckBox(_parent))
    , printReviewMarks(new CheckBox(_parent))
    , watermark(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    using namespace BusinessLayer;

    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({ "PDF" /*, "DOCX", "FDX", "Fontain" */ });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));

    screenplayTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayTemplate->setModel(TemplatesFacade::screenplayTemplates());
    for (int row = 0; row < TemplatesFacade::screenplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::screenplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString()
            != TemplatesFacade::screenplayTemplate().id()) {
            continue;
        }

        screenplayTemplate->setCurrentIndex(item->index());
        break;
    }

    printTitlePage->hide();

    for (auto checkBox :
         { /*printTitlePage,*/ printFolders, printSceneNumbers, printSceneNumbersOnLeft,
           printSceneNumbersOnRight, printReviewMarks, openDocumentAfterExport }) {
        checkBox->setChecked(true);
    }

    watermark->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(exportButton);
}


// ****


ExportDialog::ExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->fileFormat, row++, 0);
    contentsLayout()->addWidget(d->screenplayTemplate, row++, 0);
    contentsLayout()->addWidget(d->printTitlePage, row++, 0);
    contentsLayout()->addWidget(d->printFolders, row++, 0);
    contentsLayout()->addWidget(d->printInlineNotes, row++, 0);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->printSceneNumbers);
        layout->addWidget(d->printSceneNumbersOnLeft);
        layout->addWidget(d->printSceneNumbersOnRight);
        contentsLayout()->addLayout(layout, row++, 0);
    }
    contentsLayout()->addWidget(d->printDialoguesNumbers, row++, 0);
    contentsLayout()->addWidget(d->printReviewMarks, row++, 0);
    contentsLayout()->addWidget(d->watermark, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, [this] {
        auto isScreenplayTemplateVisible = true;
        auto isPrintFoldersVisible = true;
        auto isPrintInlineNotesVisible = true;
        auto isPrintScenesNumbersOnVisible = true;
        auto isPrintDialoguesNumbersVisible = true;
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
            isPrintScenesNumbersOnVisible = false;
            isWatermarkVisible = false;
            break;
        }

        //
        // FDX
        //
        case 2: {
            isPrintScenesNumbersOnVisible = false;
            isPrintDialoguesNumbersVisible = false;
            isPrintReviewMarksVisible = false;
            isWatermarkVisible = false;
            break;
        }

        //
        // Foumtain
        //
        case 3: {
            isScreenplayTemplateVisible = false;
            isPrintFoldersVisible = false;
            isPrintInlineNotesVisible = false;
            isPrintScenesNumbersOnVisible = false;
            isPrintDialoguesNumbersVisible = false;
            isPrintReviewMarksVisible = false;
            isWatermarkVisible = false;
            break;
        }
        }
        d->screenplayTemplate->setVisible(isScreenplayTemplateVisible);
        d->printFolders->setVisible(isPrintFoldersVisible);
        d->printInlineNotes->setVisible(isPrintInlineNotesVisible);
        d->printSceneNumbersOnLeft->setVisible(isPrintScenesNumbersOnVisible);
        d->printSceneNumbersOnRight->setVisible(isPrintScenesNumbersOnVisible);
        d->printDialoguesNumbers->setVisible(isPrintDialoguesNumbersVisible);
        d->printReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->watermark->setVisible(isWatermarkVisible);
    });
    connect(d->printSceneNumbers, &CheckBox::checkedChanged, d->printSceneNumbersOnLeft,
            &CheckBox::setEnabled);
    connect(d->printSceneNumbers, &CheckBox::checkedChanged, d->printSceneNumbersOnRight,
            &CheckBox::setEnabled);
    auto screenplayEditorCorrectShownSceneNumber = [this] {
        if (!d->printSceneNumbersOnLeft->isChecked() && !d->printSceneNumbersOnRight->isChecked()) {
            d->printSceneNumbersOnLeft->setChecked(true);
        }
    };
    connect(d->printSceneNumbersOnLeft, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShownSceneNumber);
    connect(d->printSceneNumbersOnRight, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShownSceneNumber);

    connect(d->exportButton, &Button::clicked, this, &ExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &ExportDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ExportDialog::~ExportDialog() = default;

BusinessLayer::ExportOptions ExportDialog::exportOptions() const
{
    BusinessLayer::ExportOptions options;
    options.templateId = d->screenplayTemplate->currentIndex()
                             .data(BusinessLayer::TemplatesFacade::kTemplateIdRole)
                             .toString();
    options.printTiltePage = d->printTitlePage->isChecked();
    options.printFolders = d->printFolders->isChecked();
    options.printInlineNotes = d->printInlineNotes->isChecked();
    options.printScenesNumbers = d->printSceneNumbers->isChecked();
    options.printScenesNumbersOnLeft = d->printSceneNumbersOnLeft->isChecked();
    options.printScenesNumbersOnRight = d->printSceneNumbersOnRight->isChecked();
    options.printDialoguesNumbers = d->printDialoguesNumbers->isChecked();
    options.printReviewMarks = d->printReviewMarks->isChecked();
    options.watermark = d->watermark->text();
    options.watermarkColor = QColor(100, 100, 100, 30);
    return options;
}

bool ExportDialog::openDocumentAfetrExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* ExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* ExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void ExportDialog::updateTranslations()
{
    setTitle(tr("Export screenplay"));

    d->fileFormat->setLabel(tr("Format"));
    d->screenplayTemplate->setLabel(tr("Template"));
    d->printTitlePage->setText(tr("Print title page"));
    d->printFolders->setText(tr("Print folders"));
    d->printInlineNotes->setText(tr("Print inline notes"));
    d->printSceneNumbers->setText(tr("Print scenes numbers"));
    d->printSceneNumbersOnLeft->setText(tr("on the left"));
    d->printSceneNumbersOnRight->setText(tr("on the right"));
    d->printDialoguesNumbers->setText(tr("Print dialogues numbers"));
    d->printReviewMarks->setText(tr("Print review marks"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void ExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    for (auto textField :
         QVector<TextField*>{ d->fileFormat, d->screenplayTemplate, d->watermark }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox :
         { d->printTitlePage, d->printFolders, d->printInlineNotes, d->printSceneNumbers,
           d->printSceneNumbersOnLeft, d->printSceneNumbersOnRight, d->printDialoguesNumbers,
           d->printReviewMarks, d->openDocumentAfterExport }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->exportButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0, Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8())
                                             .toMargins());
}

} // namespace Ui
