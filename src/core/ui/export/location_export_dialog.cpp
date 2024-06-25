#include "location_export_dialog.h"

#include <business_layer/export/locations/location_export_options.h>
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
const QString kGroupKey = "widgets/location-export-dialog/";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeMainPhotoKey = kGroupKey + "include-main-photo";
const QString kIncludeAdditionalPhototsKey = kGroupKey + "include-additional-photos";
const QString kIncludeSenseInfoKey = kGroupKey + "include-sense-info";
const QString kIncludeGeographyInfoKey = kGroupKey + "include-geography-info";
const QString kIncludeBackgroundInfoKey = kGroupKey + "include-background-info";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kWatermarkColorKey = kGroupKey + "watermark-color";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class LocationExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    ComboBox* fileFormat = nullptr;
    CheckBox* includeMainPhoto = nullptr;
    CheckBox* includeAdditionalPhotos = nullptr;
    CheckBox* includeSenseInfo = nullptr;
    CheckBox* includeGeographyInfo = nullptr;
    CheckBox* includeBackgroundInfo = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

LocationExportDialog::Implementation::Implementation(QWidget* _parent)
    : fileFormat(new ComboBox(_parent))
    , includeMainPhoto(new CheckBox(_parent))
    , includeAdditionalPhotos(new CheckBox(_parent))
    , includeSenseInfo(new CheckBox(_parent))
    , includeGeographyInfo(new CheckBox(_parent))
    , includeBackgroundInfo(new CheckBox(_parent))
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


LocationExportDialog::LocationExportDialog(QWidget* _parent)
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
    contentsLayout()->addWidget(d->includeSenseInfo, row++, column);
    contentsLayout()->addWidget(d->includeGeographyInfo, row++, column);
    contentsLayout()->addWidget(d->includeBackgroundInfo, row++, column);
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
    connect(d->exportButton, &Button::clicked, this, &LocationExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &LocationExportDialog::canceled);

    updateParametersVisibility();

    QSettings settings;
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeMainPhoto->setChecked(settings.value(kIncludeMainPhotoKey, true).toBool());
    d->includeAdditionalPhotos->setChecked(
        settings.value(kIncludeAdditionalPhototsKey, false).toBool());
    d->includeSenseInfo->setChecked(settings.value(kIncludeSenseInfoKey, true).toBool());
    d->includeGeographyInfo->setChecked(settings.value(kIncludeGeographyInfoKey, false).toBool());
    d->includeBackgroundInfo->setChecked(settings.value(kIncludeBackgroundInfoKey, false).toBool());
    d->watermark->setText(settings.value(kWatermarkKey).toString());
    d->watermarkColorPopup->setSelectedColor(
        settings.value(kWatermarkColorKey, QColor("#B7B7B7")).value<QColor>());
    d->openDocumentAfterExport->setChecked(
        settings.value(kOpenDocumentAfterExportKey, true).toBool());

    d->watermark->setTrailingIconColor(d->watermarkColorPopup->selectedColor());
}

LocationExportDialog::~LocationExportDialog()
{
    QSettings settings;
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeMainPhotoKey,
                      d->includeMainPhoto->isVisible() && d->includeMainPhoto->isChecked());
    settings.setValue(kIncludeAdditionalPhototsKey,
                      d->includeAdditionalPhotos->isVisible()
                          && d->includeAdditionalPhotos->isChecked());
    settings.setValue(kIncludeSenseInfoKey, d->includeSenseInfo->isChecked());
    settings.setValue(kIncludeGeographyInfoKey, d->includeGeographyInfo->isChecked());
    settings.setValue(kIncludeBackgroundInfoKey, d->includeBackgroundInfo->isChecked());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kWatermarkColorKey, d->watermarkColorPopup->selectedColor());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

BusinessLayer::LocationExportOptions LocationExportDialog::exportOptions() const
{
    BusinessLayer::LocationExportOptions options;
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeMainPhoto
        = d->includeMainPhoto->isVisibleTo(this) && d->includeMainPhoto->isChecked();
    options.includeAdditionalPhotos
        = d->includeAdditionalPhotos->isVisibleTo(this) && d->includeAdditionalPhotos->isChecked();
    options.includeSenseInfo
        = d->includeSenseInfo->isVisibleTo(this) && d->includeSenseInfo->isChecked();
    options.includeGeographyInfo
        = d->includeGeographyInfo->isVisibleTo(this) && d->includeGeographyInfo->isChecked();
    options.includeBackgroundInfo
        = d->includeBackgroundInfo->isVisibleTo(this) && d->includeBackgroundInfo->isChecked();
    options.watermark = d->watermark->isVisibleTo(this) ? d->watermark->text() : "";
    options.watermarkColor = d->watermarkColorPopup->isVisibleTo(this)
        ? ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3)
        : QColor();
    return options;
}

bool LocationExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* LocationExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* LocationExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void LocationExportDialog::updateTranslations()
{
    setTitle(tr("Export location"));

    d->fileFormat->setLabel(tr("Format"));
    d->includeMainPhoto->setText(tr("Include main photo"));
    d->includeAdditionalPhotos->setText(tr("additional photos"));
    d->includeSenseInfo->setText(tr("Include sense info"));
    d->includeGeographyInfo->setText(tr("Include geography info"));
    d->includeBackgroundInfo->setText(tr("Include background info"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void LocationExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
             d->includeSenseInfo,
             d->includeGeographyInfo,
             d->includeBackgroundInfo,
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
