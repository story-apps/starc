#include "abstract_export_dialog.h"

#include <business_layer/export/export_options.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QHBoxLayout>
#include <QSettings>
#include <QStringListModel>

namespace Ui {

namespace {
const QString kGroupKey = "widgets/export-dialog";
const QString kWatermarkKey = "watermark";
const QString kWatermarkColorKey = "watermark-color";
const QString kOpenDocumentAfterExportKey = "open-document-after-export";
} // namespace

class AbstractExportDialog::Implementation
{
public:
    explicit Implementation(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                            const QString& _uuidKey, QWidget* _parent);

    /**
     * @brief Получить список форматов для экспорта
     */
    QStringList formatsList() const;

    /**
     * @brief Ключ для сохранения настроек конкретного документа
     */
    const QString uuidKey;
    QVector<BusinessLayer::ExportFileFormat> formats;

    QVBoxLayout* leftLayout = nullptr;

    QVBoxLayout* rightLayout = nullptr;
    ComboBox* fileFormat = nullptr;
    TextField* watermark = nullptr;
    ColorPickerPopup* watermarkColorPopup = nullptr;

    QHBoxLayout* bottomLayout = nullptr;
    CheckBox* openDocumentAfterExport = nullptr;
    Button* cancelButton = nullptr;
    Button* exportButton = nullptr;

    BusinessLayer::ExportOptions options;
};

AbstractExportDialog::Implementation::Implementation(
    const QVector<BusinessLayer::ExportFileFormat>& _formats, const QString& _uuidKey,
    QWidget* _parent)
    : uuidKey(_uuidKey)
    , formats(_formats)
    , leftLayout(new QVBoxLayout)
    , rightLayout(new QVBoxLayout)
    , fileFormat(new ComboBox(_parent))
    , watermark(new TextField(_parent))
    , watermarkColorPopup(new ColorPickerPopup(_parent))
    , bottomLayout(new QHBoxLayout)
    , openDocumentAfterExport(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , exportButton(new Button(_parent))
{
    fileFormat->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    auto formatsModel = new QStringListModel(formatsList());
    fileFormat->setModel(formatsModel);
    fileFormat->setCurrentIndex(formatsModel->index(0, 0));

    watermark->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    watermark->setTrailingIcon(u8"\U000F0765");
    watermarkColorPopup->setColorCanBeDeselected(false);

    leftLayout->setContentsMargins({});
    leftLayout->setSpacing(0);

    rightLayout->addWidget(fileFormat);
    rightLayout->addWidget(watermark);
    rightLayout->addStretch();

    bottomLayout->setContentsMargins({});
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(openDocumentAfterExport, 0, Qt::AlignVCenter);
    bottomLayout->addStretch();
    bottomLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    bottomLayout->addWidget(exportButton, 0, Qt::AlignVCenter);
}

QStringList AbstractExportDialog::Implementation::formatsList() const
{
    QStringList formatsList;
    for (const auto& format : formats) {
        switch (format) {
        case BusinessLayer::ExportFileFormat::Docx: {
            formatsList.append("DOCX");
            break;
        }
        case BusinessLayer::ExportFileFormat::Fdx: {
            formatsList.append("FDX");
            break;
        }
        case BusinessLayer::ExportFileFormat::Fountain: {
            formatsList.append("Fountain");
            break;
        }
        case BusinessLayer::ExportFileFormat::Markdown: {
            formatsList.append("Markdown");
            break;
        }
        case BusinessLayer::ExportFileFormat::Pdf: {
            formatsList.append("PDF");
            break;
        }
        }
    }
    return formatsList;
}


// ****


AbstractExportDialog::AbstractExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                           const QString& _uuidKey, QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(_formats, _uuidKey, this))
{
    setAcceptButton(d->exportButton);
    setRejectButton(d->cancelButton);

    QSettings settings;
    d->watermark->setText(settings.value(settingsKey(kWatermarkKey)).toString());
    d->watermarkColorPopup->setSelectedColor(
        settings.value(settingsKey(kWatermarkColorKey), QColor("#B7B7B7")).value<QColor>());
    d->watermark->setTrailingIconColor(d->watermarkColorPopup->selectedColor());
    d->openDocumentAfterExport->setChecked(
        settings.value(settingsKey(kOpenDocumentAfterExportKey), true).toBool());

    connect(d->exportButton, &Button::clicked, this, &AbstractExportDialog::exportRequested);
    connect(d->cancelButton, &Button::clicked, this, &AbstractExportDialog::canceled);

    connect(d->fileFormat, &ComboBox::currentIndexChanged, this,
            &AbstractExportDialog::updateDialog);
    connect(d->watermark, &TextField::trailingIconPressed, this, [this] {
        d->watermarkColorPopup->showPopup(d->watermark, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->watermarkColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->watermark->setTrailingIconColor(_color); });
}

AbstractExportDialog::~AbstractExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kWatermarkKey), d->watermark->text());
    settings.setValue(settingsKey(kWatermarkColorKey), d->watermarkColorPopup->selectedColor());
    settings.setValue(settingsKey(kOpenDocumentAfterExportKey),
                      d->openDocumentAfterExport->isChecked());
}

