#include "characters_export_dialog.h"

#include <business_layer/export/characters/characters_export_options.h>
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
const QString kGroupKey = "widgets/characters-export-dialog/";
const QString kCharactersKey = kGroupKey + "characters";
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

    void updateSelectAllState();


    QScrollArea* charactersScrollArea = nullptr;
    Widget* charactersContainer = nullptr;
    QVBoxLayout* charactersLayout = nullptr;
    CheckBox* selectAllCharacters = nullptr;
    QVector<CheckBox*> characters;
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
    : charactersScrollArea(new QScrollArea(_parent))
    , charactersContainer(new Widget(charactersScrollArea))
    , charactersLayout(new QVBoxLayout)
    , selectAllCharacters(new CheckBox(charactersContainer))
    , fileFormat(new ComboBox(_parent))
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
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    charactersScrollArea->setPalette(palette);
    charactersScrollArea->setFrameShape(QFrame::NoFrame);
    charactersScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    charactersScrollArea->setVerticalScrollBar(new ScrollBar);
    charactersScrollArea->setWidget(charactersContainer);
    charactersScrollArea->setWidgetResizable(true);
    charactersLayout->setContentsMargins({});
    charactersLayout->addWidget(selectAllCharacters);
    charactersLayout->setSpacing(0);
    charactersLayout->addStretch();
    charactersContainer->setLayout(charactersLayout);

    new Shadow(Qt::TopEdge, charactersScrollArea);
    new Shadow(Qt::BottomEdge, charactersScrollArea);


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

void CharactersExportDialog::Implementation::updateSelectAllState()
{
    bool hasChecked = false;
    bool hasUnchecked = false;
    for (const auto characterCheckBox : std::as_const(characters)) {
        if (characterCheckBox->isChecked()) {
            hasChecked = true;
        } else {
            hasUnchecked = true;
        }
    }

    if (hasChecked && hasUnchecked) {
        selectAllCharacters->setIndeterminate();
    } else {
        QSignalBlocker signalBlocker(selectAllCharacters);
        selectAllCharacters->setChecked(hasChecked);
    }
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
    contentsLayout()->addWidget(d->charactersScrollArea, row, column++, 9, 1);
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
    contentsLayout()->addLayout(d->buttonsLayout, row++, column, 1, 2);

    connect(d->selectAllCharacters, &CheckBox::checkedChanged, this,
            [this](bool _checked, bool _indeterminate) {
                if (_indeterminate) {
                    return;
                }

                for (auto characterCheckBox : std::as_const(d->characters)) {
                    QSignalBlocker signalBlocker(characterCheckBox);
                    characterCheckBox->setChecked(_checked);
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
    QVariantMap characters;
    for (const auto characterCheckBox : std::as_const(d->characters)) {
        characters[characterCheckBox->text()] = characterCheckBox->isChecked();
    }
    settings.setValue(kCharactersKey, characters);
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

void CharactersExportDialog::setModel(BusinessLayer::AbstractModel* _model) const
{
    qDeleteAll(d->characters);
    d->characters.clear();

    d->selectAllCharacters->setChecked(true);
    for (int row = 0; row < _model->rowCount(); ++row) {
        auto characterCheckBox = new CheckBox(d->charactersContainer);
        characterCheckBox->setBackgroundColor(DesignSystem::color().background());
        characterCheckBox->setTextColor(DesignSystem::color().onBackground());
        characterCheckBox->setText(_model->index(row, 0).data().toString());
        characterCheckBox->setChecked(true);
        connect(characterCheckBox, &CheckBox::checkedChanged, this,
                [this] { d->updateSelectAllState(); });

        // 1 - чекбокс "Выделить всех"
        d->charactersLayout->insertWidget(1 + d->characters.size(), characterCheckBox);

        d->characters.append(characterCheckBox);
    }

    //
    // Если список персонажей не изменился с прошлого раза, то восстановим значения
    //
    QSettings settings;
    const auto characters = settings.value(kCharactersKey).toMap();
    bool isSame = d->characters.size() == characters.size();
    if (isSame) {
        for (int index = 0; index < d->characters.size(); ++index) {
            if (!characters.contains(d->characters[index]->text())) {
                isSame = false;
                break;
            }
        }
    }
    if (isSame) {
        for (int index = 0; index < d->characters.size(); ++index) {
            d->characters[index]->setChecked(characters[d->characters[index]->text()].toBool());
        }
    }
}

BusinessLayer::CharactersExportOptions CharactersExportDialog::exportOptions() const
{
    BusinessLayer::CharactersExportOptions options;
    options.characters.clear();
    for (const auto characterCheckBox : std::as_const(d->characters)) {
        if (characterCheckBox->isChecked()) {
            options.characters.append(characterCheckBox->text());
        }
    }
    options.fileFormat
        = static_cast<BusinessLayer::ExportFileFormat>(d->fileFormat->currentIndex().row());
    options.includeMainPhoto
        = d->includeMainPhoto->isVisibleTo(this) && d->includeMainPhoto->isChecked();
    options.includeStoryRole
        = d->includeStoryRole->isVisibleTo(this) && d->includeStoryRole->isChecked();
    options.includeAge = d->includeAge->isVisibleTo(this) && d->includeAge->isChecked();
    options.includeGender = d->includeGender->isVisibleTo(this) && d->includeGender->isChecked();
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

    d->selectAllCharacters->setText(tr("Select all"));
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

    d->charactersContainer->setBackgroundColor(DesignSystem::color().background());

    auto titleMargins = DesignSystem::label().margins().toMargins();
    titleMargins.setTop(DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    for (auto textField : std::vector<TextField*>{
             d->fileFormat,
             d->watermark,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->fileFormat,
         }) {
        combobox->setPopupBackgroundColor(DesignSystem::color().background());
    }

    for (auto checkBox : {
             d->selectAllCharacters,
             d->includeMainPhoto,
             d->includeStoryRole,
             d->includeAge,
             d->includeGender,
             d->includeOneLineDescription,
             d->includeLongDescription,
             d->openDocumentAfterExport,
         }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto button : { d->exportButton, d->cancelButton }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    d->watermarkColorPopup->setBackgroundColor(DesignSystem::color().background());
    d->watermarkColorPopup->setTextColor(DesignSystem::color().onBackground());

    contentsLayout()->setSpacing(static_cast<int>(DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(0.0, DesignSystem::layout().px12(),
                                                   DesignSystem::layout().px16(),
                                                   DesignSystem::layout().px12())
                                             .toMargins());
}

} // namespace Ui
