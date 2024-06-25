#include "character_export_dialog.h"

#include <business_layer/export/characters/character_export_options.h>
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
const QString kGroupKey = "widgets/character-export-dialog/";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeMainPhotoKey = kGroupKey + "include-main-photo";
const QString kIncludeAdditionalPhototsKey = kGroupKey + "include-additional-photos";
const QString kIncludeStoryInfoKey = kGroupKey + "include-story-info";
const QString kIncludePersonalInfoKey = kGroupKey + "include-personal-info";
const QString kIncludePhysiqueInfoKey = kGroupKey + "include-physique-info";
const QString kIncludeLifeInfoKey = kGroupKey + "include-life-info";
const QString kIncludeAttitudeInfoKey = kGroupKey + "include-attitude-info";
const QString kIncludeBiographyInfoKey = kGroupKey + "include-biography-info";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kWatermarkColorKey = kGroupKey + "watermark-color";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class CharacterExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    ComboBox* fileFormat = nullptr;
    CheckBox* includeMainPhoto = nullptr;
    CheckBox* includeAdditionalPhotos = nullptr;
    CheckBox* includeStoryInfo = nullptr;
    CheckBox* includePersonalInfo = nullptr;
    CheckBox* includePhysiqueInfo = nullptr;
    CheckBox* includeLifeInfo = nullptr;
    CheckBox* includeAttitudeInfo = nullptr;
    CheckBox* includeBiographyInfo = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

CharacterExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent))
    , includeMainPhoto(new CheckBox(_parent))
    , includeAdditionalPhotos(new CheckBox(_parent))
    , includeStoryInfo(new CheckBox(_parent))
    , includePersonalInfo(new CheckBox(_parent))
    , includePhysiqueInfo(new CheckBox(_parent))
    , includeLifeInfo(new CheckBox(_parent))
    , includeAttitudeInfo(new CheckBox(_parent))
    , includeBiographyInfo(new CheckBox(_parent))
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


CharacterExportDialog::CharacterExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    int column = 0;
    contentsLayout()->addWidget(d->fileFormat, row++, column);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->includeMainPhoto);
        layout->addWidget(d->includeAdditionalPhotos);
        layout->addStretch();
        contentsLayout()->addLayout(layout, row++, column);
    }
    contentsLayout()->addWidget(d->includeStoryInfo, row++, column);
    contentsLayout()->addWidget(d->includePersonalInfo, row++, column);
    contentsLayout()->addWidget(d->includePhysiqueInfo, row++, column);
    contentsLayout()->addWidget(d->includeLifeInfo, row++, column);
    contentsLayout()->addWidget(d->includeAttitudeInfo, row++, column);
    contentsLayout()->addWidget(d->includeBiographyInfo, row++, column);
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
        d->includeAdditionalPhotos->setVisible(isPhotoVisible);
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
    connect(d->exportButton, &Button::clicked, this, &CharacterExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &CharacterExportDialog::canceled);

    updateParametersVisibility();

    QSettings settings;
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeMainPhoto->setChecked(settings.value(kIncludeMainPhotoKey, true).toBool());
    d->includeAdditionalPhotos->setChecked(
        settings.value(kIncludeAdditionalPhototsKey, false).toBool());
    d->includeStoryInfo->setChecked(settings.value(kIncludeStoryInfoKey, true).toBool());
    d->includePersonalInfo->setChecked(settings.value(kIncludePersonalInfoKey, false).toBool());
    d->includePhysiqueInfo->setChecked(settings.value(kIncludePhysiqueInfoKey, false).toBool());
    d->includeLifeInfo->setChecked(settings.value(kIncludeLifeInfoKey, false).toBool());
    d->includeAttitudeInfo->setChecked(settings.value(kIncludeAttitudeInfoKey, false).toBool());
    d->includeBiographyInfo->setChecked(settings.value(kIncludeBiographyInfoKey, false).toBool());
    d->watermark->setText(settings.value(kWatermarkKey).toString());
    d->watermarkColorPopup->setSelectedColor(
        settings.value(kWatermarkColorKey, QColor("#B7B7B7")).value<QColor>());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());

    d->watermark->setTrailingIconColor(d->watermarkColorPopup->selectedColor());
}

CharacterExportDialog::~CharacterExportDialog()
{
    QSettings settings;
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeMainPhotoKey,
                      d->includeMainPhoto->isVisible() && d->includeMainPhoto->isChecked());
    settings.setValue(kIncludeAdditionalPhototsKey,
                      d->includeAdditionalPhotos->isVisible()
                          && d->includeAdditionalPhotos->isChecked());
    settings.setValue(kIncludeStoryInfoKey, d->includeStoryInfo->isChecked());
    settings.setValue(kIncludePersonalInfoKey, d->includePersonalInfo->isChecked());
    settings.setValue(kIncludePhysiqueInfoKey, d->includePhysiqueInfo->isChecked());
    settings.setValue(kIncludeLifeInfoKey, d->includeLifeInfo->isChecked());
    settings.setValue(kIncludeAttitudeInfoKey, d->includeAttitudeInfo->isChecked());
    settings.setValue(kIncludeBiographyInfoKey, d->includeBiographyInfo->isChecked());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kWatermarkColorKey, d->watermarkColorPopup->selectedColor());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::CharacterExportOptions CharacterExportDialog::exportOptions() const
{
    BusinessLayer::CharacterExportOptions options;
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeMainPhoto
        = d->includeMainPhoto->isVisibleTo(this) && d->includeMainPhoto->isChecked();
    options.includeAdditionalPhotos
        = d->includeAdditionalPhotos->isVisibleTo(this) && d->includeAdditionalPhotos->isChecked();
    options.includeStoryInfo
        = d->includeStoryInfo->isVisibleTo(this) && d->includeStoryInfo->isChecked();
    options.includePersonalInfo
        = d->includePersonalInfo->isVisibleTo(this) && d->includePersonalInfo->isChecked();
    options.includePhysiqueInfo
        = d->includePhysiqueInfo->isVisibleTo(this) && d->includePhysiqueInfo->isChecked();
    options.includeLifeInfo
        = d->includeLifeInfo->isVisibleTo(this) && d->includeLifeInfo->isChecked();
    options.includeAttitudeInfo
        = d->includeAttitudeInfo->isVisibleTo(this) && d->includeAttitudeInfo->isChecked();
    options.includeBiographyInfo
        = d->includeBiographyInfo->isVisibleTo(this) && d->includeBiographyInfo->isChecked();
    options.watermark = d->watermark->isVisibleTo(this) ? d->watermark->text() : "";
    options.watermarkColor = d->watermarkColorPopup->isVisibleTo(this)
        ? ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3)
        : QColor();
    return options;
}

bool CharacterExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* CharacterExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* CharacterExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void CharacterExportDialog::updateTranslations()
{
    setTitle(tr("Export character"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeMainPhoto->setText(tr("Include main photo"));
    d->includeAdditionalPhotos->setText(tr("additional photos"));
    d->includeStoryInfo->setText(tr("Include story info"));
    d->includePersonalInfo->setText(tr("Include personal info"));
    d->includePhysiqueInfo->setText(tr("Include physique info"));
    d->includeLifeInfo->setText(tr("Include life info"));
    d->includeAttitudeInfo->setText(tr("Include attitude info"));
    d->includeBiographyInfo->setText(tr("Include biography"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void CharacterExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
             d->includeAdditionalPhotos,
             d->includeStoryInfo,
             d->includePersonalInfo,
             d->includePhysiqueInfo,
             d->includeLifeInfo,
             d->includeAttitudeInfo,
             d->includeBiographyInfo,
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
