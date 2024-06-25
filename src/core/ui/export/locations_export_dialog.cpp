#include "locations_export_dialog.h"

#include <business_layer/export/locations/locations_export_options.h>
#include <business_layer/model/abstract_model.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QEvent>
#include <QGridLayout>
#include <QScrollArea>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringListModel>


namespace Ui {

namespace {
const QString kGroupKey = "widgets/locations-export-dialog/";
const QString kLocationsKey = kGroupKey + "locations";
const QString kFormatKey = kGroupKey + "format";
const QString kIncludeMainPhotoKey = kGroupKey + "include-main-photo";
const QString kIncludeStoryRoleKey = kGroupKey + "include-story-role";
const QString kIncludeOneLineDescriptionKey = kGroupKey + "include-one-line-description";
const QString kIncludeLongDescriptionKey = kGroupKey + "include-long-description";
const QString kWatermarkKey = kGroupKey + "watermark";
const QString kWatermarkColorKey = kGroupKey + "watermark-color";
const QString kOpenDocumentAfterExportKey = kGroupKey + "open-document-after-export";
} // namespace

class LocationsExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateSelectAllState();


    QScrollArea* locationsScrollArea = nullptr;
    Widget* locationsContainer = nullptr;
    QVBoxLayout* locationsLayout = nullptr;
    CheckBox* selectAllLocations = nullptr;
    QVector<CheckBox*> locations;
    ComboBox* fileFormat = nullptr;
    CheckBox* includeMainPhoto = nullptr;
    CheckBox* includeStoryRole = nullptr;
    CheckBox* includeOneLineDescription = nullptr;
    CheckBox* includeLongDescription = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;
};

LocationsExportDialog::Implementation::Implementation(QWidget* _parent)
    : locationsScrollArea(new QScrollArea(_parent))
    , locationsContainer(new Widget(locationsScrollArea))
    , locationsLayout(new QVBoxLayout)
    , selectAllLocations(new CheckBox(locationsContainer))
    , fileFormat(new ComboBox(_parent))
    , includeMainPhoto(new CheckBox(_parent))
    , includeStoryRole(new CheckBox(_parent))
    , includeOneLineDescription(new CheckBox(_parent))
    , includeLongDescription(new CheckBox(_parent))
    , watermark(new TextField(_parent))
    , watermarkColorPopup(new ColorPickerPopup(_parent))
    , buttonsLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    locationsScrollArea->setPalette(palette);
    locationsScrollArea->setFrameShape(QFrame::NoFrame);
    locationsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    locationsScrollArea->setVerticalScrollBar(new ScrollBar);
    locationsScrollArea->setWidget(locationsContainer);
    locationsScrollArea->setWidgetResizable(true);
    locationsLayout->setContentsMargins({});
    locationsLayout->addWidget(selectAllLocations);
    locationsLayout->setSpacing(0);
    locationsLayout->addStretch();
    locationsContainer->setLayout(locationsLayout);

    new Shadow(Qt::TopEdge, locationsScrollArea);
    new Shadow(Qt::BottomEdge, locationsScrollArea);


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

void LocationsExportDialog::Implementation::updateSelectAllState()
{
    bool hasChecked = false;
    bool hasUnchecked = false;
    for (const auto locationCheckBox : std::as_const(locations)) {
        if (locationCheckBox->isChecked()) {
            hasChecked = true;
        } else {
            hasUnchecked = true;
        }
    }

    if (hasChecked && hasUnchecked) {
        selectAllLocations->setIndeterminate();
    } else {
        QSignalBlocker signalBlocker(selectAllLocations);
        selectAllLocations->setChecked(hasChecked);
    }
}


// ****


