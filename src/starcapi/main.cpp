#include <business_layer/import/screenplay/screenlay_import_options.h>
#include <business_layer/import/screenplay/screenplay_document_importer.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/database.h>
#include <data_layer/storage/document_data_storage.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

using namespace BusinessLayer;

namespace {

enum class Method {
    //
    // Сохранить заданный документ
    //
    Stories,
    //
    // Получить список персонажей
    //
    Characters,
    //
    // Получить информацию по персонажу
    //
    Character,
};

enum class Error {
    DocumentNotSet,
    DocumentNotFound,
    CharacterNotFound,
};

const QHash<QString, Method> kMethodsDictionary = {
    { "stories", Method::Stories },
    { "characters", Method::Characters },
    { "character", Method::Character },
};

QJsonObject errorResult(Error _error)
{
    QJsonObject result;
    result["error_code"] = static_cast<int>(_error);
    QString errorMessage;
    switch (_error) {
    case Error::DocumentNotSet:
        errorMessage = "Document not set";
        break;
    case Error::DocumentNotFound:
        errorMessage = "Document not found";
        break;
    case Error::CharacterNotFound:
        errorMessage = "Character not found";
        break;
    default:
        Q_ASSERT(false);
    }
    result["error_message"] = errorMessage;
    return result;
}

/**
 * @brief Импортировать история из заданного файла
 */
QJsonObject postStories(const QStringList& _importFileNames)
{
    if (_importFileNames.isEmpty()) {
        return errorResult(Error::DocumentNotSet);
    }

    const auto starcFileUuid = QUuid::createUuid().toString();
    const auto starcFileName = starcFileUuid + ".starc";
    const auto starcFilePath = "/tmp/" + starcFileName;
    QDir tmp("/tmp");

    //
    // Создаём проект старка и импортируем в него все документы
    //

    QScopedPointer<BusinessLayer::ScreenplayAbstractImporter> importer;
    importer.reset(new BusinessLayer::ScreenplayDocumentImporter);

    DataStorageLayer::DocumentDataStorage documentDataStorage;
    QVector<Domain::DocumentObject*> documents;
    QVector<BusinessLayer::AbstractModel*> models;

    for (const auto& importFileName : _importFileNames) {
        //
        // Подготовимся к импорту
        //
        const QFileInfo importFileInfo("/tmp/" + importFileName);
        DatabaseLayer::Database::setCurrentFile(starcFilePath);
        BusinessLayer::ScreenplayImportOptions importOptions;
        importOptions.filePath = importFileInfo.absoluteFilePath();

        //
        // Импортируем персонажей
        //
        const auto screenplayDocuments = importer->importDocuments(importOptions);
        for (const auto& character : screenplayDocuments.characters) {
            auto characterDocument
                = DataStorageLayer::StorageFacade::documentStorage()->createDocument(
                    QUuid::createUuid(), Domain::DocumentObjectType::Character);
            characterDocument->setContent(character.content.toUtf8());
            auto characterModel = new BusinessLayer::CharacterModel;
            characterModel->setImageWrapper(&documentDataStorage);
            characterModel->setDocument(characterDocument);
            characterModel->setName(character.name);

            documents.append(characterDocument);
            models.append(characterModel);
        }
        //
        // ... локации
        //
        for (const auto& location : screenplayDocuments.locations) {
            auto characterDocument
                = DataStorageLayer::StorageFacade::documentStorage()->createDocument(
                    QUuid::createUuid(), Domain::DocumentObjectType::Location);
            characterDocument->setContent(location.content.toUtf8());
            auto characterModel = new BusinessLayer::CharacterModel;
            characterModel->setImageWrapper(&documentDataStorage);
            characterModel->setDocument(characterDocument);
            characterModel->setName(location.name);

            documents.append(characterDocument);
            models.append(characterModel);
        }
        //
        // ... сценарии
        //
        const auto screenplays = importer->importScreenplays(importOptions);
        for (const auto& screenplay : screenplays) {
            auto screenplayDocument
                = DataStorageLayer::StorageFacade::documentStorage()->createDocument(
                    QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText);
            screenplayDocument->setContent(screenplay.text.toUtf8());

            documents.append(screenplayDocument);
        }

        //
        // Удаляем исходный файл
        //
        QFile::remove(importFileInfo.absoluteFilePath());
    }

    //
    // Ждём, чтобы изменения документов применились под дебаунсером
    //
    QEventLoop eventLoop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    timer.start(std::chrono::seconds{ 1 });
    eventLoop.exec();

    //
    // Сохраняем
    //
    for (auto document : documents) {
        DataStorageLayer::StorageFacade::documentStorage()->saveDocument(document);
    }

    qDeleteAll(models);

    DatabaseLayer::Database::closeCurrentFile();
    DataStorageLayer::StorageFacade::clearStorages();

    //
    // Формируем ответ с краткой сводкой
    //
    DatabaseLayer::Database::setCurrentFile(starcFilePath);
    QJsonObject result;
    result["uid"] = starcFileUuid;
    result["characters_count"] = DataStorageLayer::StorageFacade::documentStorage()
                                     ->documents(Domain::DocumentObjectType::Character)
                                     .size();
    result["series_count"] = DataStorageLayer::StorageFacade::documentStorage()
                                 ->documents(Domain::DocumentObjectType::ScreenplayText)
                                 .size();
    DatabaseLayer::Database::closeCurrentFile();
    return result;
}

QJsonObject getCharacters(const QString& _starcFileName)
{
    const auto starcFilePath = "/tmp/" + _starcFileName + ".starc";
    if (!QFile::exists(starcFilePath)) {
        return errorResult(Error::DocumentNotFound);
    }

    DatabaseLayer::Database::setCurrentFile(starcFilePath);
    const auto charactersDocuments = DataStorageLayer::StorageFacade::documentStorage()->documents(
        Domain::DocumentObjectType::Character);

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    DataStorageLayer::DocumentDataStorage documentDataStorage;
    QString rxPattern;
    for (auto characterDocument : charactersDocuments) {
        CharacterModel characterModel;
        characterModel.setImageWrapper(&documentDataStorage);
        characterModel.setDocument(characterDocument);
        if (!rxPattern.isEmpty()) {
            rxPattern.append("|");
        }
        rxPattern.append(characterModel.name());
    }
    rxPattern.prepend("(^|\\W)(");
    rxPattern.append(")($|\\W)");
    const QRegularExpression rxCharacterFinder(
        rxPattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);

    //
    // Бежим по сценариям и собираем информацию о сценах и персонажах в них
    //
    struct CharacterData {
        QString name;
        int nonspeakingScenesCount = 0;
        int speakingScenesCount = 0;
        int dialoguesCount = 0;
    };
    QVector<CharacterData> charactersStatistics;
    QStringList characters;
    QStringList sceneSpeakingCharacters;
    QStringList sceneNonspeakingCharacters;
    std::function<void(const ScreenplayTextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &charactersStatistics, &characters,
                       &sceneSpeakingCharacters, &sceneNonspeakingCharacters,
                       &rxCharacterFinder](const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case ScreenplayTextModelItemType::Folder:
            case ScreenplayTextModelItemType::Scene: {
                includeInReport(childItem);
                break;
            }

            case ScreenplayTextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                switch (textItem->paragraphType()) {
                //
                // Сцена, всё очищаем
                //
                case ScreenplayParagraphType::SceneHeading: {
                    sceneSpeakingCharacters.clear();
                    sceneNonspeakingCharacters.clear();
                    break;
                }

                //
                // Список персонажей, всех в молчаливых
                //
                case ScreenplayParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        if (!characters.contains(character)) {
                            characters.append(character);
                            sceneNonspeakingCharacters.append(character);
                            charactersStatistics.append({ character });
                            charactersStatistics.last().nonspeakingScenesCount += 1;
                        } else {
                            //
                            // Если он ещё не добавлен в текущую сцену
                            //
                            if (!sceneSpeakingCharacters.contains(character)
                                && !sceneNonspeakingCharacters.contains(character)) {
                                sceneNonspeakingCharacters.append(character);
                                charactersStatistics[characters.indexOf(character)]
                                    .nonspeakingScenesCount
                                    += 1;
                            }
                        }
                    }
                    break;
                }

                //
                // Персонаж +1 реплика и в список говорящих
                //
                case ScreenplayParagraphType::Character: {
                    const QString character = ScreenplayCharacterParser::name(textItem->text());
                    //
                    // Если имя персонажа задано
                    //
                    if (!character.isEmpty()) {
                        //
                        // Если это первое обнаружение персонажа
                        //
                        if (!characters.contains(character)) {
                            characters.append(character);
                            sceneSpeakingCharacters.append(character);
                            charactersStatistics.append({ character });
                            charactersStatistics.last().speakingScenesCount += 1;
                            charactersStatistics.last().dialoguesCount += 1;
                        }
                        //
                        // А если персонаж уже был обнаружен ранее
                        //
                        else {
                            //
                            // Если он уже был и в говорящих
                            //
                            if (sceneSpeakingCharacters.contains(character)) {
                                //
                                // Просто увеличиваем число реплик
                                //
                                charactersStatistics[characters.indexOf(character)].dialoguesCount
                                    += 1;
                            }
                            //
                            // Если был в молчаливых
                            //
                            else if (sceneNonspeakingCharacters.contains(character)) {
                                //
                                // То перемещаем в говорящих
                                //
                                sceneNonspeakingCharacters.removeAll(character);
                                sceneSpeakingCharacters.append(character);
                                charactersStatistics[characters.indexOf(character)]
                                    .nonspeakingScenesCount
                                    -= 1;
                                charactersStatistics[characters.indexOf(character)]
                                    .speakingScenesCount
                                    += 1;
                                charactersStatistics[characters.indexOf(character)].dialoguesCount
                                    += 1;
                            }
                            //
                            // Если не было в сцене
                            //
                            else {
                                sceneSpeakingCharacters.append(character);
                                charactersStatistics[characters.indexOf(character)]
                                    .speakingScenesCount
                                    += 1;
                                charactersStatistics[characters.indexOf(character)].dialoguesCount
                                    += 1;
                            }
                        }
                    }
                    break;
                }

                //
                // Описание действия, в молчаливые, если ещё не встречался
                //
                case ScreenplayParagraphType::Action: {
                    QRegularExpressionMatch match = rxCharacterFinder.match(textItem->text());
                    while (match.hasMatch()) {
                        const QString character = TextHelper::smartToUpper(match.captured(2));
                        if (!characters.contains(character)) {
                            characters.append(character);
                            sceneNonspeakingCharacters.append(character);
                            charactersStatistics.append({ character });
                            charactersStatistics.last().nonspeakingScenesCount += 1;
                        } else {
                            //
                            // Если он ещё не добавлен в текущую сцену
                            //
                            if (!sceneSpeakingCharacters.contains(character)
                                && !sceneNonspeakingCharacters.contains(character)) {
                                sceneNonspeakingCharacters.append(character);
                                charactersStatistics[characters.indexOf(character)]
                                    .nonspeakingScenesCount
                                    += 1;
                            }
                        }

                        //
                        // Ищем дальше
                        //
                        match = rxCharacterFinder.match(textItem->text(), match.capturedEnd());
                    }
                    break;
                }

                default:
                    break;
                }

