#include "import_dialog.h"

#include <business_layer/import/import_options.h>
#include <ui/design_system/design_system.h>
#include <ui/import/import_file_delegate.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <utils/helpers/extension_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QApplication>
#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringListModel>


namespace Ui {

namespace {
const QString kGroupKey = "widgets/import-dialog/";
const QString kDocumentType = "document-type";
const QString kImportCharacters = "import-characters";
const QString kImportLocations = "import-locations";
const QString kImportResearch = "import-research";
const QString kImportText = "import-text";
const QString kKeepSceneNumbers = "keep-scene-numbers";

QString settingsKey(const QString& _filePath, const QString& _parameter)
{
    return QString("%1/%2/%3").arg(kGroupKey, _filePath, _parameter);
}

const QVector<QPair<const Domain::DocumentObjectType, const QString>> kDocumentTypes()
{
    return {
        {
            Domain::DocumentObjectType::SimpleText,
            QApplication::translate("Ui::ImportDialog", "Simple text"),
        },
        {
            Domain::DocumentObjectType::Audioplay,
            QApplication::translate("Ui::ImportDialog", "Audioplay"),
        },
        {
            Domain::DocumentObjectType::ComicBook,
            QApplication::translate("Ui::ImportDialog", "Comic Book"),
        },
        {
            Domain::DocumentObjectType::Novel,
            QApplication::translate("Ui::ImportDialog", "Novel"),
        },
        {
            Domain::DocumentObjectType::Screenplay,
            QApplication::translate("Ui::ImportDialog", "Screenplay"),
        },
        {
            Domain::DocumentObjectType::Stageplay,
            QApplication::translate("Ui::ImportDialog", "Stageplay"),
        },
        {
            Domain::DocumentObjectType::Presentation,
            QApplication::translate("Ui::ImportDialog", "Presentation"),
        },
    };
}

QString typeToString(const Domain::DocumentObjectType& _type)
{
    for (const auto& type : kDocumentTypes()) {
        if (type.first == _type) {
            return type.second;
        }
    }
    return "";
}

Domain::DocumentObjectType stringToType(const QString& _type)
{
    for (const auto& type : kDocumentTypes()) {
        if (type.second == _type) {
            return type.first;
        }
    }
    return Domain::DocumentObjectType::Undefined;
}

QVector<Domain::DocumentObjectType> importTypesForFile(const QString& _path)
{
    auto fileIs = [filePath = _path.toLower()](const QString& _extension) {
        return filePath.endsWith(_extension);
    };

    using namespace Domain;

    //
    // Бинарные форматы
    //
    if (fileIs(ExtensionHelper::msOfficeOpenXml()) || fileIs(ExtensionHelper::openDocumentXml())) {
        return {
            DocumentObjectType::SimpleText,
            DocumentObjectType::Screenplay,
        };
    } else if (fileIs(ExtensionHelper::pdf())) {
        return {
            DocumentObjectType::SimpleText,
            DocumentObjectType::Screenplay,
            DocumentObjectType::Presentation,
        };
    }
    //
    // Текстовые форматы
    //
    else if (fileIs(ExtensionHelper::fountain())) {
        return {
            DocumentObjectType::SimpleText, DocumentObjectType::Audioplay,
            DocumentObjectType::ComicBook,  DocumentObjectType::Screenplay,
            DocumentObjectType::Stageplay,
        };
    } else if (fileIs(ExtensionHelper::markdown())) {
        return {
            DocumentObjectType::SimpleText,
            DocumentObjectType::Novel,
        };
    } else if (fileIs(ExtensionHelper::plainText())) {
        return {
            DocumentObjectType::SimpleText, DocumentObjectType::Audioplay,
            DocumentObjectType::ComicBook,  DocumentObjectType::Screenplay,
            DocumentObjectType::Stageplay,  DocumentObjectType::Novel,
        };
    }
    //
    // Специализированные форматы
    //
    else if (fileIs(ExtensionHelper::celtx()) || fileIs(ExtensionHelper::finalDraft())
             || fileIs(ExtensionHelper::finalDraftTemplate())
             || fileIs(ExtensionHelper::kitScenarist()) || fileIs(ExtensionHelper::trelby())) {
        return {
            DocumentObjectType::Screenplay,
        };
    }
    return {
        DocumentObjectType::Undefined,
    };
}

} // namespace

class ImportDialog::Implementation
{
public:
    Implementation(const QStringList& _importFilePaths, ImportDialog* _parent);

