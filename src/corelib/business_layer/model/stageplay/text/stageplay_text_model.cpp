#include "stageplay_text_model.h"

#include "stageplay_text_model_folder_item.h"
#include "stageplay_text_model_scene_item.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_block_parser.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QRegularExpression>
#include <QStringListModel>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/stageplay/text/item";
}

class StageplayTextModel::Implementation
{
public:
    explicit Implementation(StageplayTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Обновить номера сцен и реплик
     */
    void updateNumbering();

    /**
     * @brief Пересчитать счетчики элемента и всех детей
     */
    void updateChildrenCounters(const TextModelItem* _item);


    /**
     * @brief Родительский элемент
     */
    StageplayTextModel* q = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    StageplayInformationModel* informationModel = nullptr;

    /**
     * @brief Количество сцен
     */
    int scenesCount = 0;

    /**
     * @brief Количество страниц
     */
    int textPageCount = 0;

    /**
     * @brief Последний сохранённый хэш документа
     */
    QByteArray lastContentHash;

    /**
     * @brief Запланировано ли обновление нумерации
     */
    bool isUpdateNumberingPlanned = false;
};

StageplayTextModel::Implementation::Implementation(StageplayTextModel* _q)
    : q(_q)
{
}

TextModelItem* StageplayTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void StageplayTextModel::Implementation::updateNumbering()
{
    if (isUpdateNumberingPlanned) {
        return;
    }

    scenesCount = 0;
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
                ++scenesCount;
                updateChildNumbering(childItem);
                auto groupItem = static_cast<TextModelGroupItem*>(childItem);
                if (groupItem->setNumber(sceneNumber, {})) {
                    ++sceneNumber;
                }
                groupItem->prepareNumberText("#");
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<TextModelTextItem*>(childItem);
                if (!textItem->isCorrection()) {
                    switch (textItem->paragraphType()) {
                    case TextParagraphType::Character: {
                        ++dialogueNumber;
                        Q_FALLTHROUGH();
                    }

                    case TextParagraphType::Dialogue:
                    case TextParagraphType::Lyrics: {
                        textItem->setNumber(dialogueNumber);
                        q->updateItemForRoles(textItem, { TextModelTextItem::TextNumberRole });
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

void StageplayTextModel::Implementation::updateChildrenCounters(const TextModelItem* _item)
{
    for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        switch (childItem->type()) {
        case TextModelItemType::Folder:
        case TextModelItemType::Group: {
            updateChildrenCounters(childItem);
            break;
        }

        case TextModelItemType::Text: {
            auto textItem = static_cast<TextModelTextItem*>(childItem);
            textItem->updateCounters();
            break;
        }

        default:
            break;
        }
    }
}


// ****


StageplayTextModel::StageplayTextModel(QObject* _parent)
    : ScriptTextModel(_parent, createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    auto updateCounters = [this](const QModelIndex& _index = {}) {
        if (const auto hash = contentHash(); d->lastContentHash != hash) {
            d->updateNumbering();
            d->lastContentHash = hash;
        }

        d->updateChildrenCounters(itemForIndex(_index));
    };
    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &StageplayTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &StageplayTextModel::afterRowsRemoved, this, updateCounters);
    connect(this, &StageplayTextModel::modelReset, this, updateCounters);
    //
    // Если модель планируем большое изменение, то планируем отложенное обновление нумерации
    //
    connect(this, &StageplayTextModel::rowsAboutToBeChanged, this,
            [this] { d->isUpdateNumberingPlanned = true; });
    connect(this, &StageplayTextModel::rowsChanged, this, [this] {
        if (d->isUpdateNumberingPlanned) {
            d->isUpdateNumberingPlanned = false;
            d->updateNumbering();
        }
    });

    connect(this, &StageplayTextModel::contentsChanged, this,
            &StageplayTextModel::markNeedUpdateRuntimeDictionaries);
}

StageplayTextModel::~StageplayTextModel() = default;

TextModelFolderItem* StageplayTextModel::createFolderItem(TextFolderType _type) const
{
    return new StageplayTextModelFolderItem(this, _type);
}

TextModelGroupItem* StageplayTextModel::createGroupItem(TextGroupType _type) const
{
    Q_UNUSED(_type)

    switch (_type) {
    case TextGroupType::Scene: {
        return new StageplayTextModelSceneItem(this);
    }

    default: {
        Q_ASSERT(false);
        return nullptr;
    }
    }
}

TextModelTextItem* StageplayTextModel::createTextItem() const
{
    return new TextModelTextItem(this);
}

QStringList StageplayTextModel::mimeTypes() const
{
    return { kMimeType };
}

int StageplayTextModel::scenesCount() const
{
    return d->scenesCount;
}

int StageplayTextModel::wordsCount() const
{
    return static_cast<StageplayTextModelFolderItem*>(d->rootItem())->wordsCount();
}

QPair<int, int> StageplayTextModel::charactersCount() const
{
    return static_cast<StageplayTextModelFolderItem*>(d->rootItem())->charactersCount();
}

int StageplayTextModel::textPageCount() const
{
    return d->textPageCount;
}

void StageplayTextModel::setTextPageCount(int _count)
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

void StageplayTextModel::setInformationModel(StageplayInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    d->informationModel = _model;
}

StageplayInformationModel* StageplayTextModel::informationModel() const
{
    return d->informationModel;
}

void StageplayTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
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
                auto textItem = static_cast<TextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::Character
                    && StageplayCharacterParser::name(textItem->text()) == oldName) {
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

    beginChangeRows();
    updateCharacterBlock(d->rootItem());
    endChangeRows();
}

QVector<QModelIndex> StageplayTextModel::characterDialogues(const QString& _name) const
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
                lastCharacter = StageplayCharacterParser::name(textItem->text());
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

QVector<QString> StageplayTextModel::findCharactersFromText() const
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
                          const auto character = StageplayCharacterParser::name(textItem->text());
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

void StageplayTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
{
    Q_UNUSED(_oldName)
    Q_UNUSED(_newName)
}

QVector<QModelIndex> StageplayTextModel::locationScenes(const QString& _name) const
{
    Q_UNUSED(_name)
    return {};
}

QVector<QString> StageplayTextModel::findLocationsFromText() const
{
    return {};
}

void StageplayTextModel::updateRuntimeDictionaries()
{
    const bool showHintsForAllItems
        = settingsValue(DataStorageLayer::kComponentsStageplayEditorShowHintsForAllItemsKey)
              .toBool();
    const bool showHintsForPrimaryItems
        = settingsValue(DataStorageLayer::kComponentsStageplayEditorShowHintsForPrimaryItemsKey)
              .toBool();
    const bool showHintsForSecondaryItems
        = settingsValue(DataStorageLayer::kComponentsStageplayEditorShowHintsForSecondaryItemsKey)
              .toBool();
    const bool showHintsForTertiaryItems
        = settingsValue(DataStorageLayer::kComponentsStageplayEditorShowHintsForTertiaryItemsKey)
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
                auto textItem = static_cast<TextModelTextItem*>(childItem);

                switch (textItem->paragraphType()) {
                case TextParagraphType::Character: {
                    characters.insert(StageplayCharacterParser::name(textItem->text()));
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

void StageplayTextModel::initEmptyDocument()
{
    auto sceneHeading = new TextModelTextItem(this);
    sceneHeading->setParagraphType(TextParagraphType::SceneHeading);
    auto scene = new StageplayTextModelSceneItem(this);
    scene->appendItem(sceneHeading);
    appendItem(scene);
}

void StageplayTextModel::finalizeInitialization()
{
    beginChangeRows();
    d->updateNumbering();
    endChangeRows();
}

ChangeCursor StageplayTextModel::applyPatch(const QByteArray& _patch)
{
    const auto changeCursor = TextModel::applyPatch(_patch);

    d->updateNumbering();

    return changeCursor;
}

} // namespace BusinessLayer
