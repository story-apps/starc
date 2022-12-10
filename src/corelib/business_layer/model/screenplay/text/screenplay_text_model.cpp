#include "screenplay_text_model.h"

#include "screenplay_text_block_parser.h"
#include "screenplay_text_model_beat_item.h"
#include "screenplay_text_model_folder_item.h"
#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_text_item.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QRegularExpression>
#include <QStringListModel>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/screenplay/text/item";
}

class ScreenplayTextModel::Implementation
{
public:
    explicit Implementation(ScreenplayTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Пересчитать хронометраж элемента и всех детей
     */
    void updateChildrenDuration(const TextModelItem* _item);


    /**
     * @brief Родительский элемент
     */
    ScreenplayTextModel* q = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    ScreenplayInformationModel* informationModel = nullptr;

    /**
     * @brief Модель справочников
     */
    ScreenplayDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Модель локаций
     */
    LocationsModel* locationsModel = nullptr;

    /**
     * @brief Нужно ли обновить справочники, которые строятся в рантайме
     */
    bool needUpdateRuntimeDictionaries = false;

    /**
     * @brief Справочники, которые строятся в рантайме
     */
    QStringListModel* charactersModelFromText = nullptr;

    /**
     * @brief Количество страниц
     */
    int treatmentPageCount = 0;
    int scriptPageCount = 0;

    /**
     * @brief Количество сцен
     */
    int scenesCount = 0;
};

ScreenplayTextModel::Implementation::Implementation(ScreenplayTextModel* _q)
    : q(_q)
{
}

TextModelItem* ScreenplayTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void ScreenplayTextModel::Implementation::updateChildrenDuration(const TextModelItem* _item)
{
    if (_item == nullptr) {
        return;
    }

    for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        switch (childItem->type()) {
        case TextModelItemType::Folder:
        case TextModelItemType::Group: {
            updateChildrenDuration(childItem);
            break;
        }

        case TextModelItemType::Text: {
            auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
            textItem->updateCounters();
            break;
        }

        default:
            break;
        }
    }
}


// ****


ScreenplayTextModel::ScreenplayTextModel(QObject* _parent)
    : TextModel(_parent, ScreenplayTextModel::createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    auto updateCounters = [this](const QModelIndex& _index) {
        updateNumbering();
        d->updateChildrenDuration(itemForIndex(_index));
    };
    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &ScreenplayTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &ScreenplayTextModel::afterRowsRemoved, this, updateCounters);

    connect(this, &ScreenplayTextModel::contentsChanged, this,
            [this] { d->needUpdateRuntimeDictionaries = true; });
}

ScreenplayTextModel::~ScreenplayTextModel() = default;

TextModelFolderItem* ScreenplayTextModel::createFolderItem(TextFolderType _type) const
{
    return new ScreenplayTextModelFolderItem(this, _type);
}

TextModelGroupItem* ScreenplayTextModel::createGroupItem(TextGroupType _type) const
{
    switch (_type) {
    case TextGroupType::Scene: {
        return new ScreenplayTextModelSceneItem(this);
    }

    case TextGroupType::Beat: {
        return new ScreenplayTextModelBeatItem(this);
    }

    default: {
        Q_ASSERT(false);
        return nullptr;
    }
    }
}

TextModelTextItem* ScreenplayTextModel::createTextItem() const
{
    return new ScreenplayTextModelTextItem(this);
}

QStringList ScreenplayTextModel::mimeTypes() const
{
    return { kMimeType };
}

void ScreenplayTextModel::setInformationModel(ScreenplayInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        d->informationModel->disconnect(this);
    }

    d->informationModel = _model;

    if (d->informationModel) {
        connect(d->informationModel, &ScreenplayInformationModel::scenesNumberingStartAtChanged,
                this, &ScreenplayTextModel::updateNumbering);
        connect(d->informationModel, &ScreenplayInformationModel::scenesNumbersTemplateChanged,
                this, &ScreenplayTextModel::updateNumbering);
        connect(d->informationModel, &ScreenplayInformationModel::isSceneNumbersLockedChanged, this,
                &ScreenplayTextModel::setScenesNumbersLocked);
    }
}

ScreenplayInformationModel* ScreenplayTextModel::informationModel() const
{
    return d->informationModel;
}

