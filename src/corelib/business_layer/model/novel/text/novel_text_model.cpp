#include "novel_text_model.h"

#include "novel_text_block_parser.h"
#include "novel_text_model_beat_item.h"
#include "novel_text_model_folder_item.h"
#include "novel_text_model_scene_item.h"
#include "novel_text_model_text_item.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/templates/novel_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QRegularExpression>
#include <QStringListModel>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/novel/text/item";
}

class NovelTextModel::Implementation
{
public:
    explicit Implementation(NovelTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Пересчитать хронометраж элемента и всех детей
     */
    void updateChildrenCounters(const TextModelItem* _item);


    /**
     * @brief Родительский элемент
     */
    NovelTextModel* q = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    NovelInformationModel* informationModel = nullptr;

    /**
     * @brief Модель справочников
     */
    NovelDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Модель локаций
     */
    LocationsModel* locationsModel = nullptr;

    /**
     * @brief Количество страниц
     */
    int outlinePageCount = 0;
    int textPageCount = 0;

    /**
     * @brief Количество сцен
     */
    int scenesCount = 0;
};

NovelTextModel::Implementation::Implementation(NovelTextModel* _q)
    : q(_q)
{
}

TextModelItem* NovelTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void NovelTextModel::Implementation::updateChildrenCounters(const TextModelItem* _item)
{
    if (_item == nullptr) {
        return;
    }

    for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        switch (childItem->type()) {
        case TextModelItemType::Folder:
        case TextModelItemType::Group: {
            updateChildrenCounters(childItem);
            break;
        }

        case TextModelItemType::Text: {
            auto textItem = static_cast<NovelTextModelTextItem*>(childItem);
            textItem->updateCounters();
            break;
        }

        default:
            break;
        }
    }
}


// ****


NovelTextModel::NovelTextModel(QObject* _parent)
    : TextModel(_parent, NovelTextModel::createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    auto updateCounters
        = [this](const QModelIndex& _index) { d->updateChildrenCounters(itemForIndex(_index)); };
    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &NovelTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &NovelTextModel::afterRowsRemoved, this, updateCounters);
}

NovelTextModel::~NovelTextModel() = default;

QString NovelTextModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Novel"), informationModel()->name());
}

TextModelFolderItem* NovelTextModel::createFolderItem(TextFolderType _type) const
{
    return new NovelTextModelFolderItem(this, _type);
}

TextModelGroupItem* NovelTextModel::createGroupItem(TextGroupType _type) const
{
    switch (_type) {
    case TextGroupType::Scene: {
        return new NovelTextModelSceneItem(this);
    }

    case TextGroupType::Beat: {
        return new NovelTextModelBeatItem(this);
    }

    default: {
        Q_ASSERT(false);
        return nullptr;
    }
    }
}

TextModelTextItem* NovelTextModel::createTextItem() const
{
    return new NovelTextModelTextItem(this);
}

QStringList NovelTextModel::mimeTypes() const
{
    return { kMimeType };
}

void NovelTextModel::setInformationModel(NovelInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        d->informationModel->disconnect(this);
    }

    d->informationModel = _model;
}

NovelInformationModel* NovelTextModel::informationModel() const
{
    return d->informationModel;
}

void NovelTextModel::setDictionariesModel(NovelDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

NovelDictionariesModel* NovelTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void NovelTextModel::setCharactersModel(CharactersModel* _model)
{
    if (d->charactersModel) {
        d->charactersModel->disconnect(this);
    }

    d->charactersModel = _model;
}

QAbstractItemModel* NovelTextModel::charactersModel() const
{
    return d->charactersModel;
}

BusinessLayer::CharacterModel* NovelTextModel::character(const QString& _name) const
{
    return d->charactersModel->character(_name);
}

void NovelTextModel::createCharacter(const QString& _name)
{
    d->charactersModel->createCharacter(_name);
}

void NovelTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
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
                auto textItem = static_cast<NovelTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneCharacters
                    && NovelSceneCharactersParser::characters(textItem->text()).contains(oldName)) {
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
                           && NovelCharacterParser::name(textItem->text()) == oldName) {
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

QVector<QModelIndex> NovelTextModel::characterDialogues(const QString& _name) const
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
                lastCharacter = NovelCharacterParser::name(textItem->text());
                break;
            }

            case TextParagraphType::Parenthetical: {
                //
                // Не очищаем имя персонажа, идём до реплики
                //
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

QSet<QString> NovelTextModel::findCharactersFromText() const
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
                auto textItem = static_cast<NovelTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneCharacters) {
                    const auto textCharacters
                        = NovelSceneCharactersParser::characters(textItem->text());
                    for (const auto& character : textCharacters) {
                        characters.insert(character);
                    }
                } else if (textItem->paragraphType() == TextParagraphType::Character) {
                    characters.insert(NovelCharacterParser::name(textItem->text()));
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

void NovelTextModel::setLocationsModel(LocationsModel* _model)
{
    d->locationsModel = _model;
}

QAbstractItemModel* NovelTextModel::locationsModel() const
{
    return d->locationsModel;
}

LocationModel* NovelTextModel::location(const QString& _name) const
{
    return d->locationsModel->location(_name);
}

void NovelTextModel::createLocation(const QString& _name)
{
    d->locationsModel->createLocation(_name);
}

QSet<QString> NovelTextModel::findLocationsFromText() const
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
                auto textItem = static_cast<NovelTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneHeading) {
                    locations.insert(NovelSceneHeadingParser::location(textItem->text()));
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

void NovelTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
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
                      auto textItem = static_cast<NovelTextModelTextItem*>(childItem);
                      if (textItem->paragraphType() == TextParagraphType::SceneHeading
                          && NovelSceneHeadingParser::location(textItem->text()) == oldName) {
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

int NovelTextModel::outlinePageCount() const
{
    return d->outlinePageCount;
}

void NovelTextModel::setOutlinePageCount(int _count)
{
    if (d->outlinePageCount == _count) {
        return;
    }

    d->outlinePageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

int NovelTextModel::textPageCount() const
{
    return d->textPageCount;
}

void NovelTextModel::setTextPageCount(int _count)
{
    if (d->textPageCount == _count) {
        return;
    }

    d->textPageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

int NovelTextModel::scenesCount() const
{
    return d->scenesCount;
}

int NovelTextModel::wordsCount() const
{
    return static_cast<NovelTextModelFolderItem*>(d->rootItem())->wordsCount();
}

QPair<int, int> NovelTextModel::charactersCount() const
{
    return static_cast<NovelTextModelFolderItem*>(d->rootItem())->charactersCount();
}

void NovelTextModel::initEmptyDocument()
{
    auto sceneHeading = new NovelTextModelTextItem(this);
    sceneHeading->setParagraphType(TextParagraphType::SceneHeading);
    auto scene = new NovelTextModelSceneItem(this);
    scene->appendItem(sceneHeading);
    appendItem(scene);
}

void NovelTextModel::finalizeInitialization()
{
}

} // namespace BusinessLayer