LocationsExportDialog::LocationsExportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    int column = 0;
    contentsLayout()->addWidget(d->locationsScrollArea, row, column++, 7, 1);
    contentsLayout()->addWidget(d->fileFormat, row++, column);
    contentsLayout()->addWidget(d->includeMainPhoto, row++, column);
    contentsLayout()->addWidget(d->includeStoryRole, row++, column);
    contentsLayout()->addWidget(d->includeOneLineDescription, row++, column);
    contentsLayout()->addWidget(d->includeLongDescription, row++, column);
    contentsLayout()->addWidget(d->watermark, row++, column, Qt::AlignTop);
    contentsLayout()->setRowStretch(row++, 1);
    column = 0;
    contentsLayout()->addLayout(d->buttonsLayout, row++, column, 1, 2);

    connect(d->selectAllLocations, &CheckBox::checkedChanged, this,
            [this](bool _checked, bool _indeterminate) {
                if (_indeterminate) {
                    return;
                }

                for (auto locationCheckBox : std::as_const(d->locations)) {
                    QSignalBlocker signalBlocker(locationCheckBox);
                    locationCheckBox->setChecked(_checked);
                }
            });
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
    connect(d->exportButton, &Button::clicked, this, &LocationsExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &LocationsExportDialog::canceled);

    updateParametersVisibility();

    QSettings settings;
    const auto fileFormatIndex
        = d->fileFormat->model()->index(settings.value(kFormatKey, 0).toInt(), 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
    d->includeMainPhoto->setChecked(settings.value(kIncludeMainPhotoKey, true).toBool());
    d->includeStoryRole->setChecked(settings.value(kIncludeStoryRoleKey, true).toBool());
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

LocationsExportDialog::~LocationsExportDialog()
{
    QSettings settings;
    QVariantMap locations;
    for (const auto locationCheckBox : std::as_const(d->locations)) {
        locations[locationCheckBox->text()] = locationCheckBox->isChecked();
    }
    settings.setValue(kLocationsKey, locations);
    settings.setValue(kFormatKey, d->fileFormat->currentIndex().row());
    settings.setValue(kIncludeMainPhotoKey, d->includeMainPhoto->isChecked());
    settings.setValue(kIncludeStoryRoleKey, d->includeStoryRole->isChecked());
    settings.setValue(kIncludeOneLineDescriptionKey, d->includeOneLineDescription->isChecked());
    settings.setValue(kIncludeLongDescriptionKey, d->includeLongDescription->isChecked());
    settings.setValue(kWatermarkKey, d->watermark->text());
    settings.setValue(kWatermarkColorKey, d->watermarkColorPopup->selectedColor());
    settings.setValue(kOpenDocumentAfterExportKey, d->openDocumentAfterExport->isChecked());
}

void LocationsExportDialog::setModel(BusinessLayer::AbstractModel* _model) const
{
    qDeleteAll(d->locations);
    d->locations.clear();

    d->selectAllLocations->setChecked(true);
    for (int row = 0; row < _model->rowCount(); ++row) {
        auto locationCheckBox = new CheckBox(d->locationsContainer);
        locationCheckBox->setBackgroundColor(Ui::DesignSystem::color().background());
        locationCheckBox->setTextColor(Ui::DesignSystem::color().onBackground());
        locationCheckBox->setText(_model->index(row, 0).data().toString());
        locationCheckBox->setChecked(true);
        connect(locationCheckBox, &CheckBox::checkedChanged, this,
                [this] { d->updateSelectAllState(); });

        // 1 - чекбокс "Выделить всех"
        d->locationsLayout->insertWidget(1 + d->locations.size(), locationCheckBox);

        d->locations.append(locationCheckBox);
    }

    //
    // Если список персонажей не изменился с прошлого раза, то восстановим значения
    //
    QSettings settings;
    const auto locations = settings.value(kLocationsKey).toMap();
    bool isSame = d->locations.size() == locations.size();
    if (isSame) {
        for (int index = 0; index < d->locations.size(); ++index) {
            if (!locations.contains(d->locations[index]->text())) {
                isSame = false;
                break;
            }
        }
    }
    if (isSame) {
        for (int index = 0; index < d->locations.size(); ++index) {
            d->locations[index]->setChecked(locations[d->locations[index]->text()].toBool());
        }
    }
}

BusinessLayer::LocationsExportOptions LocationsExportDialog::exportOptions() const
{
    BusinessLayer::LocationsExportOptions options;
    options.locations.clear();
    for (const auto locationCheckBox : std::as_const(d->locations)) {
        if (locationCheckBox->isChecked()) {
            options.locations.append(locationCheckBox->text());
        }
    }
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeMainPhoto
        = d->includeMainPhoto->isVisibleTo(this) && d->includeMainPhoto->isChecked();
    options.includeStoryRole
        = d->includeStoryRole->isVisibleTo(this) && d->includeStoryRole->isChecked();
    options.includeOneLineDescription = d->includeOneLineDescription->isVisibleTo(this)
        && d->includeOneLineDescription->isChecked();
    options.includeLongDescription
        = d->includeLongDescription->isVisibleTo(this) && d->includeLongDescription->isChecked();
    options.watermark = d->watermark->isVisibleTo(this) ? d->watermark->text() : "";
    options.watermarkColor = d->watermarkColorPopup->isVisibleTo(this)
        ? ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3)
        : QColor();
    return options;
}

bool LocationsExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

QWidget* LocationsExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* LocationsExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

void LocationsExportDialog::updateTranslations()
{
    setTitle(tr("Export locations"));

    d->selectAllLocations->setText(tr("Select all"));
    d->fileFormat->setLabel(tr("Format"));
    d->includeMainPhoto->setText(tr("Include main photo"));
    d->includeStoryRole->setText(tr("Include story role"));
    d->includeOneLineDescription->setText(tr("Include one line description"));
    d->includeLongDescription->setText(tr("Include long description"));
    d->watermark->setLabel(tr("Watermark"));

    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void LocationsExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(topLevelWidget()->width() * 0.7);

    d->locationsContainer->setBackgroundColor(DesignSystem::color().background());

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
             d->selectAllLocations,
             d->includeMainPhoto,
             d->includeStoryRole,
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
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0, Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px12())
                                             .toMargins());
}

} // namespace Ui