void ScreenplayTextModel::setDictionariesModel(ScreenplayDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

ScreenplayDictionariesModel* ScreenplayTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void ScreenplayTextModel::setCharactersModel(CharactersModel* _model)
{
    if (d->charactersModel) {
        d->charactersModel->disconnect(this);
    }

    d->charactersModel = _model;

    connect(d->charactersModel, &CharactersModel::contentsChanged, this,
            [this] { d->needUpdateRuntimeDictionaries = true; });
}

QAbstractItemModel* ScreenplayTextModel::charactersModel() const
{
    if (d->charactersModelFromText != nullptr) {
        return d->charactersModelFromText;
    }

    return d->charactersModel;
}

BusinessLayer::CharacterModel* ScreenplayTextModel::character(const QString& _name) const
{
    return d->charactersModel->character(_name);
}

void ScreenplayTextModel::createCharacter(const QString& _name)
{
    d->charactersModel->createCharacter(_name);
}

void ScreenplayTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
{
    const auto oldName = TextHelper::smartToUpper(_oldName);
    std::function<void(const TextModelItem*)> updateCharacterBlock;
    updateCharacterBlock = [this, oldName, _newName,
                            &updateCharacterBlock](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                updateCharacterBlock(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneCharacters
                    && ScreenplaySceneCharactersParser::characters(textItem->text())
                           .contains(oldName)) {
                    auto text = textItem->text();
                    auto nameIndex = TextHelper::smartToUpper(text).indexOf(oldName);
                    while (nameIndex != -1) {
                        //
                        // Убедимся, что выделено именно имя, а не часть другого имени
                        //
                        const auto nameEndIndex = nameIndex + oldName.length();
                        const bool atLeftAllOk = nameIndex == 0 || text.at(nameIndex - 1) == ','
                            || (nameIndex > 2 && text.mid(nameIndex - 2, 2) == ", ");
                        const bool atRightAllOk = nameEndIndex == text.length()
                            || text.at(nameEndIndex) == ','
                            || (text.length() > nameEndIndex + 1
                                && text.mid(nameEndIndex, 2) == " ,");
                        if (!atLeftAllOk || !atRightAllOk) {
                            nameIndex = TextHelper::smartToUpper(text).indexOf(oldName, nameIndex);
                            continue;
                        }

                        text.remove(nameIndex, oldName.length());
                        text.insert(nameIndex, _newName);
                        textItem->setText(text);
                        updateItem(textItem);
                        break;
                    }
                } else if (textItem->paragraphType() == TextParagraphType::Character
                           && ScreenplayCharacterParser::name(textItem->text()) == oldName) {
                    auto text = textItem->text();
                    text.remove(0, oldName.length());
                    text.prepend(_newName);
                    textItem->setText(text);
                    updateItem(textItem);
                } else if (textItem->text().contains(oldName, Qt::CaseInsensitive)) {
                    auto text = textItem->text();
                    const QRegularExpression nameMatcher(QString("\\b(%1)\\b").arg(oldName),
                                                         QRegularExpression::CaseInsensitiveOption);
                    auto match = nameMatcher.match(text);
                    while (match.hasMatch()) {
                        text.remove(match.capturedStart(), match.capturedLength());
                        const auto capturedName = match.captured();
                        const auto capitalizeEveryWord = true;
                        const auto newName = capturedName == oldName
                            ? TextHelper::smartToUpper(_newName)
                            : TextHelper::toSentenceCase(_newName, capitalizeEveryWord);
                        text.insert(match.capturedStart(), newName);

                        match = nameMatcher.match(text, match.capturedStart() + _newName.length());
                    }

                    textItem->setText(text);
                    updateItem(textItem);
                }
                break;
            }

            default:
                break;
            }
        }
    };

    emit rowsAboutToBeChanged();
    updateCharacterBlock(d->rootItem());
    emit rowsChanged();
}

