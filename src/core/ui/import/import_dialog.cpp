#include "import_dialog.h"

#include <business_layer/import/screenplay/screenplay_import_options.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/extension_helper.h>

#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QSettings>
#include <QStringListModel>


namespace Ui {

namespace {
const QString kGroupKey = "widgets/import-dialog/";
const QString kDocumentType = kGroupKey + "document-type";
const QString kImportCharacters = kGroupKey + "import-characters";
const QString kImportLocations = kGroupKey + "import-locations";
const QString kImportText = kGroupKey + "import-text";
const QString kKeepSceneNumbers = kGroupKey + "keep-scene-numbers";
} // namespace

class ImportDialog::Implementation
{
public:
    Implementation(const QString& _importFilePath, QWidget* _parent);

    /**
     * @brief Задать типы документов, в которые можем импортировать
     */
    void setImportDocumentTypes(const QVector<QPair<QString, Domain::DocumentObjectType>>& _types);

    /**
     * @brief Получить тип документа, в который будем экспортировать
     */
    Domain::DocumentObjectType importInType() const;

    /**
     * @brief Обновить лейбл галочки импортирования текста в зависимости от текущего типа документа
     */
    void updateImportTextLabel();


    const QString importFilePath;

    ComboBox* documentType = nullptr;
    OverlineLabel* documentsTitle = nullptr;
    CheckBox* importCharacters = nullptr;
    CheckBox* importLocations = nullptr;
    OverlineLabel* textTitle = nullptr;
    CheckBox* importText = nullptr;
    CheckBox* keepSceneNumbers = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* importButton = nullptr;

