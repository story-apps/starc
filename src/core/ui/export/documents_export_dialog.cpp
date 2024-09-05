#include "documents_export_dialog.h"

#include <business_layer/model/abstract_model.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>

#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

namespace Ui {

namespace {
const QString kIncludeMainPhotoKey = "include-main-photo";
const QString kIncludeStoryRoleKey = "include-story-role";
const QString kIncludeOneLineDescriptionKey = "include-one-line-description";
const QString kIncludeLongDescriptionKey = "include-long-description";
} // namespace

class DocumentsExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateSelectAllState();


    QScrollArea* documentsScrollArea = nullptr;
    Widget* documentsContainer = nullptr;
    QVBoxLayout* documentsLayout = nullptr;
    CheckBox* selectAllDocuments = nullptr;
    QVector<CheckBox*> documents;
    CheckBox* includeMainPhoto = nullptr;
    CheckBox* includeStoryRole = nullptr;
    CheckBox* includeOneLineDescription = nullptr;
    CheckBox* includeLongDescription = nullptr;

    BusinessLayer::DocumentsExportOptions options;
};

DocumentsExportDialog::Implementation::Implementation(QWidget* _parent)
    : documentsScrollArea(new QScrollArea(_parent))
    , documentsContainer(new Widget(documentsScrollArea))
    , documentsLayout(new QVBoxLayout)
    , selectAllDocuments(new CheckBox(documentsContainer))
    , includeMainPhoto(new CheckBox(_parent))
    , includeStoryRole(new CheckBox(_parent))
    , includeOneLineDescription(new CheckBox(_parent))
    , includeLongDescription(new CheckBox(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    documentsScrollArea->setPalette(palette);
    documentsScrollArea->setFrameShape(QFrame::NoFrame);
    documentsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    documentsScrollArea->setVerticalScrollBar(new ScrollBar);
    documentsScrollArea->setWidget(documentsContainer);
    documentsScrollArea->setWidgetResizable(true);
    documentsLayout->setContentsMargins({});
    documentsLayout->addWidget(selectAllDocuments);
    documentsLayout->setSpacing(0);
    documentsLayout->addStretch();
    documentsContainer->setLayout(documentsLayout);

    new Shadow(Qt::TopEdge, documentsScrollArea);
    new Shadow(Qt::BottomEdge, documentsScrollArea);
}

void DocumentsExportDialog::Implementation::updateSelectAllState()
{
    bool hasChecked = false;
    bool hasUnchecked = false;
    for (const auto locationCheckBox : std::as_const(documents)) {
        if (locationCheckBox->isChecked()) {
            hasChecked = true;
        } else {
            hasUnchecked = true;
        }
    }

    if (hasChecked && hasUnchecked) {
        selectAllDocuments->setIndeterminate();
    } else {
        selectAllDocuments->setChecked(hasChecked);
    }
}


// ****


DocumentsExportDialog::DocumentsExportDialog(
    const QVector<BusinessLayer::ExportFileFormat>& _formats, const QString& _uuidKey,
    QWidget* _parent)
    : AbstractExportDialog(_formats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    leftLayout()->addWidget(d->documentsScrollArea);

    int row = 1;
    rightLayout()->insertWidget(row++, d->includeMainPhoto);
    rightLayout()->insertWidget(row++, d->includeStoryRole);
    rightLayout()->insertWidget(row++, d->includeOneLineDescription);
    rightLayout()->insertWidget(row++, d->includeLongDescription);

    row = 0;
    int column = 0;
    contentsLayout()->addLayout(leftLayout(), row, column++);
    contentsLayout()->addLayout(rightLayout(), row, column);

    row = 1;
    column = 0;
    contentsLayout()->addLayout(bottomLayout(), row++, column, 1, 2);

    connect(d->selectAllDocuments, &CheckBox::checkedChanged, this,
            [this](bool _checked, bool _indeterminate) {
                if (_indeterminate) {
                    return;
                }

                for (auto locationCheckBox : std::as_const(d->documents)) {
                    QSignalBlocker signalBlocker(locationCheckBox);
                    locationCheckBox->setChecked(_checked);
                }
            });
    connect(d->selectAllDocuments, &CheckBox::checkedChanged, this,
            &DocumentsExportDialog::updateExportEnabled);

    QSettings settings;
    d->includeMainPhoto->setChecked(settings.value(uniqueKey(kIncludeMainPhotoKey), true).toBool());
    d->includeStoryRole->setChecked(settings.value(uniqueKey(kIncludeStoryRoleKey), true).toBool());
    d->includeOneLineDescription->setChecked(
        settings.value(uniqueKey(kIncludeOneLineDescriptionKey), true).toBool());
    d->includeLongDescription->setChecked(
        settings.value(uniqueKey(kIncludeLongDescriptionKey), true).toBool());

    updateDialog();
}

DocumentsExportDialog::~DocumentsExportDialog()
{
    QSettings settings;
    QVariantMap documents;
    for (const auto documentsCheckBox : std::as_const(d->documents)) {
        documents[documentsCheckBox->text()] = documentsCheckBox->isChecked();
    }
    settings.setValue(uniqueKey(kIncludeMainPhotoKey), d->includeMainPhoto->isChecked());
    settings.setValue(uniqueKey(kIncludeStoryRoleKey), d->includeStoryRole->isChecked());
    settings.setValue(uniqueKey(kIncludeOneLineDescriptionKey),
                      d->includeOneLineDescription->isChecked());
    settings.setValue(uniqueKey(kIncludeLongDescriptionKey),
                      d->includeLongDescription->isChecked());
}

BusinessLayer::DocumentsExportOptions& DocumentsExportDialog::exportOptions() const
{
    d->options = AbstractExportDialog::exportOptions();
    d->options.documents.clear();
    for (const auto documentCheckBox : std::as_const(d->documents)) {
        if (documentCheckBox->isChecked()) {
            d->options.documents.append(documentCheckBox->text());
        }
    }
    d->options.includeMainPhoto
        = d->includeMainPhoto->isVisibleTo(this) && d->includeMainPhoto->isChecked();
    d->options.includeStoryRole
        = d->includeStoryRole->isVisibleTo(this) && d->includeStoryRole->isChecked();
    d->options.includeOneLineDescription = d->includeOneLineDescription->isVisibleTo(this)
        && d->includeOneLineDescription->isChecked();
    d->options.includeLongDescription
        = d->includeLongDescription->isVisibleTo(this) && d->includeLongDescription->isChecked();
    return d->options;
}

void DocumentsExportDialog::setModel(BusinessLayer::AbstractModel* _model) const
{
    qDeleteAll(d->documents);
    d->documents.clear();

    d->selectAllDocuments->setChecked(true);
    for (int row = 0; row < _model->rowCount(); ++row) {
        auto locationCheckBox = new CheckBox(d->documentsContainer);
        locationCheckBox->setBackgroundColor(Ui::DesignSystem::color().background());
        locationCheckBox->setTextColor(Ui::DesignSystem::color().onBackground());
        locationCheckBox->setText(_model->index(row, 0).data().toString());
        locationCheckBox->setChecked(true);
        connect(locationCheckBox, &CheckBox::checkedChanged, this,
                [this] { d->updateSelectAllState(); });

        // 1 - чекбокс "Выделить всех"
        d->documentsLayout->insertWidget(1 + d->documents.size(), locationCheckBox);

        d->documents.append(locationCheckBox);
    }

    //
    // Если список документов не изменился с прошлого раза, то восстановим значения
    //
    QSettings settings;
    const auto documents = settings.value(documentsKey()).toMap();
    bool isSame = d->documents.size() == documents.size();
    if (isSame) {
        for (int index = 0; index < d->documents.size(); ++index) {
            if (!documents.contains(d->documents[index]->text())) {
                isSame = false;
                break;
            }
        }
    }
    if (isSame) {
        for (int index = 0; index < d->documents.size(); ++index) {
            d->documents[index]->setChecked(documents[d->documents[index]->text()].toBool());
        }
    }
    updateExportEnabled();
}

void DocumentsExportDialog::updateTranslations()
{
    AbstractExportDialog::updateTranslations();

    d->selectAllDocuments->setText(tr("Select all"));
    d->includeMainPhoto->setText(tr("Include main photo"));
    d->includeStoryRole->setText(tr("Include story role"));
    d->includeOneLineDescription->setText(tr("Include one line description"));
    d->includeLongDescription->setText(tr("Include long description"));
}

void DocumentsExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractExportDialog::designSystemChangeEvent(_event);

    d->documentsContainer->setBackgroundColor(DesignSystem::color().background());

    for (auto checkBox : {
             d->selectAllDocuments,
             d->includeMainPhoto,
             d->includeStoryRole,
             d->includeOneLineDescription,
             d->includeLongDescription,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void DocumentsExportDialog::updateParametersVisibility() const
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
    setWatermarkVisible(isWatermarkVisible);
}

bool DocumentsExportDialog::isExportEnabled() const
{
    bool exportEnabled = false;
    for (const auto locationCheckBox : std::as_const(d->documents)) {
        if (locationCheckBox->isChecked()) {
            exportEnabled = true;
            break;
        }
    }
    return exportEnabled;
}

QVariantMap DocumentsExportDialog::checkedDocuments() const
{
    QVariantMap documents;
    for (const auto documentCheckBox : std::as_const(d->documents)) {
        documents[documentCheckBox->text()] = documentCheckBox->isChecked();
    }
    return documents;
}

} // namespace Ui