QVector<QModelIndex> ScreenplayTextModel::characterDialogues(const QString& _name) const
{
    QVector<QModelIndex> modelIndexes;
    for (int row = 0; row < rowCount(); ++row) {
        modelIndexes.append(index(row, 0));
    }
    QString lastCharacter;
    QVector<QModelIndex> dialoguesIndexes;
    while (!modelIndexes.isEmpty()) {
        const auto itemIndex = modelIndexes.takeFirst();
        const auto item = itemForIndex(itemIndex);
        if (item->type() == TextModelItemType::Text) {
            const auto textItem = static_cast<TextModelTextItem*>(item);
            switch (textItem->paragraphType()) {
            case TextParagraphType::Character: {
                lastCharacter = ScreenplayCharacterParser::name(textItem->text());
                break;
            }

            case TextParagraphType::Dialogue:
            case TextParagraphType::Lyrics: {
                if (lastCharacter == _name) {
                    dialoguesIndexes.append(itemIndex);
                }
                break;
            }

            default: {
                lastCharacter.clear();
                break;
            }
            }
        }

        for (int childRow = 0; childRow < rowCount(itemIndex); ++childRow) {
            modelIndexes.append(index(childRow, 0, itemIndex));
        }
    }

    return dialoguesIndexes;
}

QSet<QString> ScreenplayTextModel::findCharactersFromText() const
{
    QSet<QString> characters;
    std::function<void(const TextModelItem*)> findCharacters;
    findCharacters = [&characters, &findCharacters](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                findCharacters(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneCharacters) {
                    const auto textCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : textCharacters) {
                        characters.insert(character);
                    }
                } else if (textItem->paragraphType() == TextParagraphType::Character) {
                    characters.insert(ScreenplayCharacterParser::name(textItem->text()));
                }
                break;
            }

            default:
                break;
            }
        }
    };
    findCharacters(d->rootItem());

    return characters;
}

void ScreenplayTextModel::setLocationsModel(LocationsModel* _model)
{
    d->locationsModel = _model;
}

QAbstractItemModel* ScreenplayTextModel::locationsModel() const
{
    return d->locationsModel;
}

LocationModel* ScreenplayTextModel::location(const QString& _name) const
{
    return d->locationsModel->location(_name);
}

void ScreenplayTextModel::createLocation(const QString& _name)
{
    d->locationsModel->createLocation(_name);
}

QSet<QString> ScreenplayTextModel::findLocationsFromText() const
{
    QSet<QString> locations;
    std::function<void(const TextModelItem*)> findLocations;
    findLocations = [&locations, &findLocations](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                findLocations(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneHeading) {
                    locations.insert(ScreenplaySceneHeadingParser::location(textItem->text()));
                }
                break;
            }

            default:
                break;
            }
        }
    };
    findLocations(d->rootItem());

    return locations;
}

void ScreenplayTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
{
    const auto oldName = TextHelper::smartToUpper(_oldName);
    std::function<void(const TextModelItem*)> updateLocationBlock;
    updateLocationBlock
        = [this, oldName, _newName, &updateLocationBlock](const TextModelItem* _item) {
              for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                  auto childItem = _item->childAt(childIndex);
                  switch (childItem->type()) {
                  case TextModelItemType::Folder:
                  case TextModelItemType::Group: {
                      updateLocationBlock(childItem);
                      break;
                  }

                  case TextModelItemType::Text: {
                      auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                      if (textItem->paragraphType() == TextParagraphType::SceneHeading
                          && ScreenplaySceneHeadingParser::location(textItem->text()) == oldName) {
                          auto text = textItem->text();
                          const auto nameIndex = TextHelper::smartToUpper(text).indexOf(oldName);
                          text.remove(nameIndex, oldName.length());
                          text.insert(nameIndex, _newName);
                          textItem->setText(text);
                          updateItem(textItem);
                      }
                      break;
                  }

                  default:
                      break;
                  }
              }
          };

    emit rowsAboutToBeChanged();
    updateLocationBlock(d->rootItem());
    emit rowsChanged();
}

int ScreenplayTextModel::treatmentPageCount() const
{
    return d->treatmentPageCount;
}

