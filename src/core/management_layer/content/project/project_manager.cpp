#include "project_manager.h"

#include "include/custom_events.h"
#include "project_models_facade.h"

#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/project/project_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <business_layer/model/structure/structure_proxy_model.h>
#include <data_layer/database.h>
#include <data_layer/storage/document_change_storage.h>
#include <data_layer/storage/document_data_storage.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_change_object.h>
#include <domain/document_object.h>
#include <include/custom_events.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <interfaces/ui/i_document_view.h>
#include <management_layer/plugins_builder.h>
#include <ui/abstract_navigator.h>
#include <ui/design_system/design_system.h>
#include <ui/project/create_document_dialog.h>
#include <ui/project/create_version_dialog.h>
#include <ui/project/project_navigator.h>
#include <ui/project/project_tool_bar.h>
#include <ui/project/project_view.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/splitter/splitter.h>
#include <utils/logging.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QSet>
#include <QShortcut>
#include <QTimer>
#include <QUuid>
#include <QVariantAnimation>


namespace ManagementLayer {

namespace {

const QLatin1String kVersionsVisibleKey("versions-visible");

/**
 * @brief Является ли заданный элемент текстовым
 */
bool isTextItem(BusinessLayer::StructureModelItem* _item)
{
    static const QSet<Domain::DocumentObjectType> textItems = {
        Domain::DocumentObjectType::ScreenplayTitlePage,
        Domain::DocumentObjectType::ScreenplaySynopsis,
        Domain::DocumentObjectType::ScreenplayTreatment,
        Domain::DocumentObjectType::ScreenplayText,
        Domain::DocumentObjectType::ComicBookTitlePage,
        Domain::DocumentObjectType::ComicBookSynopsis,
        Domain::DocumentObjectType::ComicBookText,
        Domain::DocumentObjectType::AudioplayTitlePage,
        Domain::DocumentObjectType::AudioplaySynopsis,
        Domain::DocumentObjectType::AudioplayText,
        Domain::DocumentObjectType::StageplayTitlePage,
        Domain::DocumentObjectType::StageplaySynopsis,
        Domain::DocumentObjectType::StageplayText,
        Domain::DocumentObjectType::SimpleText,
    };
    return textItems.contains(_item->type());
}

/**
 * @brief Сформировать ключ настроек проекта
 */
QString projectSettingsKey(const QString& _key)
{
    return DatabaseLayer::Database::currentFile() + "/" + _key;
}

/**
 * @brief Сформировать ключ настроек для документа проекта
 */
QString documentSettingsKey(const QUuid& _documentUuid, const QString& _key)
{
    return projectSettingsKey(_documentUuid.toString() + "/" + _key);
}

} // namespace

class ProjectManager::Implementation
{
public:
    explicit Implementation(ProjectManager* _q, QWidget* _parent,
                            const PluginsBuilder& _pluginsBuilder);

    /**
     * @brief Обновить текст пункта меню разделения экрана
     */
    void updateOptionsText();

    /**
     * @brief Переключить активное представление
     */
    void switchViews();

    /**
     * @brief Действия контекстного меню
     */
    enum class ContextMenuAction { AddDocument, RemoveDocument, EmptyRecycleBin };

    /**
     * @brief Обновить модель действий контекстного меню навигатора
     */
    void updateNavigatorContextMenu(const QModelIndex& _index);

    /**
     * @brief Открыть текущий документ в отдельном окне
     */
    void openCurrentDocumentInNewWindow();

    /**
     * @brief Добавить документ в проект
     */
    void addDocument();

    /**
     * @brief Добавить документ в заданный контейнер
     */
    void addDocumentToContainer(Domain::DocumentObjectType _containerType,
                                Domain::DocumentObjectType _documentType,
                                const QString& _documentName, const QByteArray& _content = {});

    /**
     * @brief Создать новую версию заданного документа
     */
    void createNewVersion(const QModelIndex& _itemIndex);

    /**
     * @brief Удалить документ
     */
    void removeDocument(const QModelIndex& _itemIndex);
    void removeDocument(BusinessLayer::StructureModelItem* _item);

    /**
     * @brief Найти всех персонажей
     */
    void findAllCharacters();

    /**
     * @brief Найти все локации
     */
    void findAllLocations();

    /**
     * @brief Очистить корзину
     */
    void emptyRecycleBin();

    //
    // Данные
    //

    QWidget* topLevelWidget = nullptr;

    Ui::ProjectToolBar* toolBar = nullptr;

    Ui::ProjectNavigator* navigator = nullptr;

    /**
     * @brief Виджеты представления текущих редакторов
     */
    struct {
        Splitter* container = nullptr;

        Ui::ProjectView* left = nullptr;
        Ui::ProjectView* right = nullptr;

        //
        // Список представлений, открытых в отдельных окнах
        //
        QVector<QWidget*> windows = {};

        //
        // Ссылки на активное представление и неактивное
        //
        Ui::ProjectView* active = nullptr;
        Ui::ProjectView* inactive = nullptr;

        //
        // Индексы элементов представлений в дереве
        //
        QModelIndex activeIndex = {};
        QModelIndex inactiveIndex = {};

        //
        // Состояние контейнера перед входом в полноэкранный режим
        //
        QByteArray stateBeforeFullscreen = {};
    } view;

    /**
     * @brief Действие разделения экрана напополам
     */
    QAction* splitScreenAction = nullptr;
    QShortcut* splitScreenShortcut = nullptr;

    /**
     * @brief Действие отображения списка версий документа
     */
    QAction* showVersionsAction = nullptr;

    /**
     * @brief Модели списка документов
     */
    BusinessLayer::StructureModel* projectStructureModel = nullptr;
    BusinessLayer::StructureProxyModel* projectStructureProxyModel = nullptr;

    /**
     * @brief Хранилище изображений проекта
     */
    DataStorageLayer::DocumentImageStorage documentDataStorage;

    /**
     * @brief Фасад доступа к моделям проекта
     */
    ProjectModelsFacade modelsFacade;

    /**
     * @brief Фабрика для создания плагинов-редакторов к моделям
     */
    const PluginsBuilder& pluginsBuilder;

    /**
     * @brief Информация о текущем документе
     */
    struct {
        BusinessLayer::AbstractModel* model = nullptr;
        QString viewMimeType;
    } currentDocument;
};

ProjectManager::Implementation::Implementation(ProjectManager* _q, QWidget* _parent,
                                               const PluginsBuilder& _pluginsBuilder)
    : topLevelWidget(_parent)
    , toolBar(new Ui::ProjectToolBar(_parent))
    , navigator(new Ui::ProjectNavigator(_parent))
    , view({
          new Splitter(_parent),
          new Ui::ProjectView(_parent),
          new Ui::ProjectView(_parent),
      })
    , splitScreenAction(new QAction(_parent))
    , splitScreenShortcut(new QShortcut(_parent))
    , showVersionsAction(new QAction(_parent))
    , projectStructureModel(new BusinessLayer::StructureModel(navigator))
    , projectStructureProxyModel(new BusinessLayer::StructureProxyModel(projectStructureModel))
    , modelsFacade(projectStructureModel, &documentDataStorage)
    , pluginsBuilder(_pluginsBuilder)
{
    toolBar->hide();
    navigator->hide();
    view.left->installEventFilter(_q);
    view.container->setWidgets(view.left, view.right);
    view.container->setSizes({ 1, 0 });
    view.container->hide();
    view.right->hide();
    view.active = view.left;
    view.inactive = view.right;

    navigator->setModel(projectStructureProxyModel);

    splitScreenAction->setCheckable(true);
    splitScreenAction->setIconText(u8"\U000F10E7");
    splitScreenAction->setShortcut(QKeySequence("F2"));
    showVersionsAction->setCheckable(true);
    showVersionsAction->setIconText(u8"\U000F0AB8");
    updateOptionsText();
    toolBar->setOptions({ splitScreenAction }, AppBarOptionsLevel::App);
    splitScreenShortcut->setKey(QKeySequence("F2"));
    splitScreenShortcut->setContext(Qt::ApplicationShortcut);
}

void ProjectManager::Implementation::updateOptionsText()
{
    splitScreenAction->setText(splitScreenAction->isChecked() ? tr("Remove split")
                                                              : tr("Split window"));
    showVersionsAction->setText(showVersionsAction->isChecked() ? tr("Hide document versions")
                                                                : tr("Show document versions"));
}

void ProjectManager::Implementation::switchViews()
{
    std::swap(view.active, view.inactive);
    view.active->setActive(true);
    view.inactive->setActive(false);

    std::swap(view.activeIndex, view.inactiveIndex);

    //
    // Прежде чем выбрать нужный элемент в навигаторе, заблокируем сигналы, чтобы не было ложных
    // срабатываний и повторной установки плагина редактора
    //
    QSignalBlocker signalBlocker(navigator);
    navigator->setCurrentIndex(projectStructureProxyModel->mapFromSource(view.activeIndex));
}

void ProjectManager::Implementation::updateNavigatorContextMenu(const QModelIndex& _index)
{
    QVector<QAction*> menuActions;

    const auto currentItemIndex = projectStructureProxyModel->mapToSource(_index);
    const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);

