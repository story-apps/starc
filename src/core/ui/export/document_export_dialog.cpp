#include "document_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>

#include <QHBoxLayout>
#include <QSettings>

namespace Ui {

namespace {
const QString kIncludeMainPhotoKey = "include-main-photo";
const QString kIncludeAdditionalPhototsKey = "include-additional-photos";
} // namespace

class DocumentExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    CheckBox* includeMainPhoto = nullptr;
    CheckBox* includeAdditionalPhotos = nullptr;

    BusinessLayer::DocumentExportOptions options;
};

DocumentExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeMainPhoto(new CheckBox(_parent))
    , includeAdditionalPhotos(new CheckBox(_parent))
{
}


// ****


DocumentExportDialog::DocumentExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                           const QString& _uuidKey, QWidget* _parent)
    : AbstractExportDialog(_formats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    int row = 1;
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->includeMainPhoto);
        layout->addWidget(d->includeAdditionalPhotos);
        layout->addStretch();
        rightLayout()->insertLayout(row++, layout);
    }

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(rightLayout(), row++, column);
    contentsLayout()->addLayout(bottomLayout(), row, column);

    QSettings settings;
    d->includeMainPhoto->setChecked(
        settings.value(settingsKey(kIncludeMainPhotoKey), true).toBool());
    d->includeAdditionalPhotos->setChecked(
        settings.value(settingsKey(kIncludeAdditionalPhototsKey), false).toBool());
}

DocumentExportDialog::~DocumentExportDialog()
{
    QSettings settings;
    settings.setValue(settingsKey(kIncludeMainPhotoKey), d->includeMainPhoto->isChecked());
    settings.setValue(settingsKey(kIncludeAdditionalPhototsKey),
                      d->includeAdditionalPhotos->isChecked());
}

BusinessLayer::DocumentExportOptions& DocumentExportDialog::exportOptions() const
{
    d->options = AbstractExportDialog::exportOptions();
    d->options.includeMainPhoto = d->includeMainPhoto->isChecked();
    d->options.includeAdditionalPhotos = d->includeAdditionalPhotos->isChecked();
    return d->options;
}

void DocumentExportDialog::updateTranslations()
{
    AbstractExportDialog::updateTranslations();

    d->includeMainPhoto->setText(tr("Include main photo"));
    d->includeAdditionalPhotos->setText(tr("additional photos"));
}

void DocumentExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractExportDialog::designSystemChangeEvent(_event);

    for (auto checkBox : {
             d->includeMainPhoto,
             d->includeAdditionalPhotos,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void DocumentExportDialog::updateParametersVisibility() const
{
    auto isPhotoVisible = true;
    auto isWatermarkVisible = true;
    switch (currentFileFormat()) {
    default:
    case BusinessLayer::ExportFileFormat::Pdf: {
        //
        // ... всё видимое
        //
        break;
    }
    case BusinessLayer::ExportFileFormat::Docx: {
        isPhotoVisible = false;
        isWatermarkVisible = false;
        break;
    }
    }
    d->includeMainPhoto->setVisible(isPhotoVisible);
    d->includeAdditionalPhotos->setVisible(isPhotoVisible);
    setWatermarkVisible(isWatermarkVisible);
}

} // namespace Ui