bool AbstractExportDialog::openDocumentAfterExport() const
{
    return d->openDocumentAfterExport->isChecked();
}

BusinessLayer::ExportOptions& AbstractExportDialog::exportOptions() const
{
    d->options.fileFormat = currentFileFormat();
    d->options.watermark = d->watermark->isVisibleTo(this) ? d->watermark->text() : "";
    d->options.watermarkColor = d->watermark->isVisibleTo(this)
        ? ColorHelper::transparent(d->watermarkColorPopup->selectedColor(), 0.3)
        : QColor();
    return d->options;
}

void AbstractExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(topLevelWidget()->width() * 0.7);

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

    d->fileFormat->setPopupBackgroundColor(DesignSystem::color().background());

    d->watermarkColorPopup->setBackgroundColor(DesignSystem::color().background());
    d->watermarkColorPopup->setTextColor(DesignSystem::color().onBackground());

    d->openDocumentAfterExport->setBackgroundColor(DesignSystem::color().background());
    d->openDocumentAfterExport->setTextColor(DesignSystem::color().onBackground());

    UiHelper::initColorsFor(d->exportButton, UiHelper::DialogAccept);
    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);

    contentsLayout()->setSpacing(static_cast<int>(DesignSystem::layout().px8()));
    d->bottomLayout->setContentsMargins(QMarginsF(0.0, DesignSystem::layout().px24(),
                                                  DesignSystem::layout().px16(),
                                                  DesignSystem::layout().px12())
                                            .toMargins());
}

void AbstractExportDialog::updateTranslations()
{
    d->fileFormat->setLabel(tr("Format"));
    d->watermark->setLabel(tr("Watermark"));
    d->openDocumentAfterExport->setText(tr("Open document after export"));
    d->exportButton->setText(tr("Export"));
    d->cancelButton->setText(tr("Cancel"));
}

void AbstractExportDialog::updateDialog() const
{
    updateParametersVisibility();
    updateExportEnabled();
}

void AbstractExportDialog::updateExportEnabled() const
{
    d->exportButton->setEnabled(isExportEnabled());
}

bool AbstractExportDialog::isExportEnabled() const
{
    return true;
}

BusinessLayer::ExportFileFormat AbstractExportDialog::currentFileFormat() const
{
    return d->formats.at(currentFileFormatRow());
}

int AbstractExportDialog::currentFileFormatRow() const
{
    return d->fileFormat->currentIndex().row();
}

void AbstractExportDialog::setCurrentFileFormat(int _row) const
{
    const auto fileFormatIndex = d->fileFormat->model()->index(_row, 0);
    d->fileFormat->setCurrentIndex(fileFormatIndex);
}

void AbstractExportDialog::setWatermarkVisible(bool _isVisible) const
{
    d->watermark->setVisible(_isVisible);
}

QVBoxLayout* AbstractExportDialog::leftLayout() const
{
    return d->leftLayout;
}

QVBoxLayout* AbstractExportDialog::rightLayout() const
{
    return d->rightLayout;
}

QHBoxLayout* AbstractExportDialog::bottomLayout() const
{
    return d->bottomLayout;
}

QWidget* AbstractExportDialog::focusedWidgetAfterShow() const
{
    return d->fileFormat;
}

QWidget* AbstractExportDialog::lastFocusableWidget() const
{
    return d->exportButton;
}

QString AbstractExportDialog::settingsKey(const QString& _parameter) const
{
    return QString("%1/%2/%3").arg(kGroupKey, d->uuidKey, _parameter);
}

} // namespace Ui