                break;
            }

            default:
                break;
            }
        }
    };
    const auto screenplayDocuments = DataStorageLayer::StorageFacade::documentStorage()->documents(
        Domain::DocumentObjectType::ScreenplayText);
    for (auto screenplayDocument : screenplayDocuments) {
        ScreenplayTextModel screenplayModel;
        ScreenplayInformationModel informationsModel;
        screenplayModel.setInformationModel(&informationsModel);
        screenplayModel.setDocument(screenplayDocument);
        includeInReport(screenplayModel.itemForIndex({}));
    }
    DatabaseLayer::Database::closeCurrentFile();

    //
    // Сортируем по количеству сцен
    //
    std::sort(charactersStatistics.begin(), charactersStatistics.end(),
              [](const CharacterData& _lhs, const CharacterData& _rhs) {
                  return _lhs.speakingScenesCount + _lhs.nonspeakingScenesCount
                      > _rhs.speakingScenesCount + _rhs.nonspeakingScenesCount;
              });

    QJsonObject result;
    result["characters_count"] = charactersStatistics.size();
    QJsonArray charactersJson;
    for (const auto& characterStatistics : charactersStatistics) {
        QJsonObject character;
        character["name"] = characterStatistics.name;
        QJsonObject scenesCount;
        scenesCount["speaking"] = characterStatistics.speakingScenesCount;
        scenesCount["nonspeaking"] = characterStatistics.nonspeakingScenesCount;
        scenesCount["total"]
            = characterStatistics.speakingScenesCount + characterStatistics.nonspeakingScenesCount;
        character["scenes_count"] = scenesCount;
        character["dialogues_count"] = characterStatistics.dialoguesCount;
        charactersJson.append(character);
    }
    result["characters"] = charactersJson;
    return result;
}