    //
    // Формируем список действий для конкретных элементов структуры проекта
    //
    if (currentItem->type() == Domain::DocumentObjectType::Characters) {
        auto findAllCharacters = new QAction(tr("Find all characters"));
        findAllCharacters->setIconText(u8"\U000F0016");
        connect(findAllCharacters, &QAction::triggered, topLevelWidget,
                [this] { this->findAllCharacters(); });
        menuActions.append(findAllCharacters);

        auto addCharacter = new QAction(tr("Add character"));
        addCharacter->setIconText(u8"\U000F0014");
        connect(addCharacter, &QAction::triggered, topLevelWidget, [this] { this->addDocument(); });
        menuActions.append(addCharacter);
    } else if (currentItem->type() == Domain::DocumentObjectType::Locations) {
        auto findAllLocations = new QAction(tr("Find all locations"));
        findAllLocations->setIconText(u8"\U000F13B0");
        connect(findAllLocations, &QAction::triggered, topLevelWidget,
                [this] { this->findAllLocations(); });
        menuActions.append(findAllLocations);

        auto addLocation = new QAction(tr("Add location"));
        addLocation->setIconText(u8"\U000F0975");
        connect(addLocation, &QAction::triggered, topLevelWidget, [this] { this->addDocument(); });
        menuActions.append(addLocation);
    } else if (currentItem->type() == Domain::DocumentObjectType::RecycleBin) {
        if (currentItem->hasChildren()) {
            auto emptyRecycleBin = new QAction(tr("Empty recycle bin"));
            emptyRecycleBin->setIconText(u8"\U000f05e8");
            connect(emptyRecycleBin, &QAction::triggered, topLevelWidget,
                    [this] { this->emptyRecycleBin(); });
            menuActions.append(emptyRecycleBin);
        }
    }
    //
    // ... для остальных элементов
    //
    else {
        auto addDocument = new QAction(tr("Add document"));
        addDocument->setIconText(u8"\U000f0415");
        connect(addDocument, &QAction::triggered, topLevelWidget, [this] { this->addDocument(); });
        menuActions.append(addDocument);

        const QSet<Domain::DocumentObjectType> cantBeRemovedItems = {
            Domain::DocumentObjectType::Project,
            Domain::DocumentObjectType::Characters,
            Domain::DocumentObjectType::Locations,
            Domain::DocumentObjectType::ScreenplayTitlePage,
            Domain::DocumentObjectType::ScreenplaySynopsis,
            Domain::DocumentObjectType::ScreenplayTreatment,
            Domain::DocumentObjectType::ScreenplayText,
            Domain::DocumentObjectType::ScreenplayStatistics,
            Domain::DocumentObjectType::ComicBookTitlePage,
            Domain::DocumentObjectType::ComicBookSynopsis,
            Domain::DocumentObjectType::ComicBookText,
            Domain::DocumentObjectType::ComicBookStatistics,
            Domain::DocumentObjectType::AudioplayTitlePage,
            Domain::DocumentObjectType::AudioplaySynopsis,
            Domain::DocumentObjectType::AudioplayText,
            Domain::DocumentObjectType::AudioplayStatistics,
            Domain::DocumentObjectType::StageplayTitlePage,
            Domain::DocumentObjectType::StageplaySynopsis,
            Domain::DocumentObjectType::StageplayText,
            Domain::DocumentObjectType::StageplayStatistics,
        };
        if (_index.isValid() && !cantBeRemovedItems.contains(currentItem->type())) {
            auto removeDocument = new QAction(tr("Remove document"));
            removeDocument->setIconText(u8"\U000f01b4");
            connect(removeDocument, &QAction::triggered, topLevelWidget,
                    [this, currentItemIndex] { this->removeDocument(currentItemIndex); });
            menuActions.append(removeDocument);
        }
    }

    bool isDocumentActionAdded = false;
    //
    // Для текстовых документов можно создать версию
    //
    if (isTextItem(currentItem)) {
        auto createNewVersion = new QAction(tr("Create new version"));
        createNewVersion->setSeparator(true);
        createNewVersion->setIconText(u8"\U000F00FB");
        connect(createNewVersion, &QAction::triggered, topLevelWidget,
                [this, currentItemIndex] { this->createNewVersion(currentItemIndex); });
        menuActions.append(createNewVersion);

        isDocumentActionAdded = true;
    }

    //
    // Каждый из элементов можно открыть в своём окне
    //
    auto openInNewWindow = new QAction(tr("Open in new window"));
    openInNewWindow->setSeparator(!isDocumentActionAdded);
    openInNewWindow->setIconText(u8"\U000F03CC");
    connect(openInNewWindow, &QAction::triggered, topLevelWidget,
            [this] { this->openCurrentDocumentInNewWindow(); });
    menuActions.append(openInNewWindow);
    isDocumentActionAdded = true;

    navigator->setContextMenuActions(menuActions);
}

void ProjectManager::Implementation::openCurrentDocumentInNewWindow()
{
    if (currentDocument.model == nullptr) {
        return;
    }

    Log::info("Activate plugin \"%1\" in new window", currentDocument.viewMimeType);
    auto view
        = pluginsBuilder.activateWindowView(currentDocument.viewMimeType, currentDocument.model);
    if (auto window = view->asQWidget()) {
        window->resize(800, 600);
        window->show();
        //
        // TODO: Почему-то ни одна из моделей не использует это поле
        //       Ещё сюда нужно писать версию открытого документа, если это не текущая версия
        //
        window->setWindowTitle(currentDocument.model->documentName());

        this->view.windows.append(window);
    }
}

