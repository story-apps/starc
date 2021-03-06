#include "export_dialog.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

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


namespace Ui
{

class ExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    ComboBox* fileFormat = nullptr;
    ComboBox* screenplayTemplate = nullptr;
    CheckBox* printTitlePage = nullptr;
    CheckBox* printSceneNumbers = nullptr;
    CheckBox* printDialoguesNumbers = nullptr;
    CheckBox* printReviewMarks = nullptr;
    TextField* watermark = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

ExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent)),
      screenplayTemplate(new ComboBox(_parent)),
      printTitlePage(new CheckBox(_parent)),
      printSceneNumbers(new CheckBox(_parent)),
      printDialoguesNumbers(new CheckBox(_parent)),
      printReviewMarks(new CheckBox(_parent)),
      watermark(new TextField(_parent)),
      buttonsLayout(new QHBoxLayout),
      openDocumentAfterExport(new CheckBox(_parent)),
      cancelButton(new Button(_parent)),
      exportButton(new Button(_parent))
{
    using namespace BusinessLayer;

    auto formatsModel = new QStringListModel({ "PDF", "DOCX", "FDX", "Fontain" });
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));

    screenplayTemplate->setModel(ScreenplayTemplateFacade::templates());
    for (int row = 0; row < ScreenplayTemplateFacade::templates()->rowCount(); ++row) {
        auto item = ScreenplayTemplateFacade::templates()->item(row);
        if (item->data(ScreenplayTemplateFacade::kTemplateIdRole).toString()
            != ScreenplayTemplateFacade::getTemplate().id()) {
            continue;
        }

        screenplayTemplate->setCurrentIndex(item->index());
        break;
    }

    for (auto checkBox : { printTitlePage, printSceneNumbers, printReviewMarks }) {
        checkBox->setChecked(true);
    }

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(exportButton);
}


// ****


ExportDialog::ExportDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->fileFormat, row++, 0);
    contentsLayout()->addWidget(d->screenplayTemplate, row++, 0);
    contentsLayout()->addWidget(d->printTitlePage, row++, 0);
    contentsLayout()->addWidget(d->printSceneNumbers, row++, 0);
    contentsLayout()->addWidget(d->printDialoguesNumbers, row++, 0);
    contentsLayout()->addWidget(d->printReviewMarks, row++, 0);
    contentsLayout()->addWidget(d->watermark, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, [this] {
        auto isScreenplayTemplateVisible = true;
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
                isWatermarkVisible = false;
                break;
            }

            //
            // FDX
            //
            case 2:
            //
            // Foumtain
            //
            case 3: {
                isScreenplayTemplateVisible = false;
                isPrintDialoguesNumbersVisible = false;
                isPrintReviewMarksVisible = false;
                isWatermarkVisible = false;
                break;
            }
        }
        d->screenplayTemplate->setVisible(isScreenplayTemplateVisible);
        d->printDialoguesNumbers->setVisible(isPrintDialoguesNumbersVisible);
        d->printReviewMarks->setVisible(isPrintReviewMarksVisible);
        d->watermark->setVisible(isWatermarkVisible);
    });
    connect(d->exportButton, &Button::clicked, this, &ExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &ExportDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ExportDialog::~ExportDialog() = default;

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
    d->printSceneNumbers->setText(tr("Print scenes numbers"));
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

    for (auto textField : QVector<TextField*>{ d->fileFormat, d->screenplayTemplate, d->watermark }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox : { d->printTitlePage, d->printSceneNumbers,
                           d->printDialoguesNumbers, d->printReviewMarks,
                           d->openDocumentAfterExport }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->exportButton,
                         d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0,
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

} //namespace Ui