void ScreenplayTextModel::setTreatmentPageCount(int _count)
{
    if (d->treatmentPageCount == _count) {
        return;
    }

    d->treatmentPageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

int ScreenplayTextModel::scriptPageCount() const
{
    return d->scriptPageCount;
}

void ScreenplayTextModel::setScriptPageCount(int _count)
{
    if (d->scriptPageCount == _count) {
        return;
    }

    d->scriptPageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

int ScreenplayTextModel::scenesCount() const
{
    return d->scenesCount;
}

int ScreenplayTextModel::wordsCount() const
{
    return static_cast<ScreenplayTextModelFolderItem*>(d->rootItem())->wordsCount();
}

QPair<int, int> ScreenplayTextModel::charactersCount() const
{
    return static_cast<ScreenplayTextModelFolderItem*>(d->rootItem())->charactersCount();
}

std::chrono::milliseconds ScreenplayTextModel::duration() const
{
    return static_cast<ScreenplayTextModelFolderItem*>(d->rootItem())->duration();
}

std::map<std::chrono::milliseconds, QColor> ScreenplayTextModel::itemsColors() const
{
    std::chrono::milliseconds lastItemDuration{ 0 };
    std::map<std::chrono::milliseconds, QColor> colors;
    std::function<void(const TextModelItem*)> collectChildColors;
    collectChildColors = [&collectChildColors, &lastItemDuration,
                          &colors](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder: {
                collectChildColors(childItem);
                break;
            }

            case TextModelItemType::Group: {
                const auto sceneItem = static_cast<const ScreenplayTextModelSceneItem*>(childItem);
                colors.emplace(lastItemDuration, sceneItem->color());
                lastItemDuration += sceneItem->duration();
                break;
            }

            default:
                break;
            }
        }
    };
    collectChildColors(d->rootItem());
    return colors;
}

std::map<std::chrono::milliseconds, QColor> ScreenplayTextModel::itemsBookmarks() const
{
    std::chrono::milliseconds lastItemDuration{ 0 };
    std::map<std::chrono::milliseconds, QColor> colors;
    std::function<void(const TextModelItem*)> collectChildColors;
    collectChildColors = [&collectChildColors, &lastItemDuration,
                          &colors](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                collectChildColors(childItem);
                break;
            }

            case TextModelItemType::Text: {
                const auto textItem = static_cast<const ScreenplayTextModelTextItem*>(childItem);
                if (textItem->bookmark().has_value() && textItem->bookmark().value().isValid()) {
                    colors.emplace(lastItemDuration, textItem->bookmark()->color);
                }
                lastItemDuration += textItem->duration();
                break;
            }

            default:
                break;
            }
        }
    };
    collectChildColors(d->rootItem());
    return colors;
}

void ScreenplayTextModel::updateNumbering()
{
    d->scenesCount = 0;
    int sceneNumber = d->informationModel->scenesNumberingStartAt();
    int dialogueNumber = 1;
    QString lastLockedSceneFullNumber;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering = [this, &sceneNumber, &dialogueNumber, &lastLockedSceneFullNumber,
                            &updateChildNumbering](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder: {
                updateChildNumbering(childItem);
                break;
            }

            case TextModelItemType::Group: {
                updateChildNumbering(childItem);
                auto groupItem = static_cast<TextModelGroupItem*>(childItem);
                if (groupItem->groupType() == TextGroupType::Scene) {
                    ++d->scenesCount;

                    //
                    // Если у сцены номер заблокирован, то запоминаем последний заблокированный для
                    // работы с номерами незаблокированных сцен и сбрасываем счётчик номеров
                    //
                    if (groupItem->number().has_value() && groupItem->number()->isLocked) {
                        lastLockedSceneFullNumber
                            = groupItem->number()->followNumber + groupItem->number()->value;
                        sceneNumber = 0;
                    }
                    //
                    // Если у сцены задан кастомный номер, то не меняем его
                    //
                    else if (groupItem->number().has_value() && groupItem->number()->isCustom) {
                        //
                        // ... но при необходимости переводим счётчик номеров сцен
                        //
                        if (groupItem->number()->isEatNumber) {
                            ++sceneNumber;
                        }
                    }
                    //
                    // А если номера назначаются автоматически, то задаём очередной номер
                    //
                    else {
                        if (groupItem->setNumber(sceneNumber, lastLockedSceneFullNumber)) {
                            updateItem(groupItem);
                            ++sceneNumber;
                        }
                    }

                    //
                    // После того, как номер сформирован, декорируем его
                    //
                    groupItem->prepareNumberText(d->informationModel->scenesNumbersTemplate());
                }
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::Character
                    && !textItem->isCorrection()) {
                    textItem->setNumber(dialogueNumber);
                    updateItemForRoles(textItem, { TextModelTextItem::TextNumberRole });
                    ++dialogueNumber;
                }
                break;
            }

            default:
                break;
            }
        }
    };
    updateChildNumbering(d->rootItem());
}