void ProjectManager::Implementation::addDocument()
{
    const auto currentItemIndex
        = projectStructureProxyModel->mapToSource(navigator->currentIndex());
    const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);

    auto dialog = new Ui::CreateDocumentDialog(topLevelWidget);
    if (currentItem->type() == Domain::DocumentObjectType::Folder) {
        dialog->setInsertionParent(currentItem->name());
    } else if (currentItem->type() == Domain::DocumentObjectType::Characters) {
        dialog->setDocumentType(Domain::DocumentObjectType::Character);
    } else if (currentItem->type() == Domain::DocumentObjectType::Locations) {
        dialog->setDocumentType(Domain::DocumentObjectType::Location);
    }

    connect(
        dialog, &Ui::CreateDocumentDialog::createPressed, navigator,
        [this, currentItemIndex, dialog](Domain::DocumentObjectType _type, const QString& _name) {
            if (_type == Domain::DocumentObjectType::Character) {
                auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
                    Domain::DocumentObjectType::Characters);
                auto model = modelsFacade.modelFor(document);
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(model);
                Q_ASSERT(charactersModel);
                if (charactersModel->exists(_name)) {
                    dialog->setNameError(tr("Character with this name already exists"));
                    return;
                }
            } else if (_type == Domain::DocumentObjectType::Location) {
                auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
                    Domain::DocumentObjectType::Locations);
                auto model = modelsFacade.modelFor(document);
                auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(model);
                Q_ASSERT(locationsModel);
                if (locationsModel->exists(_name)) {
                    dialog->setNameError(tr("Location with this name already exists"));
                    return;
                }
            }

            //
            // Определим индекс родительского элемента, ищем именно папку
            //
            auto parentIndex
                = dialog->needInsertIntoParent() ? currentItemIndex : currentItemIndex.parent();
            auto parentItem = projectStructureModel->itemForIndex(parentIndex);
            while (parentIndex.isValid() && parentItem != nullptr
                   && parentItem->type() != Domain::DocumentObjectType::Folder) {
                parentIndex = parentIndex.parent();
                parentItem = projectStructureModel->itemForIndex(parentIndex);
            }

            const auto addedItemIndex
                = projectStructureModel->addDocument(_type, _name, parentIndex);
            const auto mappedAddedItemIndex
                = projectStructureProxyModel->mapFromSource(addedItemIndex);
            navigator->setCurrentIndex(mappedAddedItemIndex);

            dialog->hideDialog();
        });
    connect(dialog, &Ui::CreateDocumentDialog::disappeared, dialog,
            &Ui::CreateDocumentDialog::deleteLater);

    dialog->showDialog();
}

void ProjectManager::Implementation::addDocumentToContainer(
    Domain::DocumentObjectType _containerType, Domain::DocumentObjectType _documentType,
    const QString& _documentName, const QByteArray& _content)
{
    for (int itemRow = 0; itemRow < projectStructureModel->rowCount(); ++itemRow) {
        const auto itemIndex = projectStructureModel->index(itemRow, 0);
        const auto item = projectStructureModel->itemForIndex(itemIndex);
        if (item->type() == _containerType) {
            projectStructureModel->addDocument(_documentType, _documentName, itemIndex, _content);
            break;
        }
    }
}

void ProjectManager::Implementation::createNewVersion(const QModelIndex& _itemIndex)
{
    auto dialog = new Ui::CreateVersionDialog(topLevelWidget);
    connect(dialog, &Ui::CreateVersionDialog::createPressed, navigator,
            [this, _itemIndex, dialog](const QString& _name, const QColor& _color, bool _readOnly) {
                const auto item = projectStructureModel->itemForIndex(_itemIndex);
                const auto model = modelsFacade.modelFor(item->uuid());
                projectStructureModel->addItemVersion(item, _name, _color, _readOnly,
                                                      model->document()->content());
                view.active->setDocumentVersions(item->versions());

                dialog->hideDialog();
            });
    connect(dialog, &Ui::CreateVersionDialog::disappeared, dialog,
            &Ui::CreateVersionDialog::deleteLater);

    dialog->showDialog();
}

void ProjectManager::Implementation::removeDocument(const QModelIndex& _itemIndex)
{
    auto item = projectStructureModel->itemForIndex(_itemIndex);
    if (item == nullptr) {
        return;
    }

    removeDocument(item);
}

