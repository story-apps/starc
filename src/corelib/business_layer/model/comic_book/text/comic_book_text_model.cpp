#include "comic_book_text_model.h"

#include "comic_book_text_block_parser.h"
#include "comic_book_text_model_folder_item.h"
#include "comic_book_text_model_page_item.h"
#include "comic_book_text_model_panel_item.h"
#include "comic_book_text_model_text_item.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>
#include <utils/shugar.h>

#include <QRegularExpression>
#include <QStringListModel>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/comicbook/text/item";
}

class ComicBookTextModel::Implementation
{
public:
    explicit Implementation(ComicBookTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Обновить номера страниц, панелей и реплик
     */
    void updateNumbering();

    /**
     * @brief Пересчитать счетчики элемента и всех детей
     */
    void updateChildrenCounters(const TextModelItem* _item);


    /**
     * @brief Родительский элемент
     */
    ComicBookTextModel* q = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    ComicBookInformationModel* informationModel = nullptr;

    /**
     * @brief Модель справочников
     */
    ComicBookDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Количество панелей
     */
    int panelsCount = 0;

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

ComicBookTextModel::Implementation::Implementation(ComicBookTextModel* _q)
    : q(_q)
{
}

TextModelItem* ComicBookTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void ComicBookTextModel::Implementation::updateNumbering()
{
    if (isUpdateNumberingPlanned) {
        return;
    }

    panelsCount = 0;
    int pageNumber = 1;
    int panelNumber = 1;
    int dialogueNumber = 0;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering = [this, &pageNumber, &panelNumber, &dialogueNumber,
                            &updateChildNumbering](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder: {
                updateChildNumbering(childItem);
                break;
            }

            case TextModelItemType::Group: {
                const auto groupItem = static_cast<TextModelGroupItem*>(childItem);
                if (groupItem->groupType() == TextGroupType::Page) {
                    panelNumber = 1;
                    dialogueNumber = 0;
                    updateChildNumbering(childItem);

                    auto pageItem = static_cast<ComicBookTextModelPageItem*>(childItem);
                    pageItem->setPageNumber(pageNumber, dictionariesModel->singlePageIntros(),
                                            dictionariesModel->multiplePageIntros());
                    pageItem->updateCounters();
                } else if (groupItem->groupType() == TextGroupType::Panel) {
                    ++panelsCount;
                    updateChildNumbering(childItem);
                    auto panelItem = static_cast<ComicBookTextModelPanelItem*>(childItem);
                    panelItem->setPanelNumber(panelNumber, dictionariesModel->singlePanelIntros(),
                                              dictionariesModel->multiplePanelIntros());
                }
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

void ComicBookTextModel::Implementation::updateChildrenCounters(const TextModelItem* _item)
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
            auto textItem = static_cast<ComicBookTextModelTextItem*>(childItem);
            textItem->updateCounters();
            break;
        }

        default:
            break;
        }
    }
}


// ****


ComicBookTextModel::ComicBookTextModel(QObject* _parent)
    : ScriptTextModel(_parent, ComicBookTextModel::createFolderItem(TextFolderType::Root))
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
    connect(this, &ComicBookTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &ComicBookTextModel::afterRowsRemoved, this, updateCounters);
    connect(this, &ComicBookTextModel::modelReset, this, updateCounters);
    //
    // Если модель планируем большое изменение, то планируем отложенное обновление нумерации
    //
    connect(this, &ComicBookTextModel::rowsAboutToBeChanged, this,
            [this] { d->isUpdateNumberingPlanned = true; });
    connect(this, &ComicBookTextModel::rowsChanged, this, [this] {
        if (d->isUpdateNumberingPlanned) {
            d->isUpdateNumberingPlanned = false;
            d->updateNumbering();
        }
    });

    connect(this, &ComicBookTextModel::contentsChanged, this,
            &ComicBookTextModel::markNeedUpdateRuntimeDictionaries);
}

ComicBookTextModel::~ComicBookTextModel() = default;

TextModelFolderItem* ComicBookTextModel::createFolderItem(TextFolderType _type) const
{
    return new ComicBookTextModelFolderItem(this, _type);
}

TextModelGroupItem* ComicBookTextModel::createGroupItem(TextGroupType _type) const
{
    switch (_type) {
    case TextGroupType::Page: {
        return new ComicBookTextModelPageItem(this);
    }

    case TextGroupType::Panel: {
        return new ComicBookTextModelPanelItem(this);
    }

    default: {
        Q_ASSERT(false);
        return nullptr;
    }
    }
}

TextModelTextItem* ComicBookTextModel::createTextItem() const
{
    return new ComicBookTextModelTextItem(this);
}

void ComicBookTextModel::setInformationModel(ComicBookInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    d->informationModel = _model;
}