    QVector<Domain::DocumentObjectType> documentTypes;
};


ImportDialog::Implementation::Implementation(const QString& _importFilePath, QWidget* _parent)
    : importFilePath(_importFilePath)
    , documentType(new ComboBox(_parent))
    , documentsTitle(new OverlineLabel(_parent))
    , importCharacters(new CheckBox(_parent))
    , importLocations(new CheckBox(_parent))
    , textTitle(new OverlineLabel(_parent))
    , importText(new CheckBox(_parent))
    , keepSceneNumbers(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , importButton(new Button(_parent))
{
    documentType->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    for (auto checkBox : { importCharacters, importLocations, importText, keepSceneNumbers }) {
        checkBox->setChecked(true);
    }

    keepSceneNumbers->setChecked(false);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(importButton);
}

void ImportDialog::Implementation::setImportDocumentTypes(
    const QVector<QPair<QString, Domain::DocumentObjectType>>& _types)
{
    QStringList list;
    for (const auto& type : _types) {
        list.append(type.first);
        documentTypes.append(type.second);
    }
    auto documentTypesModel = new QStringListModel(list);
    documentType->setModel(documentTypesModel);

    QSettings settings;
    if (const int index = list.indexOf(settings.value(kDocumentType, "").toString()); index >= 0) {
        documentType->setCurrentIndex(documentType->model()->index(index, 0));
    } else {
        documentType->setCurrentIndex(documentType->model()->index(0, 0));
    }
}

Domain::DocumentObjectType ImportDialog::Implementation::importInType() const
{
    return documentTypes.value(documentType->currentIndex().row(),
                               Domain::DocumentObjectType::Undefined);
}

void ImportDialog::Implementation::updateImportTextLabel()
{
    switch (importInType()) {
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::Screenplay:
    case Domain::DocumentObjectType::Stageplay: {
        importText->setText(tr("Import script text"));
        break;
    }
    case Domain::DocumentObjectType::Novel: {
        importText->setText(tr("Import novel text"));
        break;
    }
    default: {
        importText->setText(tr("Import text"));
        break;
    }
    }
}


// ****


ImportDialog::ImportDialog(const QString& _importFilePath, QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(_importFilePath, this))
{
    setAcceptButton(d->importButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->documentType, row++, 0);
    contentsLayout()->addWidget(d->documentsTitle, row++, 0);
    contentsLayout()->addWidget(d->importCharacters, row++, 0);
    contentsLayout()->addWidget(d->importLocations, row++, 0);
    contentsLayout()->addWidget(d->textTitle, row++, 0);
    contentsLayout()->addWidget(d->importText, row++, 0);
    contentsLayout()->addWidget(d->keepSceneNumbers, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    auto configureImportAvailability = [this] {
        d->importButton->setEnabled(d->importCharacters->isChecked()
                                    || d->importLocations->isChecked()
                                    || d->importText->isChecked());
    };
    connect(d->importCharacters, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importLocations, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importText, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importText, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (d->importInType() == Domain::DocumentObjectType::Screenplay) {
            d->keepSceneNumbers->setVisible(_checked);
        }
    });
    connect(d->importButton, &Button::clicked, this, &ImportDialog::importRequested);
    connect(d->cancelButton, &Button::clicked, this, &ImportDialog::canceled);

    auto updateParameters = [this] {
        auto isImportCharactersVisible = true;
        auto isImportLocationsVisible = true;
        auto isKeepSceneNumbersVisible = true;
        switch (d->importInType()) {
        case Domain::DocumentObjectType::Audioplay:
        case Domain::DocumentObjectType::ComicBook:
        case Domain::DocumentObjectType::Stageplay: {
            isImportLocationsVisible = false;
            isKeepSceneNumbersVisible = false;
            break;
        }
        case Domain::DocumentObjectType::Novel: {
            isImportCharactersVisible = false;
            isImportLocationsVisible = false;
            isKeepSceneNumbersVisible = false;
            break;
        }
        case Domain::DocumentObjectType::Screenplay: {
            //
            // ... всё видимое
            //
            break;
        }
        default: {
            isImportLocationsVisible = false;
            isKeepSceneNumbersVisible = false;
            break;
        }
        }
        d->importCharacters->setVisible(isImportCharactersVisible);
        d->importLocations->setVisible(isImportLocationsVisible);
        d->keepSceneNumbers->setVisible(isKeepSceneNumbersVisible);
        d->documentsTitle->setVisible(isImportCharactersVisible || isImportLocationsVisible);

        d->updateImportTextLabel();
    };

    connect(d->documentType, &ComboBox::currentIndexChanged, this, updateParameters);

    QSettings settings;
    d->importCharacters->setChecked(settings.value(kImportCharacters, true).toBool());
    d->importLocations->setChecked(settings.value(kImportLocations, true).toBool());
    d->importText->setChecked(settings.value(kImportText, true).toBool());
    d->keepSceneNumbers->setChecked(settings.value(kKeepSceneNumbers, false).toBool());
}

ImportDialog::~ImportDialog()
{
    QSettings settings;
    settings.setValue(kDocumentType, d->documentType->currentText());
    settings.setValue(kImportCharacters, d->importCharacters->isChecked());
    settings.setValue(kImportLocations, d->importLocations->isChecked());
    settings.setValue(kImportText, d->importText->isChecked());
    settings.setValue(kKeepSceneNumbers, d->keepSceneNumbers->isChecked());
}

BusinessLayer::ImportOptions ImportDialog::importOptions() const
{
    BusinessLayer::ImportOptions options;
    options.filePath = d->importFilePath;
    options.documentType = d->importInType();
    options.importText = d->importText->isChecked();

    options.importCharacters
        = d->importCharacters->isVisibleTo(this) && d->importCharacters->isChecked();
    options.importLocations
        = d->importLocations->isVisibleTo(this) && d->importLocations->isChecked();
    return options;
}

BusinessLayer::ScreenplayImportOptions ImportDialog::screenplayImportOptions() const
{
    BusinessLayer::ScreenplayImportOptions options;
    options.filePath = d->importFilePath;
    options.documentType = d->importInType();
    options.importText = d->importText->isChecked();

    options.importCharacters
        = d->importCharacters->isVisibleTo(this) && d->importCharacters->isChecked();
    options.importLocations
        = d->importLocations->isVisibleTo(this) && d->importLocations->isChecked();
    options.keepSceneNumbers
        = d->keepSceneNumbers->isVisibleTo(this) && d->keepSceneNumbers->isChecked();
    return options;
}

QWidget* ImportDialog::focusedWidgetAfterShow() const
{
    return d->importCharacters;
}

QWidget* ImportDialog::lastFocusableWidget() const
{
    return d->importButton;
}

void ImportDialog::updateTranslations()
{
    const QFileInfo importFileInfo(d->importFilePath);
    setTitle(QString("%1 \"%2\"").arg(tr("Import data from the file"), importFileInfo.fileName()));

    d->documentsTitle->setText(tr("Documents"));
    d->importCharacters->setText(tr("Import characters"));
    d->importLocations->setText(tr("Import locations"));
    d->textTitle->setText(tr("Text"));
    d->updateImportTextLabel();
    d->keepSceneNumbers->setText(tr("Keep scene numbers"));

    d->importButton->setText(tr("Import"));
    d->cancelButton->setText(tr("Cancel"));

    auto fileIs = [filePath = d->importFilePath.toLower()](const QString& _extension) {
        return filePath.endsWith(_extension);
    };

    const auto audioplay = qMakePair(tr("Audioplay"), Domain::DocumentObjectType::Audioplay);
    const auto comicBook = qMakePair(tr("Comic Book"), Domain::DocumentObjectType::ComicBook);
    const auto novel = qMakePair(tr("Novel"), Domain::DocumentObjectType::Novel);
    const auto screenplay = qMakePair(tr("Screenplay"), Domain::DocumentObjectType::Screenplay);
    const auto stageplay = qMakePair(tr("Stageplay"), Domain::DocumentObjectType::Stageplay);

    if (fileIs(ExtensionHelper::msOfficeOpenXml()) || fileIs(ExtensionHelper::openDocumentXml())
        || fileIs(ExtensionHelper::plainText())) {
        d->setImportDocumentTypes({ audioplay, comicBook, novel, screenplay, stageplay });
    } else if (fileIs(ExtensionHelper::markdown())) {
        d->setImportDocumentTypes({ novel });
    } else if (fileIs(ExtensionHelper::fountain())) {
        d->setImportDocumentTypes({ audioplay, comicBook, screenplay, stageplay });
    } else if (fileIs(ExtensionHelper::celtx()) || fileIs(ExtensionHelper::finalDraft())
               || fileIs(ExtensionHelper::finalDraftTemplate())
               || fileIs(ExtensionHelper::kitScenarist()) || fileIs(ExtensionHelper::trelby())) {
        d->setImportDocumentTypes({ screenplay });
    }

    d->documentType->setLabel(tr("Import to"));
}

void ImportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);
    for (auto label : { d->documentsTitle, d->textTitle }) {
        label->setContentsMargins(titleMargins);
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox :
         { d->importCharacters, d->importLocations, d->importText, d->keepSceneNumbers }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->importButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    for (auto textField : std::vector<TextField*>{
             d->documentType,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto combobox : {
             d->documentType,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
