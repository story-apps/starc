#include "comic_book_text_model.h"

#include "comic_book_text_block_parser.h"
#include "comic_book_text_model_page_item.h"
#include "comic_book_text_model_panel_item.h"

#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>
#include <utils/shugar.h>

#include <QRegularExpression>
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
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;
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
    int pageNumber = 1;
    int panelNumber = 1;
    int dialogueNumber = 1;
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
                    dialogueNumber = 1;
                    updateChildNumbering(childItem);

                    auto pageItem = static_cast<ComicBookTextModelPageItem*>(childItem);
                    pageItem->setPageNumber(pageNumber, dictionariesModel->singlePageIntros(),
                                            dictionariesModel->multiplePageIntros());
                } else {
                    updateChildNumbering(childItem);
                    auto panelItem = static_cast<ComicBookTextModelPanelItem*>(childItem);
                    if (panelItem->setNumber(panelNumber, {})) {
                        panelNumber++;
                    }
                }
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<TextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::Character
                    && !textItem->isCorrection()) {
                    textItem->setNumber(dialogueNumber++);
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


// ****


ComicBookTextModel::ComicBookTextModel(QObject* _parent)
    : TextModel(_parent, ComicBookTextModel::createFolderItem())
    , d(new Implementation(this))
{
    auto updateNumbering = [this] { d->updateNumbering(); };
    connect(this, &ComicBookTextModel::rowsInserted, this, updateNumbering);
    connect(this, &ComicBookTextModel::rowsRemoved, this, updateNumbering);
}

ComicBookTextModel::~ComicBookTextModel() = default;

TextModelFolderItem* ComicBookTextModel::createFolderItem() const
{
    return new TextModelFolderItem(this);
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
    return new TextModelTextItem(this);
}

void ComicBookTextModel::setInformationModel(ComicBookInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        disconnect(d->informationModel);
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

void ComicBookTextModel::setCharactersModel(CharactersModel* _model)
{
    d->charactersModel = _model;
}

CharactersModel* ComicBookTextModel::charactersModel() const
{
    return d->charactersModel;
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

QSet<QString> ComicBookTextModel::findCharactersFromText() const
{
    QSet<QString> characters;
    std::function<void(const TextModelItem*)> findCharacters;
    findCharacters = [&characters, &findCharacters](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Group: {
                findCharacters(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<TextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::Character) {
                    characters.insert(ComicBookCharacterParser::name(textItem->text()));
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

QStringList ComicBookTextModel::mimeTypes() const
{
    return { kMimeType };
}

void ComicBookTextModel::initEmptyDocument()
{
    auto pageText = createTextItem();
    pageText->setParagraphType(TextParagraphType::PageHeading);
    auto page = createGroupItem(TextGroupType::Page);
    page->appendItem(pageText);
    appendItem(page);
}

void ComicBookTextModel::finalizeInitialization()
{
    emit rowsAboutToBeChanged();
    d->updateNumbering();
    emit rowsChanged();
}

} // namespace BusinessLayer