ComicBookInformationModel* ComicBookTextModel::informationModel() const
{
    return d->informationModel;
}

void ComicBookTextModel::setDictionariesModel(ComicBookDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

ComicBookDictionariesModel* ComicBookTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void ComicBookTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
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
                    && ComicBookCharacterParser::name(textItem->text()) == oldName) {
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

QVector<QModelIndex> ComicBookTextModel::characterDialogues(const QString& _name) const
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
                lastCharacter = ComicBookCharacterParser::name(textItem->text());
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

QVector<QString> ComicBookTextModel::findCharactersFromText() const
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
                          const auto character = ComicBookCharacterParser::name(textItem->text());
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

void ComicBookTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
{
    Q_UNUSED(_oldName)
    Q_UNUSED(_newName)
}

QVector<QModelIndex> ComicBookTextModel::locationScenes(const QString& _name) const
{
    Q_UNUSED(_name);
    return {};
}

QVector<QString> ComicBookTextModel::findLocationsFromText() const
{
    return {};
}

void ComicBookTextModel::updateRuntimeDictionaries()
{
    const bool showHintsForAllItems
        = settingsValue(DataStorageLayer::kComponentsComicBookEditorShowHintsForAllItemsKey)
              .toBool();
    const bool showHintsForPrimaryItems
        = settingsValue(DataStorageLayer::kComponentsComicBookEditorShowHintsForPrimaryItemsKey)
              .toBool();
    const bool showHintsForSecondaryItems
        = settingsValue(DataStorageLayer::kComponentsComicBookEditorShowHintsForSecondaryItemsKey)
              .toBool();
    const bool showHintsForTertiaryItems
        = settingsValue(DataStorageLayer::kComponentsComicBookEditorShowHintsForTertiaryItemsKey)
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
                    characters.insert(ComicBookCharacterParser::name(textItem->text()));
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

QStringList ComicBookTextModel::mimeTypes() const
{
    return { kMimeType };
}

int ComicBookTextModel::panelsCount() const
{
    return d->panelsCount;
}

int ComicBookTextModel::wordsCount() const
{
    return static_cast<ComicBookTextModelFolderItem*>(d->rootItem())->wordsCount();
}

QPair<int, int> ComicBookTextModel::charactersCount() const
{
    return static_cast<ComicBookTextModelFolderItem*>(d->rootItem())->charactersCount();
}

int ComicBookTextModel::textPageCount() const
{
    return d->textPageCount;
}

void ComicBookTextModel::setTextPageCount(int _count)
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

void ComicBookTextModel::initEmptyDocument()
{
    auto pageText = createTextItem();
    pageText->setParagraphType(TextParagraphType::PageHeading);
    auto page = createGroupItem(TextGroupType::Page);
    page->resetUuid();
    page->appendItem(pageText);
    appendItem(page);
}

void ComicBookTextModel::finalizeInitialization()
{
    beginChangeRows();
    d->updateNumbering();
    endChangeRows();
}

ChangeCursor ComicBookTextModel::applyPatch(const QByteArray& _patch)
{
    const auto changeCursor = TextModel::applyPatch(_patch);

    d->updateNumbering();

    return changeCursor;
}

QByteArray ComicBookTextModel::restoreAfterComparison(const QByteArray& _xml) const
{
    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
        + "\" version=\"1.0\">\n";
    bool hasOpenedPage = false;
    bool hasOpenedPanel = false;
    auto closePage = [&xml, &hasOpenedPage] {
        if (hasOpenedPage) {
            xml += "</content>\n"
                   "</page>\n";
            hasOpenedPage = false;
        }
    };
    auto closePanel = [&xml, &hasOpenedPanel] {
        if (hasOpenedPanel) {
            xml += "</content>\n"
                   "</panel>\n";
            hasOpenedPanel = false;
        }
    };
    const auto lines = _xml.split('\n');
    for (const auto& line : lines) {
        if (!line.startsWith("<")) {
            xml += line + "\n";
            continue;
        }

        if (line.startsWith("<page_heading")) {
            closePanel();
            closePage();
            xml += QString("<page uuid=\"%1\" plots=\"\">\n"
                           "<content>\n")
                       .arg(QUuid::createUuid().toString())
                       .toUtf8();
            hasOpenedPage = true;
            xml += line + "\n";
        } else if (line.startsWith("<panel_heading")) {
            closePanel();
            xml += QString("<panel uuid=\"%1\" plots=\"\">\n"
                           "<content>\n")
                       .arg(QUuid::createUuid().toString())
                       .toUtf8();
            hasOpenedPanel = true;
            xml += line + "\n";
        } else {
            xml += line + "\n";
        }
    }
    xml += "</document";
    return xml;
}

} // namespace BusinessLayer