void ProjectManager::Implementation::removeDocument(BusinessLayer::StructureModelItem* _item)
{
    auto itemTopLevelParent = _item->parent();
    if (itemTopLevelParent == nullptr) {
        return;
    }
    while (itemTopLevelParent->parent()
           && itemTopLevelParent->parent()->type() != Domain::DocumentObjectType::Undefined) {
        itemTopLevelParent = itemTopLevelParent->parent();
    }

    //
    // Если документ ещё не в корзине, то переносим его в корзину
    //
    if (itemTopLevelParent->type() != Domain::DocumentObjectType::RecycleBin) {
        projectStructureModel->moveItemToRecycleBin(_item);
        return;
    }

    //
    // Если документ в корзине
    //
    // ... то спросим действительно ли пользователь хочет его удалить
    //
    const int kCancelButtonId = 0;
    const int kRemoveButtonId = 1;
    auto dialog = new Dialog(topLevelWidget);
    dialog->showDialog({}, tr("Do you really want to permanently remove document?"),
                       { { kCancelButtonId, tr("No"), Dialog::RejectButton },
                         { kRemoveButtonId, tr("Yes, remove"), Dialog::NormalButton } });
    QObject::connect(
        dialog, &Dialog::finished,
        [this, _item, kCancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал удалять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Если таки хочет, то удаляем документ
            // NOTE: порядок удаления важен
            //
            auto document
                = DataStorageLayer::StorageFacade::documentStorage()->document(_item->uuid());
            modelsFacade.removeModelFor(document);
            DataStorageLayer::StorageFacade::documentStorage()->removeDocument(document);
            projectStructureModel->removeItem(_item);
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::findAllCharacters()
{
    //
    // Найти все модели где могут встречаться персонажи и определить их
    //
    QSet<QString> charactersFromText;
    const auto screenplayModels
        = modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
    for (auto model : screenplayModels) {
        auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
        charactersFromText.unite(screenplay->findCharactersFromText());
    }
    //
    const auto comicBookModels = modelsFacade.modelsFor(Domain::DocumentObjectType::ComicBookText);
    for (auto model : comicBookModels) {
        auto comicBook = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
        charactersFromText.unite(comicBook->findCharactersFromText());
    }
    //
    const auto audioplayModels = modelsFacade.modelsFor(Domain::DocumentObjectType::AudioplayText);
    for (auto model : audioplayModels) {
        auto audioplay = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
        charactersFromText.unite(audioplay->findCharactersFromText());
    }
    //
    const auto stageplayModels = modelsFacade.modelsFor(Domain::DocumentObjectType::StageplayText);
    for (auto model : stageplayModels) {
        auto stageplay = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
        charactersFromText.unite(stageplay->findCharactersFromText());
    }
    charactersFromText.remove({});

    //
    // Определить персонажи, которых нет в тексте
    //
    QSet<QString> charactersNotFromText;
    const auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
        modelsFacade.modelFor(Domain::DocumentObjectType::Characters));
    for (int row = 0; row < charactersModel->rowCount(); ++row) {
        const auto characterName = charactersModel->index(row, 0).data().toString();
        if (!charactersFromText.contains(characterName)) {
            charactersNotFromText.insert(characterName);
        }
    }

    //
    // Спросить пользователя, что он хочет сделать с ними
    //
    QString message;
    if (!charactersFromText.isEmpty()) {
        QStringList characters = charactersFromText.values();
        std::sort(characters.begin(), characters.end());
        message.append(
            QString("%1:\n%2.").arg(tr("Characters from the text"), characters.join(", ")));
    }
    if (!charactersNotFromText.isEmpty()) {
        if (!message.isEmpty()) {
            message.append("\n\n");
        }
        QStringList characters = charactersNotFromText.values();
        std::sort(characters.begin(), characters.end());
        message.append(
            QString("%1:\n%2.")
                .arg(tr("Characters that are not found in the text"), characters.join(", ")));
    }
    const int kCancelButtonId = 0;
    const int kKeepFromTextButtonId = 1;
    const int kKeepAllButtonId = 2;
    auto dialog = new Dialog(topLevelWidget);
    //
    // Т.к. персов может быть очень много, расширяем максимальное ограничение по ширине для диалога
    //
    dialog->setContentMaximumWidth(topLevelWidget->width() * 0.7);
    const auto placeButtonSideBySide = false;
    dialog->showDialog(
        {}, message,
        { { kKeepFromTextButtonId, tr("Save only characters from the text"), Dialog::NormalButton },
          { kKeepAllButtonId, tr("Save all characters"), Dialog::NormalButton },
          { kCancelButtonId, tr("Change nothing"), Dialog::RejectButton } },
        placeButtonSideBySide);
    QObject::connect(
        dialog, &Dialog::finished, dialog,
        [this, charactersFromText, charactersNotFromText, charactersModel, dialog, kCancelButtonId,
         kKeepFromTextButtonId](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал что-либо менять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Если надо, удалим персонажей, которые не встречаются в тексте
            //
            if (_buttonInfo.id == kKeepFromTextButtonId) {
                for (const auto& characterName : charactersNotFromText) {
                    const auto characterModel = charactersModel->character(characterName);
                    auto item
                        = projectStructureModel->itemForUuid(characterModel->document()->uuid());
                    removeDocument(item);
                }
            }

            //
            // Сохраняем новых персонажей, которых ещё не было в базе
            //
            for (const auto& characterName : charactersFromText) {
                if (charactersModel->exists(characterName)) {
                    continue;
                }

                addDocumentToContainer(Domain::DocumentObjectType::Characters,
                                       Domain::DocumentObjectType::Character, characterName);
            }
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::findAllLocations()
{
    //
    // Найти все модели где могут встречаться локации и определить их
    //
    QSet<QString> locationsFromText;
    const auto screenplayModels
        = modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
    for (auto model : screenplayModels) {
        auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
        locationsFromText.unite(screenplay->findLocationsFromText());
    }
    locationsFromText.remove({});

    //
    // Определить локации, которых нет в тексте
    //
    QSet<QString> locationsNotFromText;
    const auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
        modelsFacade.modelFor(Domain::DocumentObjectType::Locations));
    for (int row = 0; row < locationsModel->rowCount(); ++row) {
        const auto locationName = locationsModel->index(row, 0).data().toString();
        if (!locationsFromText.contains(locationName)) {
            locationsNotFromText.insert(locationName);
        }
    }

    //
    // Спросить пользователя, что он хочет сделать с ними
    //
    QString message;
    if (!locationsFromText.isEmpty()) {
        QStringList locations = locationsFromText.values();
        std::sort(locations.begin(), locations.end());
        message.append(
            QString("%1:\n%2.").arg(tr("Locations from the text"), locations.join(", ")));
    }
    if (!locationsNotFromText.isEmpty()) {
        if (!message.isEmpty()) {
            message.append("\n\n");
        }
        QStringList locations = locationsNotFromText.values();
        std::sort(locations.begin(), locations.end());
        message.append(
            QString("%1:\n%2.")
                .arg(tr("Locations that are not found in the text"), locations.join(", ")));
    }
    const int kCancelButtonId = 0;
    const int kKeepFromTextButtonId = 1;
    const int kKeepAllButtonId = 2;
    auto dialog = new Dialog(topLevelWidget);
    //
    // Т.к. локаций может быть очень много, расширяем максимальное ограничение по ширине для диалога
    //
    dialog->setContentMaximumWidth(topLevelWidget->width() * 0.7);
    const auto placeButtonSideBySide = false;
    dialog->showDialog(
        {}, message,
        { { kKeepFromTextButtonId, tr("Save only locations from the text"), Dialog::NormalButton },
          { kKeepAllButtonId, tr("Save all locations"), Dialog::NormalButton },
          { kCancelButtonId, tr("Change nothing"), Dialog::RejectButton } },
        placeButtonSideBySide);
    QObject::connect(
        dialog, &Dialog::finished, dialog,
        [this, locationsFromText, locationsNotFromText, locationsModel, dialog, kCancelButtonId,
         kKeepFromTextButtonId](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал что-либо менять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Если надо, удалим локации, которые не встречаются в тексте
            //
            if (_buttonInfo.id == kKeepFromTextButtonId) {
                for (const auto& locationName : locationsNotFromText) {
                    const auto locationModel = locationsModel->location(locationName);
                    auto item
                        = projectStructureModel->itemForUuid(locationModel->document()->uuid());
                    removeDocument(item);
                }
            }

            //
            // Сохраняем новых локации, которых ещё не было в базе
            //
            for (const auto& locationName : locationsFromText) {
                if (locationsModel->exists(locationName)) {
                    continue;
                }

                addDocumentToContainer(Domain::DocumentObjectType::Locations,
                                       Domain::DocumentObjectType::Location, locationName);
            }
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::emptyRecycleBin()
{
    auto recycleBin = projectStructureModel->itemForType(Domain::DocumentObjectType::RecycleBin);
    if (recycleBin == nullptr) {
        return;
    }

    //
    // Спросим действительно ли пользователь хочет очистить корзину
    //
    const int kCancelButtonId = 0;
    const int kEmptyButtonId = 1;
    auto dialog = new Dialog(topLevelWidget);
    dialog->showDialog(
        {}, tr("Do you really want to permanently remove all documents from the recycle bin?"),
        { { kCancelButtonId, tr("No"), Dialog::RejectButton },
          { kEmptyButtonId, tr("Yes, remove"), Dialog::NormalButton } });
    QObject::connect(
        dialog, &Dialog::finished,
        [this, recycleBin, kCancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал очищать корзину
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Если таки хочет, то удаляем все вложенные документы рекурсивно
            // NOTE: порядок удаления важен
            //
            std::function<void(BusinessLayer::StructureModelItem*)> removeItem;
            removeItem = [this, &removeItem](BusinessLayer::StructureModelItem* _item) {
                //
                // Сначала удаляем детей
                //
                while (_item->hasChildren()) {
                    auto child = _item->childAt(0);
                    removeItem(child);
                }
                //
                // ... а потом сам элемент
                //
                auto documentToRemove
                    = DataStorageLayer::StorageFacade::documentStorage()->document(_item->uuid());
                if (documentToRemove != nullptr) {
                    modelsFacade.removeModelFor(documentToRemove);
                    DataStorageLayer::StorageFacade::documentStorage()->removeDocument(
                        documentToRemove);
                }
                projectStructureModel->removeItem(_item);
            };
            while (recycleBin->hasChildren()) {
                auto itemToRemove = recycleBin->childAt(0);
                removeItem(itemToRemove);
            }

            //
            // Дизейблим кнопку очистки корзины в навигаторе
            //
            navigator->setButtonEnabled(false);
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}


// ****


ProjectManager::ProjectManager(QObject* _parent, QWidget* _parentWidget,
                               const PluginsBuilder& _pluginsBuilder)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget, _pluginsBuilder))
{
    connect(d->toolBar, &Ui::ProjectToolBar::menuPressed, this, &ProjectManager::menuRequested);
    connect(d->toolBar, &Ui::ProjectToolBar::viewPressed, this, [this](const QString& _mimeType) {
        showView(d->navigator->currentIndex(), _mimeType);
    });
    connect(d->splitScreenAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsText();
        if (_checked) {
            d->view.container->setSizes({ 1, 1 });
            d->view.right->show();
            d->switchViews();
        } else {
            d->view.container->setSizes({ 1, 0 });
            d->view.right->hide();
            if (d->view.active == d->view.right) {
                d->switchViews();
            }
        }
    });
    connect(d->splitScreenShortcut, &QShortcut::activated, this, [this] {
        if (!d->view.container->isVisible()) {
            return;
        }

        d->splitScreenAction->toggle();
    });
    connect(d->showVersionsAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsText();
        d->view.active->setVersionsVisible(_checked);
    });

    //
    // Отображаем необходимый редактор при выборе документа в списке
    //
    connect(d->navigator, &Ui::ProjectNavigator::itemSelected, this,
            [this](const QModelIndex& _index) {
                if (!_index.isValid()) {
                    d->view.active->showDefaultPage();
                    return;
                }

                //
                // Определим выделенный элемент и скорректируем интерфейс
                //
                const auto mappedIndex = d->projectStructureProxyModel->mapToSource(_index);
                const auto item = d->projectStructureModel->itemForIndex(mappedIndex);
                const auto documentMimeType = Domain::mimeTypeFor(item->type());
                //
                // ... настроим иконки представлений
                //
                d->toolBar->clearViews();
                const auto views = d->pluginsBuilder.editorsInfoFor(documentMimeType);
                for (const auto& view : views) {
                    const auto tooltip
                        = d->pluginsBuilder.editorDescription(documentMimeType, view.mimeType);
                    const bool isActive = view.mimeType == views.first().mimeType;
                    d->toolBar->addView(view.mimeType, view.icon, tooltip, isActive);
                }

                //
                // Откроем документ на редактирование в первом из представлений
                //
                if (views.isEmpty()) {
                    d->view.active->showNotImplementedPage();
                    return;
                }
                showView(_index, views.first().mimeType);

                //
                // Для корзины и вложенных элементов вместо кнопки добавления документов показываем
                // кнопку очистки корзины
                //
                bool isInRecycleBin = false;
                auto topLevelParent = item;
                while (topLevelParent != nullptr) {
                    if (topLevelParent->type() == Domain::DocumentObjectType::RecycleBin) {
                        isInRecycleBin = true;
                        break;
                    }
                    topLevelParent = topLevelParent->parent();
                }
                d->navigator->showButton(isInRecycleBin
                                             ? Ui::ProjectNavigator::ActionButton::EmptyRecycleBin
                                             : Ui::ProjectNavigator::ActionButton::AddDocument);
                d->navigator->setButtonEnabled(true);
            });
    //
    // Отображаем навигатор выбранного элемента
    //
    connect(d->navigator, &Ui::ProjectNavigator::itemDoubleClicked, this,
            [this](const QModelIndex& _index) {
                const auto mappedIndex = d->projectStructureProxyModel->mapToSource(_index);
                if (!d->projectStructureModel
                         ->data(mappedIndex,
                                static_cast<int>(
                                    BusinessLayer::StructureModelDataRole::IsNavigatorAvailable))
                         .toBool()) {
                    return;
                }

                //                if (mappedIndex.child())
                showNavigator(_index);
            });
    connect(d->navigator, &Ui::ProjectNavigator::itemNavigationRequested, this,
            [this](const QModelIndex& _index) { showNavigator(_index); });
    connect(d->navigator, &Ui::ProjectNavigator::contextMenuUpdateRequested, this,
            [this](const QModelIndex& _index) { d->updateNavigatorContextMenu(_index); });
    connect(d->navigator, &Ui::ProjectNavigator::addDocumentClicked, this,
            [this] { d->addDocument(); });
    connect(d->navigator, &Ui::ProjectNavigator::emptyRecycleBinClicked, this,
            [this] { d->emptyRecycleBin(); });

    //
    // Соединения с моделью структуры проекта
    //
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::documentAdded, this,
            [this](const QUuid& _uuid, const QUuid& _parentUuid, Domain::DocumentObjectType _type,
                   const QString& _name, const QByteArray& _content) {
                Q_UNUSED(_parentUuid);

                auto document = DataStorageLayer::StorageFacade::documentStorage()->createDocument(
                    _uuid, _type);
                if (!_content.isNull()) {
                    document->setContent(_content);
                }

                auto documentModel = d->modelsFacade.modelFor(document);
                documentModel->setDocumentName(_name);
                if (!_content.isNull()) {
                    documentModel->reassignContent();
                }

                switch (_type) {
                case Domain::DocumentObjectType::Character: {
                    auto charactersDocument
                        = DataStorageLayer::StorageFacade::documentStorage()->document(
                            Domain::DocumentObjectType::Characters);
                    auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                        d->modelsFacade.modelFor(charactersDocument));
                    auto characterModel
                        = qobject_cast<BusinessLayer::CharacterModel*>(documentModel);
                    charactersModel->addCharacterModel(characterModel);

                    break;
                }

                case Domain::DocumentObjectType::Location: {
                    auto locationsDocument
                        = DataStorageLayer::StorageFacade::documentStorage()->document(
                            Domain::DocumentObjectType::Locations);
                    auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
                        d->modelsFacade.modelFor(locationsDocument));
                    auto locationModel = qobject_cast<BusinessLayer::LocationModel*>(documentModel);
                    locationsModel->addLocationModel(locationModel);

                    break;
                }

                default:
                    break;
                }
            });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::contentsChanged, this,
            [this](const QByteArray& _undo, const QByteArray& _redo) {
                handleModelChange(d->projectStructureModel, _undo, _redo);
            });
    connect(
        d->projectStructureModel, &BusinessLayer::StructureModel::rowsAboutToBeMoved, this,
        [this](const QModelIndex& _sourceParent, int _sourceStart, int _sourceEnd,
               const QModelIndex& _destination) {
            const auto sourceParentItem = d->projectStructureModel->itemForIndex(_sourceParent);
            const auto destinationItem = d->projectStructureModel->itemForIndex(_destination);
            if (sourceParentItem == nullptr || destinationItem == nullptr) {
                return;
            }

            //
            // Удаляем персонажа
            //
            if (sourceParentItem->type() == Domain::DocumentObjectType::Characters
                && destinationItem->type() == Domain::DocumentObjectType::RecycleBin) {
                const auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = d->modelsFacade.modelFor(charactersDocument);
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    characters->removeCharacterModel(qobject_cast<BusinessLayer::CharacterModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Удаляем локации
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::Locations
                     && destinationItem->type() == Domain::DocumentObjectType::RecycleBin) {
                const auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = d->modelsFacade.modelFor(locationsDocument);
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    locations->removeLocationModel(qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем персонажей
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::RecycleBin
                     && destinationItem->type() == Domain::DocumentObjectType::Characters) {
                const auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = d->modelsFacade.modelFor(charactersDocument);
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    characters->addCharacterModel(qobject_cast<BusinessLayer::CharacterModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем локации
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::RecycleBin
                     && destinationItem->type() == Domain::DocumentObjectType::Locations) {
                const auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = d->modelsFacade.modelFor(locationsDocument);
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    locations->addLocationModel(qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
        });

    //
    // Соединения представления
    //
    for (auto view : { d->view.left, d->view.right }) {
        connect(view, &Ui::ProjectView::createNewItemPressed, this, [this] { d->addDocument(); });
        connect(view, &Ui::ProjectView::showVersionPressed, this, [this](int _versionIndex) {
            const auto currentItemIndex
                = d->projectStructureProxyModel->mapToSource(d->navigator->currentIndex());
            const auto currentItem = d->projectStructureModel->itemForIndex(currentItemIndex);

            //
            // Показать текущую версию
            //
            if (_versionIndex == 0) {
                showViewForVersion(currentItem);
                return;
            }

            //
            // Показать одну из установленных версий
            //
            showViewForVersion(currentItem->versions().at(_versionIndex - 1));
        });
    }

    //
    // Соединения со строителем моделей
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::modelNameChanged, this,
            [this](BusinessLayer::AbstractModel* _model, const QString& _name) {
                auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
                d->projectStructureModel->setItemName(item, _name);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::modelColorChanged, this,
            [this](BusinessLayer::AbstractModel* _model, const QColor& _color) {
                auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
                d->projectStructureModel->setItemColor(item, _color);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::modelContentChanged, this,
            &ProjectManager::handleModelChange);
    connect(&d->modelsFacade, &ProjectModelsFacade::modelUndoRequested, this,
            &ProjectManager::undoModelChange);
    connect(&d->modelsFacade, &ProjectModelsFacade::modelRemoveRequested, this,
            [this](BusinessLayer::AbstractModel* _model) {
                auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
                d->removeDocument(item);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::projectNameChanged, this,
            &ProjectManager::projectNameChanged);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectLoglineChanged, this,
            &ProjectManager::projectLoglineChanged);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectCoverChanged, this,
            &ProjectManager::projectCoverChanged);
    connect(&d->modelsFacade, &ProjectModelsFacade::createCharacterRequested, this,
            [this](const QString& _name, const QByteArray& _content) {
                d->addDocumentToContainer(Domain::DocumentObjectType::Characters,
                                          Domain::DocumentObjectType::Character, _name, _content);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::characterNameChanged, this,
            [this](const QString& _newName, const QString& _oldName) {
                if (_oldName.isEmpty()) {
                    return;
                }

                //
                // Найти все модели где может встречаться персонаж и заменить в них его имя со
                // старого на новое
                //
                const auto screenplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
                for (auto model : screenplayModels) {
                    auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    screenplay->updateCharacterName(_oldName, _newName);
                }
                //
                const auto comicBookModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ComicBookText);
                for (auto model : comicBookModels) {
                    auto comicBook = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
                    comicBook->updateCharacterName(_oldName, _newName);
                }
                //
                const auto audioplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::AudioplayText);
                for (auto model : audioplayModels) {
                    auto audioplay = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
                    audioplay->updateCharacterName(_oldName, _newName);
                }
                //
                const auto stageplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::StageplayText);
                for (auto model : stageplayModels) {
                    auto stageplay = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
                    stageplay->updateCharacterName(_oldName, _newName);
                }
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::createLocationRequested, this,
            [this](const QString& _name, const QByteArray& _content) {
                d->addDocumentToContainer(Domain::DocumentObjectType::Locations,
                                          Domain::DocumentObjectType::Location, _name, _content);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::locationNameChanged, this,
            [this](const QString& _newName, const QString& _oldName) {
                if (_oldName.isEmpty()) {
                    return;
                }

                //
                // Найти все модели где может встречаться персонаж и заменить в них его имя со
                // старого на новое
                //
                const auto models
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
                for (auto model : models) {
                    auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    screenplay->updateLocationName(_oldName, _newName);
                }
            });
    //
    auto setDocumentVisible = [this](BusinessLayer::AbstractModel* _screenplayModel,
                                     Domain::DocumentObjectType _type, bool _visible) {
        auto screenplayItem
            = d->projectStructureModel->itemForUuid(_screenplayModel->document()->uuid());
        for (int childIndex = 0; childIndex < screenplayItem->childCount(); ++childIndex) {
            auto childItem = screenplayItem->childAt(childIndex);
            if (childItem->type() == _type) {
                d->projectStructureModel->setItemVisible(childItem, _visible);
                break;
            }
        }
    };
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayTitlePageVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayTitlePage,
                                   _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySynopsisVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplaySynopsis,
                                   _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayTreatmentVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayTreatment,
                                   _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayTextVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayStatisticsVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayStatistics,
                                   _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookTitlePageVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookTitlePage,
                                   _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookSynopsisVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookSynopsis, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookTextVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookStatisticsVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookStatistics,
                                   _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplayTitlePageVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::AudioplayTitlePage,
                                   _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplaySynopsisVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::AudioplaySynopsis, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplayTextVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::AudioplayText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplayStatisticsVisibilityChanged, this,
            [setDocumentVisible](BusinessLayer::AbstractModel* _model, bool _visible) {
                setDocumentVisible(_model, Domain::DocumentObjectType::AudioplayStatistics,
                                   _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::emptyRecycleBinRequested, this,
            [this] { d->emptyRecycleBin(); });
}

ProjectManager::~ProjectManager() = default;

QWidget* ProjectManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ProjectManager::navigator() const
{
    return d->navigator;
}

QWidget* ProjectManager::view() const
{
    return d->view.container;
}

void ProjectManager::setHasUnreadNotifications(bool _hasUnreadNotifications)
{
    d->toolBar->setBadgeVisible(d->toolBar->actions().constFirst(), _hasUnreadNotifications);
}

void ProjectManager::toggleFullScreen(bool _isFullScreen)
{
    //
    // Отключаем возможность разделять и объединять панели в полноэкранном режиме
    //
    d->splitScreenAction->setEnabled(!_isFullScreen);

    //
    // При переходе в полноэкранный режим, если активировано разделение экрана, то скроем неактивный
    // редактор и запомним состояние разделения
    //
    if (_isFullScreen) {
        if (d->splitScreenAction->isChecked()) {
            d->view.stateBeforeFullscreen = d->view.container->saveState();
            d->view.inactive->hide();
        }
        if (d->showVersionsAction->isChecked()) {
            d->view.active->setVersionsVisible(false);
        }
    }

    //
    // Собственно активируем полноэкранный режим
    //
    if (d->view.active == d->view.left) {
        d->pluginsBuilder.toggleViewFullScreen(_isFullScreen, d->currentDocument.viewMimeType);
    } else {
        d->pluginsBuilder.toggleSecondaryViewFullScreen(_isFullScreen,
                                                        d->currentDocument.viewMimeType);
    }

    //
    // При выходе из полноэкранного режима, если экран был разделён, то покажем неактивный редактор
    // и восстановим состояние разеделения
    //
    if (!_isFullScreen) {
        if (d->splitScreenAction->isChecked()) {
            d->view.inactive->show();
            d->view.container->restoreState(d->view.stateBeforeFullscreen);
        }
        if (d->showVersionsAction->isChecked()) {
            d->view.active->setVersionsVisible(true);
        }
    }
}

void ProjectManager::reconfigureAll()
{
    d->pluginsBuilder.reconfigureAll();
}

void ProjectManager::reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureSimpleTextEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureSimpleTextNavigator()
{
    d->pluginsBuilder.reconfigureSimpleTextNavigator();
}

void ProjectManager::reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureScreenplayEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureScreenplayNavigator()
{
    d->pluginsBuilder.reconfigureScreenplayNavigator();
}

void ProjectManager::reconfigureScreenplayDuration()
{
    for (auto model : d->modelsFacade.loadedModels()) {
        auto screenplayModel = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
        if (screenplayModel == nullptr) {
            continue;
        }

        screenplayModel->recalculateDuration();
    }
}

void ProjectManager::reconfigureComicBookEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureComicBookEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureComicBookNavigator()
{
    d->pluginsBuilder.reconfigureComicBookNavigator();
}

void ProjectManager::reconfigureAudioplayEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureAudioplayEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureAudioplayNavigator()
{
    d->pluginsBuilder.reconfigureAudioplayNavigator();
}

void ProjectManager::reconfigureAudioplayDuration()
{
    for (auto model : d->modelsFacade.loadedModels()) {
        auto audioplayModel = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
        if (audioplayModel == nullptr) {
            continue;
        }

        audioplayModel->recalculateDuration();
    }
}

void ProjectManager::reconfigureStageplayEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureStageplayEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureStageplayNavigator()
{
    d->pluginsBuilder.reconfigureStageplayNavigator();
}

void ProjectManager::checkAvailabilityToEdit()
{
    d->pluginsBuilder.checkAvailabilityToEdit();
}

void ProjectManager::loadCurrentProject(const QString& _name, const QString& _path)
{
    //
    // Загружаем структуру
    //
    d->projectStructureModel->setProjectName(_name);
    d->projectStructureModel->setDocument(
        DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Structure));

    //
    // Загружаем информацию о проекте
    //
    auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(
        d->modelsFacade.modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Project)));
    if (projectInformationModel->name().isEmpty()) {
        projectInformationModel->setName(_name);
    } else {
        emit projectNameChanged(projectInformationModel->name());
        emit projectLoglineChanged(projectInformationModel->logline());
        emit projectCoverChanged(projectInformationModel->cover());
    }


    //
    // Синхронизировать структуру с облаком
    //

    //
    // Открыть структуру
    //

    restoreCurrentProjectState(_path);

    //
    // Синхронизировать выбранный документ
    //

    //
    // Синхрониировать все остальные изменения
    //
}

void ProjectManager::restoreCurrentProjectState(const QString& _path)
{
    //
    // Загрузить состояние дерева
    //
    d->navigator->restoreState(d->projectStructureModel->isNewProject(),
                               settingsValue(DataStorageLayer::projectStructureKey(_path)));

    //
    // При необходимости открыть навигатор по документу
    //
    const auto isProjectStructureVisible
        = settingsValue(DataStorageLayer::projectStructureVisibleKey(_path));
    if (isProjectStructureVisible.isValid() && !isProjectStructureVisible.toBool()) {
        showNavigator(d->navigator->currentIndex());
    }
}

void ProjectManager::closeCurrentProject(const QString& _path)
{
    //
    // Сохранить состояние дерева
    //
    setSettingsValue(DataStorageLayer::projectStructureKey(_path), d->navigator->saveState());
    setSettingsValue(DataStorageLayer::projectStructureVisibleKey(_path),
                     d->navigator->isProjectNavigatorShown());

    //
    // FIXME: Сохранять и восстанавливать состояние панелей
    //
    while (!d->view.windows.isEmpty()) {
        auto window = d->view.windows.takeFirst();
        window->close();
        window->deleteLater();
    }
    if (d->splitScreenAction->isChecked()) {
        d->splitScreenAction->toggle();
        d->view.activeIndex = {};
        d->view.active->showDefaultPage();
        d->view.inactiveIndex = {};
        d->view.inactive->showDefaultPage();
    }

    //
    // Очищаем структуру
    //
    d->projectStructureModel->clear();

    //
    // Сбрасываем все плагины
    //
    d->pluginsBuilder.resetModels();

    //
    // Очищаем все загруженные модели документов
    //
    d->modelsFacade.clear();

    //
    // Сбрасываем загруженные изображения
    //
    d->documentDataStorage.clear();
}

void ProjectManager::saveChanges()
{
    //
    // Сохраняем структуру
    //
    const auto structure = d->projectStructureModel->document();
    DataStorageLayer::StorageFacade::documentStorage()->saveDocument(structure);

    //
    // Сохраняем остальные документы
    //
    const auto loadedDocuments = d->modelsFacade.loadedDocuments();
    for (auto document : loadedDocuments) {
        DataStorageLayer::StorageFacade::documentStorage()->saveDocument(document);
    }

    //
    // Сохраняем изображения
    //
    d->documentDataStorage.saveChanges();

    //
    // Сохраняем все изменения документов
    //
    DataStorageLayer::StorageFacade::documentChangeStorage()->store();
}

void ProjectManager::addCharacter(const QString& _name, const QString& _content)
{
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
        Domain::DocumentObjectType::Characters);
    auto model = d->modelsFacade.modelFor(document);
    auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(model);
    if (charactersModel == nullptr) {
        return;
    }

    charactersModel->createCharacter(_name, _content.toUtf8());
}

void ProjectManager::addLocation(const QString& _name, const QString& _content)
{
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
        Domain::DocumentObjectType::Locations);
    auto model = d->modelsFacade.modelFor(document);
    auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(model);
    if (locationsModel == nullptr) {
        return;
    }

    locationsModel->createLocation(_name, _content.toUtf8());
}

void ProjectManager::addScreenplay(const QString& _name, const QString& _titlePage,
                                   const QString& _synopsis, const QString& _treatment,
                                   const QString& _text)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible);
    };

    auto rootItem = d->projectStructureModel->itemForIndex({});
    auto screenplayItem = createItem(DocumentObjectType::Screenplay, _name);
    d->projectStructureModel->appendItem(screenplayItem, rootItem);

    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ScreenplayTitlePage, tr("Title page")), screenplayItem,
        _titlePage.toUtf8());
    auto synopsisItem = createItem(DocumentObjectType::ScreenplaySynopsis, tr("Synopsis"));
    d->projectStructureModel->appendItem(synopsisItem, screenplayItem, _synopsis.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ScreenplayText, tr("Screenplay")), screenplayItem,
        _text.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ScreenplayStatistics, tr("Statistics")), screenplayItem, {});
    //
    // Вставляем тритмент после всех документов, т.к. он является алиасом к документу сценария
    // и чтобы его сконструировать, нужны другие документы
    //
    d->projectStructureModel->insertItem(
        createItem(DocumentObjectType::ScreenplayTreatment, tr("Treatment")), synopsisItem,
        _treatment.toUtf8());

    emit contentsChanged();
}

