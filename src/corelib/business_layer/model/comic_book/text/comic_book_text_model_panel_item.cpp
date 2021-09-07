#include "comic_book_text_model_panel_item.h"

#include "comic_book_text_block_parser.h"
#include "comic_book_text_model_splitter_item.h"
#include "comic_book_text_model_text_item.h"
#include "comic_book_text_model_xml.h"
#include "comic_book_text_model_xml_writer.h"

#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>


namespace BusinessLayer {

class ComicBookTextModelPanelItem::Implementation
{
public:
    /**
     * @brief Идентификатор панели
     */
    QUuid uuid;

    /**
     * @brief Номер панели
     */
    std::optional<Number> number;

    /**
     * @brief Цвет панели
     */
    QColor color;

    //    /**
    //     * @brief Тэги панели
    //     */
    //    QVector<Tag> tags;

    //    /**
    //     * @brief Сюжетные линии панели
    //     */
    //    QVector<StoryLine> storyLines;

    /**
     * @brief Штамп на панели
     */
    QString stamp;

    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Заголовок панели
     */
    QString heading;

    /**
     * @brief Текст панели
     */
    QString text;

    /**
     * @brief Количество заметок по тексту
     */
    int inlineNotesSize = 0;

    /**
     * @brief Количество редакторских заметок
     */
    int reviewMarksSize = 0;

    /**
     * @brief Количество слов в диалогах
     */
    int dialoguesWordsCount = 0;
};


// ****


bool ComicBookTextModelPanelItem::Number::operator==(
    const ComicBookTextModelPanelItem::Number& _other) const
{
    return value == _other.value;
}

ComicBookTextModelPanelItem::ComicBookTextModelPanelItem()
    : ComicBookTextModelItem(ComicBookTextModelItemType::Panel)
    , d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

ComicBookTextModelPanelItem::ComicBookTextModelPanelItem(QXmlStreamReader& _contentReader)
    : ComicBookTextModelItem(ComicBookTextModelItemType::Panel)
    , d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kPanelTag);

    const auto attributes = _contentReader.attributes();
    if (attributes.hasAttribute(xml::kUuidAttribute)) {
        d->uuid = attributes.value(xml::kUuidAttribute).toString();
    }

    //
    // TODO: plots
    //
    xml::readNextElement(_contentReader);

    auto currentTag = _contentReader.name();
    if (currentTag == xml::kNumberTag) {
        d->number = { _contentReader.attributes().value(xml::kNumberValueAttribute).toString() };
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kColorTag) {
        d->color = xml::readContent(_contentReader).toString();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kStampTag) {
        d->stamp = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kContentTag) {
        xml::readNextElement(_contentReader); // next item
        do {
            currentTag = _contentReader.name();

            //
            // Проглатываем закрывающий контентный тэг
            //
            if (currentTag == xml::kContentTag && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                continue;
            }
            //
            // Если дошли до конца панели, выходим из обработки
            //
            else if (currentTag == xml::kPanelTag && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (currentTag == xml::kPanelTag) {
                appendItem(new ComicBookTextModelPanelItem(_contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(new ComicBookTextModelSplitterItem(_contentReader));
            } else {
                appendItem(new ComicBookTextModelTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Соберём заголовок, текст панели и прочие параметры
    //
    handleChange();
}

ComicBookTextModelPanelItem::~ComicBookTextModelPanelItem() = default;

ComicBookTextModelPanelItem::Number ComicBookTextModelPanelItem::number() const
{
    if (!d->number.has_value()) {
        return {};
    }

    return *d->number;
}

bool ComicBookTextModelPanelItem::setNumber(int _number)
{
    if (childCount() == 0) {
        return false;
    }

    bool hasContent = false;
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        if (child->type() != ComicBookTextModelItemType::Text) {
            continue;
        }

        const auto textItemChild = static_cast<const ComicBookTextModelTextItem*>(child);
        if (!textItemChild->isCorrection()) {
            hasContent = true;
            break;
        }
    }
    if (!hasContent) {
        return false;
    }

    const auto newNumber = QString::number(_number);
    if (d->number.has_value() && d->number->value == newNumber) {
        return true;
    }

    d->number = { newNumber };
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
    //    setChanged(true);

    return true;
}

QColor ComicBookTextModelPanelItem::color() const
{
    return d->color;
}

void ComicBookTextModelPanelItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

int ComicBookTextModelPanelItem::dialoguesWordsCount() const
{
    return d->dialoguesWordsCount;
}

QVariant ComicBookTextModelPanelItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000F0B77";
    }

    case PanelNumberRole: {
        if (d->number.has_value()) {
            return d->number->value;
        }
        return {};
    }

    case PanelColorRole: {
        return d->color;
    }

    case PanelHeadingRole: {
        return d->heading;
    }

    case PanelTextRole: {
        return d->text;
    }

    case PanelInlineNotesSizeRole: {
        return d->inlineNotesSize;
    }

    case PanelReviewMarksSizeRole: {
        return d->reviewMarksSize;
    }

    case PanelDialoguesWordsSizeRole: {
        return d->dialoguesWordsCount;
    }

    default: {
        return ComicBookTextModelItem::data(_role);
    }
    }
}

QByteArray ComicBookTextModelPanelItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray ComicBookTextModelPanelItem::toXml(ComicBookTextModelItem* _from, int _fromPosition,
                                              ComicBookTextModelItem* _to, int _toPosition,
                                              bool _clearUuid) const
{
    xml::ComicBookTextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() != ComicBookTextModelItemType::Text) {
            xml += child;
            continue;
        }

        //
        // Текстовые блоки, в зависимости от необходимости вставить блок целиком, или его часть
        //
        auto textItem = static_cast<ComicBookTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += { textItem, _fromPosition, _toPosition - _fromPosition };
            } else {
                xml += { textItem, 0, _toPosition };
            }
            break;
        }
        //
        else if (textItem == _from) {
            xml += { textItem, _fromPosition, textItem->text().length() - _fromPosition };
        } else {
            xml += textItem;
        }
    }
    xml += QString("</%1>\n").arg(xml::kContentTag).toUtf8();
    xml += QString("</%1>\n").arg(xml::kPanelTag).toUtf8();

    return xml.data();
}

QByteArray ComicBookTextModelPanelItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    //
    // TODO: plots
    //
    xml += QString("<%1 %2=\"%3\" %4=\"%5\">\n")
               .arg(xml::kPanelTag, xml::kUuidAttribute,
                    _clearUuid ? QUuid::createUuid().toString() : d->uuid.toString(),
                    xml::kPlotsAttribute, {})
               .toUtf8();
    if (d->color.isValid()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(xml::kColorTag, d->color.name()).toUtf8();
    }
    if (!d->stamp.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kStampTag, TextHelper::toHtmlEscaped(d->stamp))
                   .toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void ComicBookTextModelPanelItem::copyFrom(ComicBookTextModelItem* _item)
{
    if (_item->type() != ComicBookTextModelItemType::Panel) {
        Q_ASSERT(false);
        return;
    }

    auto panelItem = static_cast<ComicBookTextModelPanelItem*>(_item);
    d->uuid = panelItem->d->uuid;
    d->number = panelItem->d->number;
    d->color = panelItem->d->color;
    d->stamp = panelItem->d->stamp;
}

bool ComicBookTextModelPanelItem::isEqual(ComicBookTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto panelItem = static_cast<ComicBookTextModelPanelItem*>(_item);
    return d->uuid == panelItem->d->uuid && d->color == panelItem->d->color
        && d->stamp == panelItem->d->stamp;
}

void ComicBookTextModelPanelItem::handleChange()
{
    d->heading.clear();
    d->text.clear();
    d->inlineNotesSize = 0;
    d->reviewMarksSize = 0;
    d->dialoguesWordsCount = 0;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != ComicBookTextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<ComicBookTextModelTextItem*>(child);

        //
        // Собираем текст
        //
        switch (childTextItem->paragraphType()) {
        case ComicBookParagraphType::Panel: {
            d->heading
                = TextHelper::smartToUpper(ComicBookPanelParser::panelTitle(childTextItem->text()));
            const auto panelDescription
                = ComicBookPanelParser::panelDescription(childTextItem->text());
            d->text = panelDescription;
            break;
        }

        case ComicBookParagraphType::Dialogue: {
            d->dialoguesWordsCount += TextHelper::wordsCount(childTextItem->text());
            break;
        }

        case ComicBookParagraphType::InlineNote: {
            ++d->inlineNotesSize;
            break;
        }

        default: {
            if (!d->text.isEmpty() && !childTextItem->text().isEmpty()) {
                d->text.append(" ");
            }
            d->text.append(childTextItem->text());
            d->reviewMarksSize += std::count_if(
                childTextItem->reviewMarks().begin(), childTextItem->reviewMarks().end(),
                [](const ComicBookTextModelTextItem::ReviewMark& _reviewMark) {
                    return !_reviewMark.isDone;
                });
            break;
        }
        }
    }
}

} // namespace BusinessLayer