QJsonObject getCharacter(const QString& _starcFileName, const QString& _characterName)
{
    const auto starcFilePath = "/tmp/" + _starcFileName + ".starc";
    if (!QFile::exists(starcFilePath)) {
        return errorResult(Error::DocumentNotFound);
    }

    if (_characterName.isEmpty()) {
        return errorResult(Error::CharacterNotFound);
    }

    DatabaseLayer::Database::setCurrentFile(starcFilePath);
    const auto screenplayDocuments = DataStorageLayer::StorageFacade::documentStorage()->documents(
        Domain::DocumentObjectType::ScreenplayText);
    QJsonObject result;
    QJsonArray screenplays;
    struct SceneData {
        QString uuid;
        QString heading;
        std::chrono::seconds duration;
        int dialoguesCount = 0;
        std::chrono::seconds dialoguesDuration;
        bool characterPresented = false;
    };
    QVector<SceneData> scenesData;
    QString lastCharacter;
    const QRegularExpression rxCharacterFinder(
        "(^|\\W)(" + _characterName + ")($|\\W)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    std::function<void(const ScreenplayTextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &scenesData, &lastCharacter, &rxCharacterFinder,
                       _characterName](const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case ScreenplayTextModelItemType::Folder: {
                includeInReport(childItem);
                break;
            }

            case ScreenplayTextModelItemType::Scene: {
                scenesData.append(SceneData());
                auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(childItem);
                scenesData.last().uuid = sceneItem->uuid().toString();
                scenesData.last().heading = sceneItem->heading();
                scenesData.last().duration
                    = std::chrono::duration_cast<std::chrono::seconds>(sceneItem->duration());
                includeInReport(childItem);

                lastCharacter.clear();
                break;
            }

            case ScreenplayTextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                switch (textItem->paragraphType()) {

                //
                // Список персонажей, ищем нашего среди всех
                //
                case ScreenplayParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    if (sceneCharacters.contains(_characterName, Qt::CaseInsensitive)) {
                        scenesData.last().characterPresented = true;
                    }
                    break;
                }

                //
                // Персонаж, запоним, кто говорит
                //
                case ScreenplayParagraphType::Character: {
                    lastCharacter = ScreenplayCharacterParser::name(textItem->text());
                    if (lastCharacter.compare(_characterName, Qt::CaseInsensitive) == 0) {
                        scenesData.last().characterPresented = true;
                        ++scenesData.last().dialoguesCount;
                        scenesData.last().dialoguesDuration
                            += std::chrono::duration_cast<std::chrono::seconds>(
                                textItem->duration());
                    }
                    break;
                }

                //
                // Реплика, или лирика, если говорит искомый, обновим счётчики
                //
                case ScreenplayParagraphType::Dialogue:
                case ScreenplayParagraphType::Lyrics: {
                    if (lastCharacter.compare(_characterName, Qt::CaseInsensitive) == 0) {
                        scenesData.last().dialoguesDuration
                            += std::chrono::duration_cast<std::chrono::seconds>(
                                textItem->duration());
                    }
                    break;
                }

                //
                // Описание действия, возможно персонаж упоминается
                //
                case ScreenplayParagraphType::Action: {
                    QRegularExpressionMatch match = rxCharacterFinder.match(textItem->text());
                    if (match.hasMatch()) {
                        scenesData.last().characterPresented = true;
                    }
                    break;
                }

                default:
                    break;
                }

                break;
            }

            default:
                break;
            }
        }
    };
    for (auto screenplayDocument : screenplayDocuments) {
        QJsonObject screenplay;
        screenplay["uid"] = screenplayDocument->uuid().toString();
        scenesData.clear();
        scenesData.append(SceneData());

        ScreenplayTextModel screenplayModel;
        ScreenplayInformationModel informationsModel;
        screenplayModel.setInformationModel(&informationsModel);
        screenplayModel.setDocument(screenplayDocument);
        includeInReport(screenplayModel.itemForIndex({}));

        QJsonArray scenes;
        for (const auto& sceneData : scenesData) {
            if (!sceneData.characterPresented) {
                continue;
            }

            QJsonObject scene;
            scene["uid"] = sceneData.uuid;
            scene["heading"] = sceneData.heading;
            scene["duration"] = static_cast<int>(sceneData.duration.count());
            scene["dialogues_count"] = sceneData.dialoguesCount;
            scene["dialogues_duration"] = static_cast<int>(sceneData.dialoguesDuration.count());
            scenes.append(scene);
        }

        screenplay["scenes"] = scenes;
        screenplay["scenes_count"] = scenes.size();
        screenplays.append(screenplay);
    }
    result["screenplays"] = screenplays;
    result["screenplays_count"] = screenplayDocuments.size();
    return result;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    auto arguments = a.arguments();
    arguments.removeFirst();
    QJsonObject result;
    switch (kMethodsDictionary.value(arguments.takeFirst())) {
    case Method::Stories: {
        result = postStories(arguments);
        break;
    }

    case Method::Characters: {
        result = getCharacters(arguments.constFirst());
        break;
    }

    case Method::Character: {
        result = getCharacter(arguments.constFirst(), arguments.constLast());
        break;
    }

    default: {
        Q_ASSERT(false);
    }
    }

    qDebug() << qUtf8Printable(QJsonDocument(result).toJson(QJsonDocument::Indented));

    return 0;
}