    /**
     * @brief Обновить ширину диалога
     */
    void updateDialogWidth();

    /**
     * @brief Текущие опции импорта
     */
    BusinessLayer::ImportOptions currentOptions() const;

    /**
     * @brief Записать текущие настройки импорта в список опций
     */
    void writeCurrentOptions();

    /**
     * @brief Текущий тип документа, в который будет импортирован файл
     */
    Domain::DocumentObjectType currentType() const;

    /**
     * @brief Задать типы документов, в которые можем импортировать
     */
    void setImportDocumentTypes(const QVector<Domain::DocumentObjectType>& _types);

    /**
     * @brief Обновить лейбл галочки импортирования текста в зависимости от текущего типа документа
     */
    void updateImportTextLabel();

    /**
     * @brief Обновить видимость параметров
     */
    void updateParametersVisibility();

    /**
     * @brief Отобразить настройки импорта для файла
     */
    void showOptionsForFile(const QString& _path);

    /**
     * @brief Доступен ли импорт при текущих настройках
     */
    bool isImportAvailable() const;

    /**
     * @brief Может ли файл быть импортирован
     */
    bool fileCanBeImported(const BusinessLayer::ImportOptions& _options) const;

    /**
     * @brief Сохранить настройки импорта для файлов
     */
    void saveSetting();


    ImportDialog* q = nullptr;

    const QStringList importFilePaths;
    OverlineLabel* filesCountTitle = nullptr;
    Tree* tree = nullptr;

    Widget* optionsWidget = nullptr;
    ComboBox* documentType = nullptr;
    OverlineLabel* documentsTitle = nullptr;
    CheckBox* importCharacters = nullptr;
    CheckBox* importLocations = nullptr;
    CheckBox* importResearch = nullptr;
    OverlineLabel* textTitle = nullptr;
    CheckBox* importText = nullptr;
    CheckBox* keepSceneNumbers = nullptr;

    QHBoxLayout* bottomLayout = nullptr;
    CheckBox* sameOptions = nullptr;
    Button* cancelButton = nullptr;
    Button* importButton = nullptr;

    QString currentFile;
    QVector<Domain::DocumentObjectType> documentTypes;

