#include "audioplay_text_model.h"

#include "audioplay_text_model_folder_item.h"
#include "audioplay_text_model_scene_item.h"
#include "audioplay_text_model_text_item.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_block_parser.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/templates/audioplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QRegularExpression>
#include <QStringListModel>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/audioplay/text/item";
}

class AudioplayTextModel::Implementation
{
public:
    explicit Implementation(AudioplayTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Обновить номера сцен и реплик
     */
    void updateNumbering();

    /**
     * @brief Пересчитать хронометраж элемента и всех детей
     */
    void updateChildrenDuration(const TextModelItem* _item);


    /**
     * @brief Родительский элемент
     */
    AudioplayTextModel* q = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    AudioplayInformationModel* informationModel = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Нужно ли обновить справочники, которые строятся в рантайме
     */
    bool needUpdateRuntimeDictionaries = false;

    /**
     * @brief Справочники, которые строятся в рантайме
     */
    QStringListModel* charactersModelFromText = nullptr;
};

AudioplayTextModel::Implementation::Implementation(AudioplayTextModel* _q)
    : q(_q)
{
}

TextModelItem* AudioplayTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void AudioplayTextModel::Implementation::updateNumbering()
{
    int sceneNumber = 1;
    int blockNumber = 1;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering
        = [&sceneNumber, &blockNumber, &updateChildNumbering](const TextModelItem* _item) {
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
                      if (groupItem->setNumber(sceneNumber, {})) {
                          ++sceneNumber;
                      }
                      break;
                  }

                  case TextModelItemType::Text: {
                      auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);
                      if ((textItem->paragraphType() == TextParagraphType::Dialogue
                           || textItem->paragraphType() == TextParagraphType::Sound
                           || textItem->paragraphType() == TextParagraphType::Music
                           || textItem->paragraphType() == TextParagraphType::Cue)
                          && !textItem->isCorrection()) {
                          textItem->setNumber(blockNumber++);
                      }
                      break;
                  }

                  default:
                      break;
                  }
              }
          };
    updateChildNumbering(rootItem());
}

void AudioplayTextModel::Implementation::updateChildrenDuration(const TextModelItem* _item)
{
    for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        switch (childItem->type()) {
        case TextModelItemType::Folder:
        case TextModelItemType::Group: {
            updateChildrenDuration(childItem);
            break;
        }

        case TextModelItemType::Text: {
            auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);
            textItem->updateDuration();
            q->updateItem(textItem);
            break;
        }

        default:
            break;
        }
    }
}


// ****


AudioplayTextModel::AudioplayTextModel(QObject* _parent)
    : TextModel(_parent, createFolderItem(TextFolderType::Act))
    , d(new Implementation(this))
{
    auto updateCounters = [this](const QModelIndex& _index) {
        d->updateNumbering();
        d->updateChildrenDuration(itemForIndex(_index));
    };
    connect(this, &AudioplayTextModel::rowsInserted, this, updateCounters);
    connect(this, &AudioplayTextModel::rowsRemoved, this, updateCounters);

    connect(this, &AudioplayTextModel::contentsChanged, this,
            [this] { d->needUpdateRuntimeDictionaries = true; });
}

AudioplayTextModel::~AudioplayTextModel() = default;

TextModelFolderItem* AudioplayTextModel::createFolderItem(TextFolderType _type) const
{
    Q_UNUSED(_type)

    return new AudioplayTextModelFolderItem(this);
}

TextModelGroupItem* AudioplayTextModel::createGroupItem(TextGroupType _type) const
{
    Q_UNUSED(_type)

    switch (_type) {
    case TextGroupType::Scene: {
        return new AudioplayTextModelSceneItem(this);
    }

    default: {
        Q_ASSERT(false);
        return nullptr;
    }
    }
}

TextModelTextItem* AudioplayTextModel::createTextItem() const
{
    return new AudioplayTextModelTextItem(this);
}

QStringList AudioplayTextModel::mimeTypes() const
{
    return { kMimeType };
}

void AudioplayTextModel::setInformationModel(AudioplayInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    d->informationModel = _model;
}

AudioplayInformationModel* AudioplayTextModel::informationModel() const
{
    return d->informationModel;
}

void AudioplayTextModel::setCharactersModel(CharactersModel* _model)
{
    if (d->charactersModel) {
        d->charactersModel->disconnect(this);
    }

    d->charactersModel = _model;

    connect(d->charactersModel, &CharactersModel::contentsChanged, this,
            [this] { d->needUpdateRuntimeDictionaries = true; });
}

QAbstractItemModel* AudioplayTextModel::charactersModel() const
{
    if (d->charactersModelFromText != nullptr) {
        return d->charactersModelFromText;
    }

    return d->charactersModel;
}

BusinessLayer::CharacterModel* AudioplayTextModel::character(const QString& _name) const
{
    return d->charactersModel->character(_name);
}

void AudioplayTextModel::createCharacter(const QString& _name)
{
    d->charactersModel->createCharacter(_name);
}

void AudioplayTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
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
                auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::Character
                    && AudioplayCharacterParser::name(textItem->text()) == oldName) {
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

QSet<QString> AudioplayTextModel::findCharactersFromText() const
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
                auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::Character) {
                    characters.insert(AudioplayCharacterParser::name(textItem->text()));
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

std::chrono::milliseconds AudioplayTextModel::duration() const
{
    return static_cast<AudioplayTextModelFolderItem*>(d->rootItem())->duration();
}

std::map<std::chrono::milliseconds, QColor> AudioplayTextModel::itemsColors() const
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
                const auto sceneItem = static_cast<const AudioplayTextModelSceneItem*>(childItem);
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

std::map<std::chrono::milliseconds, QColor> AudioplayTextModel::itemsBookmarks() const
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
                const auto textItem = static_cast<const AudioplayTextModelTextItem*>(childItem);
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

void AudioplayTextModel::recalculateDuration()
{
    emit rowsAboutToBeChanged();
    d->updateChildrenDuration(d->rootItem());
    emit rowsChanged();
}

void AudioplayTextModel::updateRuntimeDictionariesIfNeeded()
{
    if (!d->needUpdateRuntimeDictionaries) {
        return;
    }

    updateRuntimeDictionaries();

    d->needUpdateRuntimeDictionaries = false;
}

void AudioplayTextModel::updateRuntimeDictionaries()
{
    const bool useCharactersFromText
        = settingsValue(DataStorageLayer::kComponentsAudioplayEditorUseCharactersFromTextKey)
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
                auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);

                switch (textItem->paragraphType()) {
                case TextParagraphType::Character: {
                    const auto character = AudioplayCharacterParser::name(textItem->text());
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

void AudioplayTextModel::initEmptyDocument()
{
    auto sceneHeading = new AudioplayTextModelTextItem(this);
    sceneHeading->setParagraphType(TextParagraphType::SceneHeading);
    auto scene = new AudioplayTextModelSceneItem(this);
    scene->appendItem(sceneHeading);
    appendItem(scene);
}

void AudioplayTextModel::finalizeInitialization()
{
    emit rowsAboutToBeChanged();
    d->updateNumbering();
    emit rowsChanged();
}

void AudioplayTextModel::applyPatch(const QByteArray& _patch)
{
    TextModel::applyPatch(_patch);

    d->updateNumbering();
}

} // namespace BusinessLayer