BusinessLayer::AbstractModel* ProjectManager::currentModelForExport() const
{
    auto scriptTextModel
        = [this](const BusinessLayer::StructureModelItem* _item) -> BusinessLayer::AbstractModel* {
        if (_item == nullptr) {
            return nullptr;
        }

        constexpr int scriptTextIndex = 2;
        return d->modelsFacade.modelFor(_item->childAt(scriptTextIndex)->uuid());
    };

    const auto document = d->currentDocument.model->document();
    const auto item = d->projectStructureModel->itemForUuid(document->uuid());
    switch (document->type()) {
    case Domain::DocumentObjectType::Audioplay: {
        return scriptTextModel(item);
    }
    case Domain::DocumentObjectType::AudioplayTitlePage:
    case Domain::DocumentObjectType::AudioplaySynopsis: {
        if (item == nullptr) {
            break;
        }

        return scriptTextModel(item->parent());
    }

    case Domain::DocumentObjectType::ComicBook: {
        return scriptTextModel(item);
    }
    case Domain::DocumentObjectType::ComicBookTitlePage:
    case Domain::DocumentObjectType::ComicBookSynopsis: {
        if (item == nullptr) {
            break;
        }

        return scriptTextModel(item->parent());
    }

    case Domain::DocumentObjectType::Screenplay: {
        return scriptTextModel(item);
    }
    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis: {
        if (item == nullptr) {
            break;
        }

        return scriptTextModel(item->parent());
    }

    case Domain::DocumentObjectType::Stageplay: {
        return scriptTextModel(item);
    }
    case Domain::DocumentObjectType::StageplayTitlePage:
    case Domain::DocumentObjectType::StageplaySynopsis: {
        if (item == nullptr) {
            break;
        }

        return scriptTextModel(item->parent());
    }

    default: {
        break;
    }
    }

    return d->currentDocument.model;
}