    /**
     * @brief Список опций импорта всех файлов
     */
    QHash<QString, BusinessLayer::ImportOptions> filesOptions;
};

ImportDialog::Implementation::Implementation(const QStringList& _importFilePaths,
                                             ImportDialog* _parent)
    : q(_parent)
    , importFilePaths(_importFilePaths)
    , filesCountTitle(new OverlineLabel(_parent))
    , tree(new Tree(_parent))
    , optionsWidget(new Widget(_parent))
    , documentType(new ComboBox(optionsWidget))
    , documentsTitle(new OverlineLabel(optionsWidget))
    , importCharacters(new CheckBox(optionsWidget))
    , importLocations(new CheckBox(optionsWidget))
    , importResearch(new CheckBox(optionsWidget))
    , textTitle(new OverlineLabel(optionsWidget))
    , importText(new CheckBox(optionsWidget))
    , keepSceneNumbers(new CheckBox(optionsWidget))
    , bottomLayout(new QHBoxLayout)
    , sameOptions(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , importButton(new Button(_parent))
    , currentFile(_importFilePaths.first())
{
    documentType->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    sameOptions->setChecked(false);

    bottomLayout->setContentsMargins({});
    bottomLayout->setSpacing(0);
    if (_importFilePaths.size() > 1) {
        bottomLayout->addWidget(sameOptions);
    }
    bottomLayout->addStretch();
    bottomLayout->addWidget(cancelButton);
    bottomLayout->addWidget(importButton);

    //
    // Заполняем опции импорта файлов
    //
    QSettings settings;
    for (const auto& path : _importFilePaths) {
        BusinessLayer::ImportOptions options;

        const auto documentTypeValue = settings.value(settingsKey(path, kDocumentType));
        if (!documentTypeValue.isNull()) {
            bool ok = false;
            const auto documentType
                = static_cast<Domain::DocumentObjectType>(documentTypeValue.toInt(&ok));
            if (ok && documentType != Domain::DocumentObjectType::Undefined) {
                options.documentType = documentType;
            }
        } else {
            options.documentType = importTypesForFile(path).constFirst();
        }
        options.filePath = path;
        options.importCharacters
            = settings.value(settingsKey(path, kImportCharacters), true).toBool();
        options.importLocations
            = settings.value(settingsKey(path, kImportLocations), true).toBool();
        options.importText = settings.value(settingsKey(path, kImportText), true).toBool();
        options.importResearch = settings.value(settingsKey(path, kImportResearch), true).toBool();
        options.keepSceneNumbers
            = settings.value(settingsKey(path, kKeepSceneNumbers), false).toBool();

        filesOptions[path] = options;
    }
}

void ImportDialog::Implementation::updateDialogWidth()
{
    q->setContentFixedWidth(
        filesOptions.size() > 1
            ? std::max(q->topLevelWidget()->width() * 0.5, DesignSystem::dialog().maximumWidth())
            : DesignSystem::dialog().maximumWidth());
}

BusinessLayer::ImportOptions ImportDialog::Implementation::currentOptions() const
{
    BusinessLayer::ImportOptions options;
    options.filePath = currentFile;
    options.documentType = currentType();
    options.importText = importText->isVisibleTo(optionsWidget) && importText->isChecked();
    options.importCharacters
        = importCharacters->isVisibleTo(optionsWidget) && importCharacters->isChecked();
    options.importLocations
        = importLocations->isVisibleTo(optionsWidget) && importLocations->isChecked();
    options.importResearch
        = importResearch->isVisibleTo(optionsWidget) && importResearch->isChecked();
    options.keepSceneNumbers
        = keepSceneNumbers->isVisibleTo(optionsWidget) && keepSceneNumbers->isChecked();
    return options;
}

void ImportDialog::Implementation::writeCurrentOptions()
{
    filesOptions[currentFile] = currentOptions();
}

Domain::DocumentObjectType ImportDialog::Implementation::currentType() const
{
    return documentTypes.value(documentType->currentIndex().row(),
                               Domain::DocumentObjectType::Undefined);
}

void ImportDialog::Implementation::setImportDocumentTypes(
    const QVector<Domain::DocumentObjectType>& _types)
{
    documentTypes.clear();
    QStringList list;
    for (const auto& type : _types) {
        list.append(typeToString(type));
        documentTypes.append(type);
    }
    delete documentType->model();

    QSignalBlocker signalBlocker(documentType);
    documentType->setModel(new QStringListModel(list));
}

void ImportDialog::Implementation::updateImportTextLabel()
{
    switch (currentType()) {
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
    case Domain::DocumentObjectType::Presentation: {
        importText->setText(tr("Import presentation"));
        break;
    }
    default: {
        importText->setText(tr("Import text"));
        break;
    }
    }
}

void ImportDialog::Implementation::updateParametersVisibility()
{
    auto isImportCharactersVisible = true;
    auto isImportLocationsVisible = true;
    auto isImportResearchVisible = true;
    auto isKeepSceneNumbersVisible = true;
    switch (currentType()) {
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::Stageplay: {
        isImportLocationsVisible = false;
        isImportResearchVisible = false;
        isKeepSceneNumbersVisible = false;
        break;
    }
    case Domain::DocumentObjectType::Screenplay: {
        isKeepSceneNumbersVisible = importText->isChecked();
        break;
    }
    default: {
        isImportCharactersVisible = false;
        isImportLocationsVisible = false;
        isImportResearchVisible = false;
        isKeepSceneNumbersVisible = false;
        break;
    }
    }
    importCharacters->setVisible(isImportCharactersVisible);
    importLocations->setVisible(isImportLocationsVisible);
    importResearch->setVisible(isImportResearchVisible);
    keepSceneNumbers->setVisible(isKeepSceneNumbersVisible);
    documentsTitle->setVisible(isImportCharactersVisible || isImportLocationsVisible);

    updateImportTextLabel();
}

void ImportDialog::Implementation::showOptionsForFile(const QString& _path)
{
    const auto& options = filesOptions[_path];
    const auto type = options.documentType;
    int index = documentTypes.indexOf(type) >= 0 ? documentTypes.indexOf(type) : 0;
    documentType->setCurrentIndex(documentType->model()->index(index, 0));
    updateParametersVisibility();

    importText->setChecked(options.importText);
    importCharacters->setChecked(options.importCharacters);
    importLocations->setChecked(options.importLocations);
    importResearch->setChecked(options.importResearch);
    keepSceneNumbers->setChecked(options.keepSceneNumbers);
}

bool ImportDialog::Implementation::isImportAvailable() const
{
    return (importCharacters->isVisibleTo(optionsWidget) && importCharacters->isChecked())
        || (importLocations->isVisibleTo(optionsWidget) && importLocations->isChecked())
        || (importResearch->isVisibleTo(optionsWidget) && importResearch->isChecked())
        || (importText->isVisibleTo(optionsWidget) && importText->isChecked());
}

bool ImportDialog::Implementation::fileCanBeImported(
    const BusinessLayer::ImportOptions& _options) const
{
    return _options.importCharacters || _options.importLocations || _options.importText
        || _options.importResearch;
}

void ImportDialog::Implementation::saveSetting()
{
    QSettings settings;
    for (const auto& fileOptions : filesOptions) {
        settings.setValue(settingsKey(fileOptions.filePath, kDocumentType),
                          static_cast<int>(fileOptions.documentType));
        settings.setValue(settingsKey(fileOptions.filePath, kImportCharacters),
                          fileOptions.importCharacters);
        settings.setValue(settingsKey(fileOptions.filePath, kImportLocations),
                          fileOptions.importLocations);
        settings.setValue(settingsKey(fileOptions.filePath, kImportText), fileOptions.importText);
        settings.setValue(settingsKey(fileOptions.filePath, kImportResearch),
                          fileOptions.importResearch);
        settings.setValue(settingsKey(fileOptions.filePath, kKeepSceneNumbers),
                          fileOptions.keepSceneNumbers);
    }
}


// ****


ImportDialog::ImportDialog(const QStringList& _importFilePaths, QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(_importFilePaths, this))
{
    setAcceptButton(d->importButton);
    setRejectButton(d->cancelButton);

    int column = 0;
    if (_importFilePaths.size() > 1) {
        QStandardItemModel* model = new QStandardItemModel(d->tree);
        for (const auto& path : _importFilePaths) {
            const auto& options = d->filesOptions[path];
            const auto type = options.documentType;
            auto item = new QStandardItem(path);
            item->setData(typeToString(type), ImportTypeRole);
            item->setData(d->fileCanBeImported(options), ImportEnabledRole);
            model->appendRow(item);
        }
        d->tree->setModel(model);
        d->tree->setItemDelegate(new ImportFileDelegate(d->tree));
        d->tree->setCurrentIndex(d->tree->model()->index(0, 0));

        //
        // Левая сторона
        //
        QVBoxLayout* leftLayout = new QVBoxLayout;
        leftLayout->addWidget(d->filesCountTitle);
        leftLayout->addWidget(d->tree);
        contentsLayout()->addLayout(leftLayout, 0, column++);
    }

    //
    // Правая сторона
    //
    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->setContentsMargins({});
    rightLayout->setSpacing(0);
    rightLayout->addWidget(d->documentType);
    rightLayout->addWidget(d->documentsTitle);
    rightLayout->addWidget(d->importCharacters);
    rightLayout->addWidget(d->importLocations);
    rightLayout->addWidget(d->importResearch);
    rightLayout->addWidget(d->textTitle);
    rightLayout->addWidget(d->importText);
    rightLayout->addWidget(d->keepSceneNumbers);
    rightLayout->addStretch();
    d->optionsWidget->setLayout(rightLayout);
    contentsLayout()->addWidget(d->optionsWidget, 0, column);

    //
    // Низ
    //
    int columnSpan = column + 1;
    contentsLayout()->addLayout(d->bottomLayout, 1, 0, 1, columnSpan);

    //
    // Определить активность кнопки импорта
    //
    auto configureImportAvailability = [this] {
        bool importEnabled = true;
        const auto currentFileImportAvailable = d->isImportAvailable();
        if (d->tree->model()) {
            d->tree->model()->setData(d->tree->currentIndex(), currentFileImportAvailable,
                                      ImportEnabledRole);
        }
        if (d->sameOptions->isVisibleTo(this) && d->sameOptions->isChecked()) {
            importEnabled = currentFileImportAvailable;
        } else if (!currentFileImportAvailable) {
            importEnabled = false;
        } else {
            for (const auto& fileOptions : d->filesOptions) {
                if (!d->fileCanBeImported(fileOptions) && fileOptions.filePath != d->currentFile) {
                    importEnabled = false;
                    break;
                }
            }
        }
        d->importButton->setEnabled(importEnabled);
    };
    configureImportAvailability();

    //
    // Соединения чекбоксов настроек импорта
    //
    connect(d->importCharacters, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importLocations, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importResearch, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importText, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importText, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (d->currentType() == Domain::DocumentObjectType::Screenplay) {
            d->keepSceneNumbers->setVisible(_checked);
        }
    });

    //
    // Передать опции импорта всем файлам
    //
    auto setSameOptionsForAll = [this] {
        if (d->sameOptions->isVisibleTo(this) && d->sameOptions->isChecked()) {
            BusinessLayer::ImportOptions currentOptions = d->currentOptions();

            for (auto& fileOptions : d->filesOptions) {
                if (fileOptions.documentType != currentOptions.documentType) {
                    const auto path = fileOptions.filePath;
                    fileOptions = currentOptions;
                    fileOptions.filePath = path;
                } else {
                    fileOptions.importCharacters = currentOptions.importCharacters;
                    fileOptions.importLocations = currentOptions.importLocations;
                    fileOptions.importText = currentOptions.importText;
                    fileOptions.importResearch = currentOptions.importResearch;
                    fileOptions.keepSceneNumbers = currentOptions.keepSceneNumbers;
                }

                auto model = qobject_cast<QStandardItemModel*>(d->tree->model());
                auto items = model->findItems(fileOptions.filePath);
                if (!items.isEmpty()) {
                    items.first()->setData(typeToString(fileOptions.documentType), ImportTypeRole);
                    items.first()->setData(d->fileCanBeImported(fileOptions), ImportEnabledRole);
                }
            }
            d->tree->update();
        }
    };

    //
    // Соединения для работы чекбокса "Same options for all"
    //
    connect(d->sameOptions, &CheckBox::checkedChanged, this,
            [this, setSameOptionsForAll](bool _checked) {
                if (_checked) {
                    d->importButton->setEnabled(d->isImportAvailable());
                    setSameOptionsForAll();
                }
            });
    connect(d->importCharacters, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->importLocations, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->importText, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->importResearch, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->keepSceneNumbers, &CheckBox::checkedChanged, this, setSameOptionsForAll);

    //
    // Определить можно ли импортировать все документы с одинаковыми настройками
    //
    auto updateSameOptionsAvailable = [this] {
        bool state = d->sameOptions->isChecked();
        bool isEnabled = true;
        for (const auto& fileOptions : d->filesOptions) {
            if (!importTypesForFile(fileOptions.filePath).contains(d->currentType())) {
                state = false;
                isEnabled = false;
                break;
            }
        }
        d->sameOptions->setChecked(state);
        d->sameOptions->setEnabled(isEnabled);
    };

    //
    // Соединение комбобокса типов для импорта
    //
    connect(d->documentType, &ComboBox::currentIndexChanged, this,
            [this, updateSameOptionsAvailable, configureImportAvailability, setSameOptionsForAll] {
                d->updateParametersVisibility();
                if (d->tree->model()) {
                    d->tree->model()->setData(d->tree->currentIndex(),
                                              d->documentType->currentText(), ImportTypeRole);
                    updateSameOptionsAvailable();
                    configureImportAvailability();
                    if (d->sameOptions->isEnabled() && d->sameOptions->isChecked()) {
                        setSameOptionsForAll();
                    }
                }
            });

    //
    // Соединение списка файлов
    //
    connect(d->tree, &Tree::currentIndexChanged, this,
            [this, updateSameOptionsAvailable](const QModelIndex& _index) {
                d->writeCurrentOptions();
                d->currentFile = _index.data().toString();
                d->setImportDocumentTypes(importTypesForFile(d->currentFile));
                d->showOptionsForFile(d->currentFile);
                if (d->sameOptions->isChecked()) {
                    return;
                }
                const bool sameOptionsState = d->sameOptions->isChecked();
                updateSameOptionsAvailable();
                if (d->sameOptions->isEnabled()) {
                    d->sameOptions->setChecked(sameOptionsState);
                }
            });

    //
    // Соединения кнопок импорта/отмены
    //
    connect(d->importButton, &Button::clicked, this, &ImportDialog::importRequested);
    connect(d->cancelButton, &Button::clicked, this, &ImportDialog::canceled);

    //
    // Если импортируем один файл, прячем лишние виджеты
    //
    if (_importFilePaths.size() == 1) {
        d->tree->setVisible(false);
        d->sameOptions->setVisible(false);
        d->filesCountTitle->setVisible(false);
    }

    //
    // Отображаем настройки текущего файла
    //
    d->setImportDocumentTypes(importTypesForFile(d->currentFile));
    d->showOptionsForFile(d->currentFile);
    d->sameOptions->setChecked(false);
    updateSameOptionsAvailable();
}

ImportDialog::~ImportDialog()
{
    d->writeCurrentOptions();
    d->saveSetting();
}

QVector<BusinessLayer::ImportOptions> ImportDialog::importOptions() const
{
    d->writeCurrentOptions();
    d->saveSetting();

    QVector<BusinessLayer::ImportOptions> options;
    for (auto& fileOptions : d->filesOptions) {
        options.push_back(fileOptions);
    }
    d->filesOptions.clear();
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
    if (d->filesOptions.size() > 1) {
        setTitle(tr("Import data from files"));
    } else {
        QFileInfo fileInfo(d->currentFile);
        setTitle(QString("%1 \"%2\"").arg(tr("Import data from the file"), fileInfo.fileName()));
    }

    d->filesCountTitle->setText(tr("Importing files")
                                + QString(" (%1)").arg(d->importFilePaths.size()));
    d->documentsTitle->setText(tr("Documents"));
    d->importCharacters->setText(tr("Import characters"));
    d->importLocations->setText(tr("Import locations"));
    d->importResearch->setText(tr("Import research"));
    d->textTitle->setText(tr("Text"));
    d->updateImportTextLabel();
    d->keepSceneNumbers->setText(tr("Keep scene numbers"));

    d->sameOptions->setText(tr("Same options for all"));
    d->importButton->setText(tr("Import"));
    d->cancelButton->setText(tr("Cancel"));

    //
    // Обновляем выпадающий список
    //
    d->setImportDocumentTypes(importTypesForFile(d->currentFile));
    d->showOptionsForFile(d->currentFile);

    d->documentType->setLabel(tr("Import to"));

    //
    // После установки текста фиксируем размер виджета опций
    //
    if (d->filesOptions.size() > 1) {
        QList<QPair<CheckBox*, bool>> optionsVisability;
        for (auto* option : {
                 d->importCharacters,
                 d->importLocations,
                 d->importResearch,
                 d->importText,
                 d->keepSceneNumbers,
             }) {
            optionsVisability.append(QPair(option, option->isVisibleTo(d->optionsWidget)));
            option->setVisible(true);
        }
        d->optionsWidget->adjustSize();
        d->optionsWidget->setFixedSize(d->optionsWidget->size());
        for (auto option : optionsVisability) {
            option.first->setVisible(option.second);
        }
    }
}

void ImportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->updateDialogWidth();

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());

    auto titleMargins = DesignSystem::label().margins().toMargins();
    titleMargins.setTop(DesignSystem::layout().px16());
    titleMargins.setBottom(DesignSystem::layout().px4());
    d->filesCountTitle->setContentsMargins(titleMargins);
    d->filesCountTitle->setBackgroundColor(DesignSystem::color().background());
    d->filesCountTitle->setTextColor(DesignSystem::color().onBackground());

    d->optionsWidget->setBackgroundColor(DesignSystem::color().background());
    d->optionsWidget->layout()->setContentsMargins(0, DesignSystem::layout().px(32), 0, 0);

    for (auto label : { d->documentsTitle, d->textTitle }) {
        label->setContentsMargins(titleMargins);
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto checkBox : {
             d->importCharacters,
             d->importLocations,
             d->importResearch,
             d->importText,
             d->keepSceneNumbers,
             d->sameOptions,
         }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->importButton, UiHelper::DialogAccept);

    for (auto textField : std::vector<TextField*>{
             d->documentType,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto combobox : {
             d->documentType,
         }) {
        combobox->setPopupBackgroundColor(DesignSystem::color().background());
    }

    d->bottomLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
