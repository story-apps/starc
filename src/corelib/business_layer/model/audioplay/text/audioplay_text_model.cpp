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
    int dialogueNumber = 0;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering = [this, &sceneNumber, &dialogueNumber,
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
                if (groupItem->setNumber(sceneNumber, {})) {
                    q->updateItemForRoles(groupItem, { TextModelGroupItem::GroupNumberRole });
                }
                ++sceneNumber;
                groupItem->prepareNumberText("#");
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);
                if (!textItem->isCorrection()) {
                    switch (textItem->paragraphType()) {
                    case TextParagraphType::Character:
                    case TextParagraphType::Sound:
                    case TextParagraphType::Music:
                    case TextParagraphType::Cue: {
                        ++dialogueNumber;
                        Q_FALLTHROUGH();
                    }

                    case TextParagraphType::Dialogue:
                    case TextParagraphType::Lyrics: {
                        if (textItem->setNumber(dialogueNumber)) {
                            q->updateItemForRoles(textItem, { TextModelTextItem::TextNumberRole });
                        }
                        break;
                    }

                    default: {
                        break;
                    }
                    }
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
            break;
        }

        default:
            break;
        }
    }
}


// ****


AudioplayTextModel::AudioplayTextModel(QObject* _parent)
    : ScriptTextModel(_parent, createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    auto updateCounters = [this](const QModelIndex& _index) {
        d->updateNumbering();
        d->updateChildrenDuration(itemForIndex(_index));
    };
    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &AudioplayTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &AudioplayTextModel::afterRowsRemoved, this, updateCounters);

    connect(this, &AudioplayTextModel::contentsChanged, this,
            &AudioplayTextModel::markNeedUpdateRuntimeDictionaries);
}

AudioplayTextModel::~AudioplayTextModel() = default;

TextModelFolderItem* AudioplayTextModel::createFolderItem(TextFolderType _type) const
{
    return new AudioplayTextModelFolderItem(this, _type);
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
                    const QRegularExpression nameMatcher(
                        QString("\\b(%1)\\b").arg(TextHelper::toRxEscaped(oldName)),
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

QVector<QModelIndex> AudioplayTextModel::characterDialogues(const QString& _name) const
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
                lastCharacter = AudioplayCharacterParser::name(textItem->text());
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

QVector<QString> AudioplayTextModel::findCharactersFromText() const
{
    QVector<QString> characters;
    QHash<QString, int> charactersDialogues;
    std::function<void(const TextModelItem*)> findCharacters;
    findCharacters
        = [&characters, &charactersDialogues, &findCharacters](const TextModelItem* _item) {
              for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                  auto childItem = _item->childAt(childIndex);
                  switch (childItem->type()) {
                  case TextModelItemType::Folder:
                  case TextModelItemType::Group: {
                      findCharacters(childItem);
                      break;
                  }

                  case TextModelItemType::Text: {
                      auto textItem = static_cast<TextModelTextItem*>(childItem);
                      if (textItem->paragraphType() == TextParagraphType::Character) {
                          const auto character = AudioplayCharacterParser::name(textItem->text());
                          if (charactersDialogues.contains(character)) {
                              ++charactersDialogues[character];
                          } else {
                              characters.append(character);
                              charactersDialogues.insert(character, 1);
                          }
                      }
                      break;
                  }

                  default:
                      break;
                  }
              }
          };
    findCharacters(d->rootItem());
    std::sort(characters.begin(), characters.end(),
              [&charactersDialogues](const QString& _lhs, const QString& _rhs) {
                  return charactersDialogues.value(_lhs) > charactersDialogues.value(_rhs);
              });

    return characters;
}

void AudioplayTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
{
    Q_UNUSED(_oldName)
    Q_UNUSED(_newName)
}

QVector<QString> AudioplayTextModel::findLocationsFromText() const
{
    return {};
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

void AudioplayTextModel::updateRuntimeDictionaries()
{
    const bool showHintsForAllItems
        = settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowHintsForAllItemsKey)
              .toBool();
    const bool showHintsForPrimaryItems
        = settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowHintsForPrimaryItemsKey)
              .toBool();
    const bool showHintsForSecondaryItems
        = settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowHintsForSecondaryItemsKey)
              .toBool();
    const bool showHintsForTertiaryItems
        = settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowHintsForTertiaryItemsKey)
              .toBool();

    //
    // В противном случае, собираем персонажей из текста
    //
    QSet<QString> characters;
    std::function<void(const TextModelItem*)> findInText;
    findInText = [&findInText, &characters](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                findInText(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);

                switch (textItem->paragraphType()) {
                case TextParagraphType::Character: {
                    characters.insert(AudioplayCharacterParser::name(textItem->text()));
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
    findInText(d->rootItem());
    characters.remove({});
    //
    // ... не забываем приаттачить всех персонажей, у кого определена роль в истории
    //
    for (int row = 0; row < charactersModel()->rowCount(); ++row) {
        const auto character = charactersModel()->character(row);

        //
        // ... фильтруем по ролям, если необходимо
        //
        if (!showHintsForAllItems) {
            bool skipCharacter = true;
            switch (character->storyRole()) {
            case CharacterStoryRole::Primary: {
                skipCharacter = !showHintsForPrimaryItems;
                break;
            }
            case CharacterStoryRole::Secondary: {
                skipCharacter = !showHintsForSecondaryItems;
                break;
            }
            case CharacterStoryRole::Tertiary: {
                skipCharacter = !showHintsForTertiaryItems;
                break;
            }
            default: {
                break;
            }
            }

            if (skipCharacter) {
                continue;
            }
        }

        characters.insert(character->name());
    }
    //
    // ... создаём (при необходимости) и наполняем модель
    //
    charactersModelFromText()->setStringList(characters.values());
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

ChangeCursor AudioplayTextModel::applyPatch(const QByteArray& _patch)
{
    const auto changeCursor = TextModel::applyPatch(_patch);

    d->updateNumbering();

    return changeCursor;
}

} // namespace BusinessLayer