BusinessLayer::AbstractModel* ProjectManager::firstScriptModel() const
{
    auto model = d->projectStructureModel;
    for (auto row = 0; row < model->rowCount(); ++row) {
        const auto index = model->index(row, 0);
        const auto item = d->projectStructureModel->itemForIndex(index);
        if (item != nullptr
            && (item->type() == Domain::DocumentObjectType::Audioplay
                || item->type() == Domain::DocumentObjectType::ComicBook
                || item->type() == Domain::DocumentObjectType::Screenplay
                || item->type() == Domain::DocumentObjectType::Stageplay)) {
            constexpr int scriptTextIndex = 2;
            return d->modelsFacade.modelFor(item->childAt(scriptTextIndex)->uuid());
        }
    }

    return nullptr;
}

bool ProjectManager::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::IdleEvent): {
        //
        // Выполняем отложенную работу для всех моделей, где она имеет место быть
        //
        // ... для сценариев корректируем список подсказок имён персонажей
        //
        auto models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::ScreenplayText);
        for (auto model : std::as_const(models)) {
            auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
            screenplay->updateRuntimeDictionariesIfNeeded();
        }
        //
        models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::ComicBookText);
        for (auto model : std::as_const(models)) {
            auto comicBook = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
            comicBook->updateRuntimeDictionariesIfNeeded();
        }
        //
        models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::AudioplayText);
        for (auto model : std::as_const(models)) {
            auto audioplay = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
            audioplay->updateRuntimeDictionariesIfNeeded();
        }
        //
        models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::StageplayText);
        for (auto model : std::as_const(models)) {
            auto stageplay = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
            stageplay->updateRuntimeDictionariesIfNeeded();
        }
        break;
    }

    case static_cast<QEvent::Type>(EventType::FocusChangeEvent): {
        //
        // При смене сфокусированного виджета, проверяем не изменился ли активный из редакторов
        //

        if (!d->view.right->isVisible()) {
            break;
        }

        auto targetWidget = QApplication::focusWidget();
        if (targetWidget == nullptr) {
            break;
        }

        do {
            if (targetWidget == d->view.left) {
                if (d->view.active != d->view.left) {
                    d->switchViews();
                }
                break;
            } else if (targetWidget == d->view.right) {
                if (d->view.active != d->view.right) {
                    d->switchViews();
                }
                break;
            }

            targetWidget = targetWidget->parentWidget();
        } while (targetWidget != nullptr);
        break;
    }

    default:
        break;
    }

    return QObject::event(_event);
}