void ScreenplayTextModel::setScenesNumbersLocked(bool _locked)
{
    std::function<void(const TextModelItem*)> setSceneNumbersLockedImpl;
    setSceneNumbersLockedImpl
        = [this, _locked, &setSceneNumbersLockedImpl](const TextModelItem* _item) {
              for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                  auto childItem = _item->childAt(childIndex);
                  switch (childItem->type()) {
                  case TextModelItemType::Folder: {
                      setSceneNumbersLockedImpl(childItem);
                      break;
                  }

                  case TextModelItemType::Group: {
                      auto groupItem = static_cast<TextModelGroupItem*>(childItem);
                      if (groupItem->groupType() == TextGroupType::Scene) {
                          if (_locked) {
                              groupItem->lockNumber();
                          } else {
                              groupItem->resetNumber();
                          }
                          updateItem(groupItem);
                      }
                      break;
                  }

                  default:
                      break;
                  }
              }
          };
    setSceneNumbersLockedImpl(d->rootItem());

    //
    // Если номера были разблокированы, то нужно сформировать их заново
    //
    if (!_locked) {
        updateNumbering();
    }
}

void ScreenplayTextModel::recalculateDuration()
{
    emit rowsAboutToBeChanged();
    d->updateChildrenDuration(d->rootItem());
    emit rowsChanged();
}

void ScreenplayTextModel::updateRuntimeDictionariesIfNeeded()
{
    if (!d->needUpdateRuntimeDictionaries) {
        return;
    }

    updateRuntimeDictionaries();

    d->needUpdateRuntimeDictionaries = false;
}

void ScreenplayTextModel::updateRuntimeDictionaries()
{
    const bool useCharactersFromText
        = settingsValue(DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey)
              .toBool();

    //
    // Если нет необходимости собирать персонажей из текста
    //
    if (!useCharactersFromText) {
        //
        // ... удалим старый список, если он был создан
        //
        if (d->charactersModelFromText != nullptr) {
            d->charactersModelFromText->deleteLater();
            d->charactersModelFromText = nullptr;
        }
        //
        // ... и далее ничего не делаем
        //
        return;
    }

    //
    // В противном случае, собираем персонажей из текста
    //
    QSet<QString> characters;
    std::function<void(const TextModelItem*)> findCharactersInText;
    findCharactersInText = [this, &findCharactersInText, &characters](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                findCharactersInText(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);

                switch (textItem->paragraphType()) {
                case TextParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        if (d->charactersModel->exists(character)) {
                            characters.insert(character);
                        }
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    const auto character = ScreenplayCharacterParser::name(textItem->text());
                    if (d->charactersModel->exists(character)) {
                        characters.insert(character);
                    }
                    break;
                }

                default: {
                    break;
                }
                }

                break;
            }

            default:
                break;
            }
        }
    };
    findCharactersInText(d->rootItem());
    //
    // ... не забываем приаттачить всех персонажей, у кого определена роль в истории
    //
    for (int row = 0; row < d->charactersModel->rowCount(); ++row) {
        const auto character = d->charactersModel->character(row);
        if (character->storyRole() != CharacterStoryRole::Undefined) {
            characters.insert(character->name());
        }
    }
    //
    // ... создаём (при необходимости) и наполняем модель
    //
    if (d->charactersModelFromText == nullptr) {
        d->charactersModelFromText = new QStringListModel(this);
    }
    d->charactersModelFromText->setStringList(characters.values());
}

void ScreenplayTextModel::initEmptyDocument()
{
    auto sceneHeading = new ScreenplayTextModelTextItem(this);
    sceneHeading->setParagraphType(TextParagraphType::SceneHeading);
    auto scene = new ScreenplayTextModelSceneItem(this);
    scene->appendItem(sceneHeading);
    appendItem(scene);
}

void ScreenplayTextModel::finalizeInitialization()
{
    emit rowsAboutToBeChanged();
    updateNumbering();
    emit rowsChanged();
}

void ScreenplayTextModel::applyPatch(const QByteArray& _patch)
{
    TextModel::applyPatch(_patch);

    updateNumbering();
}

} // namespace BusinessLayer
