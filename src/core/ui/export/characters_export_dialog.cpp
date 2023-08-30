#include "characters_export_dialog.h"

#include <business_layer/export/characters/characters_export_options.h>
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
const QString kGroupKey = "widgets/characters-export-dialog/";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeMainPhotoKey = kGroupKey + "include-main-photo";
const QString kIncludeStoryRoleKey = kGroupKey + "include-story-role";
const QString kIncludeAgeKey = kGroupKey + "include-age";
const QString kIncludeGenderKey = kGroupKey + "include-gender";
const QString kIncludeOneLineDescriptionKey = kGroupKey + "include-one-line-description";
const QString kIncludeLongDescriptionKey = kGroupKey + "include-long-description";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kWatermarkColorKey = kGroupKey + "watermark-color";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class CharactersExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    ComboBox* fileFormat = nullptr;
    CheckBox* includeMainPhoto = nullptr;
    CheckBox* includeStoryRole = nullptr;
    CheckBox* includeAge = nullptr;
    CheckBox* includeGender = nullptr;
    CheckBox* includeOneLineDescription = nullptr;
    CheckBox* includeLongDescription = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

CharactersExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent))
    , includeMainPhoto(new CheckBox(_parent))
    , includeStoryRole(new CheckBox(_parent))
    , includeAge(new CheckBox(_parent))
    , includeGender(new CheckBox(_parent))
    , includeOneLineDescription(new CheckBox(_parent))
    , includeLongDescription(new CheckBox(_parent))
    , watermark(new TextField(_parent))
    , watermarkColorPopup(new ColorPickerPopup(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel({
        "PDF",
        "DOCX",
    });
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


// ****


CharactersExportDialog::CharactersExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    int column = 0;
    contentsLayout()->addWidget(d->fileFormat, row++, column);
    contentsLayout()->addWidget(d->includeMainPhoto, row++, column);
    contentsLayout()->addWidget(d->includeStoryRole, row++, column);
    contentsLayout()->addWidget(d->includeAge, row++, column);
    contentsLayout()->addWidget(d->includeGender, row++, column);
    contentsLayout()->addWidget(d->includeOneLineDescription, row++, column);
    contentsLayout()->addWidget(d->includeLongDescription, row++, column);
    contentsLayout()->addWidget(d->watermark, row++, column, Qt::AlignTop);
    contentsLayout()->setRowStretch(row++, 1);
    column = 0;
    contentsLayout()->addLayout(d->buttonsLayout, row++, column);

    auto updateParametersVisibility = [this] {
        auto isPhotoVisible = true;
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
            isPhotoVisible = false;
            isWatermarkVisible = false;
            break;
        }
        }
        d->includeMainPhoto->setVisible(isPhotoVisible);
        d->watermark->setVisible(isWatermarkVisible);
    };
    connect(d->fileFormat, &ComboBox::currentIndexChanged, this, updateParametersVisibility);
    //
    connect(d->watermark, &TextField::trailingIconPressed, this, [this] {
        d->watermarkColorPopup->showPopup(d->watermark, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->watermarkColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->watermark->setTrailingIconColor(_color); });
    //
    connect(d->exportButton, &Button::clicked, this, &CharactersExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &CharactersExportDialog::canceled);

    updateParametersVisibility();

    QSettings settings;
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeMainPhoto->setChecked(settings.value(kIncludeMainPhotoKey, true).toBool());
    d->includeStoryRole->setChecked(settings.value(kIncludeStoryRoleKey, true).toBool());
    d->includeAge->setChecked(settings.value(kIncludeAgeKey, true).toBool());
    d->includeGender->setChecked(settings.value(kIncludeGenderKey, true).toBool());
    d->includeOneLineDescription->setChecked(
        settings.value(kIncludeOneLineDescriptionKey, true).toBool());
    d->includeLongDescription->setChecked(
        settings.value(kIncludeLongDescriptionKey, true).toBool());
    d->watermark->setText(settings.value(kWatermarkKey).toString());
    d->watermarkColorPopup->setSelectedColor(
        settings.value(kWatermarkColorKey, QColor("#B7B7B7")).value<QColor>());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());

    d->watermark->setTrailingIconColor(d->watermarkColorPopup->selectedColor());
}

CharactersExportDialog::~CharactersExportDialog()
{
    QSettings settings;
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeMainPhotoKey, d->includeMainPhoto->isChecked());
    settings.setValue(kIncludeStoryRoleKey, d->includeStoryRole->isChecked());
    settings.setValue(kIncludeAgeKey, d->includeAge->isChecked());
    settings.setValue(kIncludeGenderKey, d->includeGender->isChecked());
    settings.setValue(kIncludeOneLineDescriptionKey, d->includeOneLineDescription->isChecked());
    settings.setValue(kIncludeLongDescriptionKey, d->includeLongDescription->isChecked());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kWatermarkColorKey, d->watermarkColorPopup->selectedColor());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::CharactersExportOptions CharactersExportDialog::exportOptions() const
{
    BusinessLayer::CharactersExportOptions options;
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeMainPhoto = d->includeMainPhoto->isVisible() && d->includeMainPhoto->isChecked();
    options.includeStoryRole = d->includeStoryRole->isChecked();
    options.includeAge = d->includeAge->isChecked();
    options.includeGender = d->includeGender->isChecked();
    options.includeOneLineDescription = d->includeOneLineDescription->isChecked();
    options.includeLongDescription = d->includeLongDescription->isChecked();
    options.watermark = d->watermark->text();
    options.watermarkColor = ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3);
    return options;
}

bool CharactersExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* CharactersExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* CharactersExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void CharactersExportDialog::updateTranslations()
{
    setTitle(tr("Export characters"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeMainPhoto->setText(tr("Include main photo"));
    d->includeStoryRole->setText(tr("Include story role"));
    d->includeAge->setText(tr("Include age"));
    d->includeGender->setText(tr("Include gender"));
    d->includeOneLineDescription->setText(tr("Include one line description"));
    d->includeLongDescription->setText(tr("Include long description"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void CharactersExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
             d->includeMainPhoto,
             d->includeStoryRole,
             d->includeAge,
             d->includeGender,
             d->includeOneLineDescription,
             d->includeLongDescription,
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