bool ProjectManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (static_cast<EventType>(_event->type()) == EventType::DesignSystemChangeEvent
        && _watched == d->view.active) {
        for (auto window : std::as_const(d->view.windows)) {
            QCoreApplication::sendEvent(window, _event);
        }
    }

    return QObject::eventFilter(_watched, _event);
}

void ProjectManager::handleModelChange(BusinessLayer::AbstractModel* _model,
                                       const QByteArray& _undo, const QByteArray& _redo)
{
    using namespace DataStorageLayer;
    StorageFacade::documentChangeStorage()->appendDocumentChange(
        _model->document()->uuid(), QUuid::createUuid(), _undo, _redo,
        StorageFacade::settingsStorage()->accountName(),
        StorageFacade::settingsStorage()->accountEmail());

    emit contentsChanged();
}

void ProjectManager::undoModelChange(BusinessLayer::AbstractModel* _model, int _undoStep)
{
    const auto change = DataStorageLayer::StorageFacade::documentChangeStorage()->documentChangeAt(
        _model->document()->uuid(), _undoStep);
    if (change == nullptr) {
        return;
    }

    _model->undoChange(change->undoPatch(), change->redoPatch());
}

void ProjectManager::showView(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    Log::info("Activate plugin \"%1\"", _viewMimeType);

    //
    // Сохранить состояние отображения версий для текущего документа
    //
    if (d->currentDocument.model != nullptr && d->currentDocument.model->document() != nullptr) {
        setSettingsValue(
            documentSettingsKey(d->currentDocument.model->document()->uuid(), kVersionsVisibleKey),
            d->showVersionsAction->isChecked());
    }

    if (!_itemIndex.isValid()) {
        updateCurrentDocument(nullptr, {});
        d->view.active->setVersionsVisible(false);
        d->view.active->showDefaultPage();
        return;
    }

    const auto sourceItemIndex = d->projectStructureProxyModel->mapToSource(_itemIndex);
    const auto item = d->projectStructureModel->itemForIndex(sourceItemIndex);

    //
    // Определим модель
    //
    updateCurrentDocument(d->modelsFacade.modelFor(item->uuid()), _viewMimeType);
    if (d->currentDocument.model == nullptr) {
        d->view.active->showNotImplementedPage();
        d->view.active->setVersionsVisible(false);
        return;
    }

    //
    // Определим представление и отобразим
    //
    Ui::IDocumentView* view = nullptr;
    if (d->view.active == d->view.left) {
        view = d->pluginsBuilder.activateView(_viewMimeType, d->currentDocument.model);
    } else {
        view = d->pluginsBuilder.activateSecondView(_viewMimeType, d->currentDocument.model);
    }
    if (view == nullptr) {
        d->view.active->showNotImplementedPage();
        d->view.active->setVersionsVisible(false);
        return;
    }
    d->view.active->setDocumentVersions(item->versions());
    d->view.active->showEditor(view->asQWidget());
    d->view.activeIndex = sourceItemIndex;

    //
    // Настроим опции редактора
    //
    auto viewOptions = view->options();
    if (isTextItem(item)) {
        viewOptions.prepend(d->showVersionsAction);
    }
    d->toolBar->setOptions(viewOptions, AppBarOptionsLevel::View);

    //
    // Настроим возможность перехода в навигатор
    //
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(_viewMimeType);
    d->projectStructureModel->setNavigatorAvailableFor(sourceItemIndex,
                                                       !navigatorMimeType.isEmpty());

    //
    // Если в данный момент отображён кастомный навигатов, откроем навигатор соответствующий
    // редактору
    //
    if (!d->navigator->isProjectNavigatorShown() && d->view.active == d->view.left) {
        showNavigator(_itemIndex, _viewMimeType);
    }

    //
    // Настроим уведомления плагина
    //
    auto documentManager = d->pluginsBuilder.plugin(_viewMimeType)->asQObject();
    if (documentManager) {
        const auto invalidSignalIndex = -1;
        if (documentManager->metaObject()->indexOfSignal("upgradeRequested()")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(upgradeRequested()), this, SIGNAL(upgradeRequested()),
                    Qt::UniqueConnection);
        }
    }

    //
    // Отобразим версии документа, если необходимо
    //
    d->showVersionsAction->setChecked(
        settingsValue(
            documentSettingsKey(d->currentDocument.model->document()->uuid(), kVersionsVisibleKey),
            false)
            .toBool());

    //
    // Фокусируем представление
    //
    QTimer::singleShot(d->view.active->animationDuration() * 1.3, this,
                       [this] { d->view.active->setFocus(); });
}

void ProjectManager::showViewForVersion(BusinessLayer::StructureModelItem* _item)
{
    const auto viewMimeType = d->currentDocument.viewMimeType;

    //
    // Определим модель
    //
    updateCurrentDocument(d->modelsFacade.modelFor(_item->uuid()), viewMimeType);
    if (d->currentDocument.model == nullptr) {
        d->view.active->showNotImplementedPage();
        return;
    }

    //
    // Определим представление и отобразим
    //
    Ui::IDocumentView* view = nullptr;
    if (d->view.active == d->view.left) {
        view = d->pluginsBuilder.activateView(viewMimeType, d->currentDocument.model);
    } else {
        view = d->pluginsBuilder.activateSecondView(viewMimeType, d->currentDocument.model);
    }
    if (view == nullptr) {
        d->view.active->showNotImplementedPage();
        return;
    }
    d->view.active->showEditor(view->asQWidget());

    //
    // Если в данный момент отображён кастомный навигатов, откроем навигатор соответствующий
    // редактору
    //
    if (!d->navigator->isProjectNavigatorShown() && d->view.active == d->view.left) {
        showNavigatorForVersion(_item);
    }
}

void ProjectManager::showNavigator(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    const auto mappedItemIndex = d->projectStructureProxyModel->mapToSource(_itemIndex);
    if (!mappedItemIndex.isValid()) {
        d->navigator->showProjectNavigator();
        return;
    }

    const auto item = d->projectStructureModel->itemForIndex(mappedItemIndex);

    //
    // Определим модель
    //
    auto model = d->modelsFacade.modelFor(item->uuid());
    if (model == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Определим представление и отобразим
    //
    const auto viewMimeType
        = !_viewMimeType.isEmpty() ? _viewMimeType : d->toolBar->currentViewMimeType();
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(viewMimeType);
    auto view = d->pluginsBuilder.activateView(navigatorMimeType, model);
    if (view == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Связываем редактор с навигатоом
    //
    d->pluginsBuilder.bind(viewMimeType, navigatorMimeType);

    //
    // Настроим опции редактора
    //
    d->toolBar->setOptions(view->options(), AppBarOptionsLevel::Navigation);

    //
    // Настраиваем возможность перехода к навигатору проекта
    //
    auto navigatorView = qobject_cast<Ui::AbstractNavigator*>(view->asQWidget());
    connect(navigatorView, &Ui::AbstractNavigator::backPressed, d->navigator,
            &Ui::ProjectNavigator::showProjectNavigator, Qt::UniqueConnection);
    connect(navigatorView, &Ui::AbstractNavigator::backPressed, d->toolBar,
            &Ui::ProjectToolBar::clearNavigatorOptions, Qt::UniqueConnection);
    d->navigator->setCurrentWidget(navigatorView);
}

void ProjectManager::showNavigatorForVersion(BusinessLayer::StructureModelItem* _item)
{
    //
    // Определим модель
    //
    auto model = d->modelsFacade.modelFor(_item->uuid());
    if (model == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Определим представление и отобразим
    //
    const auto viewMimeType = d->currentDocument.viewMimeType;
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(viewMimeType);
    auto view = d->pluginsBuilder.activateView(navigatorMimeType, model);
    if (view == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }
}

void ProjectManager::updateCurrentDocument(BusinessLayer::AbstractModel* _model,
                                           const QString& _viewMimeType)
{
    d->currentDocument.model = _model;
    d->currentDocument.viewMimeType = _viewMimeType;

    emit currentModelChanged(d->currentDocument.model);
}

} // namespace ManagementLayer
