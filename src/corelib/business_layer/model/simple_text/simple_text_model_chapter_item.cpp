#include "simple_text_model_chapter_item.h"

#include "simple_text_model_text_item.h"
#include "simple_text_model_xml.h"

#include <business_layer/templates/simple_text_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>


namespace BusinessLayer {

class SimpleTextModelChapterItem::Implementation
{
public:
    /**
     * @brief Идентификатор главы
     */
    QUuid uuid;

    /**
     * @brief Номер главы
     */
    std::optional<Number> number;

    /**
     * @brief Штамп на главе
     */
    QString stamp;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Уровень главы
     */
    int level = 0;

    /**
     * @brief Заголовок главы
     */
    QString heading;

    /**
     * @brief Текст главы
     */
    QString text;

    /**
     * @brief Количество слов главы
     */
    int wordsCount = 0;

    /**
     * @brief Количество заметок по тексту
     */
    int inlineNotesSize = 0;

    /**
     * @brief Количество редакторских заметок
     */
    int reviewMarksSize = 0;
};


// ****


bool SimpleTextModelChapterItem::Number::operator==(
    const SimpleTextModelChapterItem::Number& _other) const
{
    return value == _other.value;
}

SimpleTextModelChapterItem::SimpleTextModelChapterItem()
    : SimpleTextModelItem(TextModelItemType::Chapter)
    , d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

SimpleTextModelChapterItem::SimpleTextModelChapterItem(QXmlStreamReader& _contentReader)
    : SimpleTextModelItem(TextModelItemType::Chapter)
    , d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kChapterTag);

    const auto attributes = _contentReader.attributes();
    if (attributes.hasAttribute(xml::kUuidAttribute)) {
        d->uuid = QUuid::fromString(attributes.value(xml::kUuidAttribute).toString());
    }

    xml::readNextElement(_contentReader);

    auto currentTag = _contentReader.name();
    if (currentTag == xml::kNumberTag) {
        d->number = { _contentReader.attributes().value(xml::kNumberValueAttribute).toString() };
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
            // Если дошли до конца сцены, выходим из обработки
            //
            else if (currentTag == xml::kChapterTag && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (currentTag == xml::kChapterTag) {
                appendItem(new SimpleTextModelChapterItem(_contentReader));
            } else {
                appendItem(new SimpleTextModelTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Соберём заголовок, текст сцены и прочие параметры
    //
    handleChange();
}

SimpleTextModelChapterItem::~SimpleTextModelChapterItem() = default;

SimpleTextModelChapterItem::Number SimpleTextModelChapterItem::number() const
{
    if (!d->number.has_value()) {
        return {};
    }

    return *d->number;
}

void SimpleTextModelChapterItem::setNumber(int _number)
{
    const auto newNumber = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1." : ".%1")
                               .arg(QString::number(_number));
    if (d->number.has_value() && d->number->value == newNumber) {
        return;
    }

    d->number = { newNumber };
}

int SimpleTextModelChapterItem::level() const
{
    return d->level;
}

QString SimpleTextModelChapterItem::heading() const
{
    return d->heading;
}

QString SimpleTextModelChapterItem::text() const
{
    return d->text;
}

int SimpleTextModelChapterItem::wordsCount() const
{
    return d->wordsCount;
}

QVariant SimpleTextModelChapterItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000f021a";
    }

    case ChapterNumberRole: {
        if (d->number.has_value()) {
            return d->number->value;
        }
        return {};
    }

    case ChapterHeadingRole: {
        return d->heading;
    }

    case ChapterTextRole: {
        return d->text;
    }

    case ChapterInlineNotesSizeRole: {
        return d->inlineNotesSize;
    }

    case ChapterReviewMarksSizeRole: {
        return d->reviewMarksSize;
    }

    case ChapterWordsCountRole: {
        return d->wordsCount;
    }

    default: {
        return SimpleTextModelItem::data(_role);
    }
    }
}

QByteArray SimpleTextModelChapterItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray SimpleTextModelChapterItem::toXml(SimpleTextModelItem* _from, int _fromPosition,
                                             SimpleTextModelItem* _to, int _toPosition,
                                             bool _clearUuid) const
{
    QByteArray xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() != TextModelItemType::Text) {
            xml += child->toXml();
            continue;
        }

        //
        // Текстовые блоки, в зависимости от необходимости вставить блок целиком, или его часть
        //
        auto textItem = static_cast<SimpleTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += textItem->toXml(_fromPosition, _toPosition - _fromPosition);
            } else {
                xml += textItem->toXml(0, _toPosition);
            }
            break;
        }
        //
        else if (textItem == _from) {
            xml += textItem->toXml(_fromPosition, textItem->text().length() - _fromPosition);
        } else {
            xml += textItem->toXml();
        }
    }
    xml += QString("</%1>\n").arg(xml::kContentTag).toUtf8();
    xml += QString("</%1>\n").arg(xml::kChapterTag).toUtf8();

    return xml;
}

QByteArray SimpleTextModelChapterItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    if (_clearUuid) {
        xml += QString("<%1>\n").arg(xml::kChapterTag).toUtf8();
    } else {
        xml += QString("<%1 %2=\"%3\">\n")
                   .arg(xml::kChapterTag, xml::kUuidAttribute, d->uuid.toString())
                   .toUtf8();
    }
    if (!d->stamp.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kStampTag, TextHelper::toHtmlEscaped(d->stamp))
                   .toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void SimpleTextModelChapterItem::copyFrom(SimpleTextModelItem* _item)
{
    if (_item->type() != TextModelItemType::Chapter) {
        Q_ASSERT(false);
        return;
    }

    auto chapterItem = static_cast<SimpleTextModelChapterItem*>(_item);
    d->uuid = chapterItem->d->uuid;
    d->number = chapterItem->d->number;
    d->stamp = chapterItem->d->stamp;
}

bool SimpleTextModelChapterItem::isEqual(SimpleTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto chapterItem = static_cast<SimpleTextModelChapterItem*>(_item);
    return d->uuid == chapterItem->d->uuid && d->stamp == chapterItem->d->stamp;
}

void SimpleTextModelChapterItem::handleChange()
{
    d->heading.clear();
    d->text.clear();
    d->inlineNotesSize = 0;
    d->reviewMarksSize = 0;
    d->wordsCount = 0;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Chapter: {
            auto childChapterItem = static_cast<SimpleTextModelChapterItem*>(child);
            const int maxTextLength = 1000;
            if (d->text.length() < maxTextLength) {
                d->text.append(childChapterItem->heading() + " " + childChapterItem->text());
            } else if (d->text.length() > maxTextLength) {
                d->text = d->text.left(maxTextLength);
            }
            d->wordsCount += childChapterItem->wordsCount();
            break;
        }

        case TextModelItemType::Text: {
            auto childTextItem = static_cast<SimpleTextModelTextItem*>(child);
            switch (childTextItem->paragraphType()) {
            case TextParagraphType::Heading1:
            case TextParagraphType::Heading2:
            case TextParagraphType::Heading3:
            case TextParagraphType::Heading4:
            case TextParagraphType::Heading5:
            case TextParagraphType::Heading6: {
                d->level = static_cast<int>(childTextItem->paragraphType());
                d->heading = childTextItem->text();
                d->wordsCount += childTextItem->text().count(' ') + 1;
                break;
            }

            case TextParagraphType::InlineNote: {
                ++d->inlineNotesSize;
                break;
            }

            default: {
                d->text.append(childTextItem->text() + " ");
                d->wordsCount += childTextItem->text().count(' ') + 1;
                d->reviewMarksSize += std::count_if(
                    childTextItem->reviewMarks().begin(), childTextItem->reviewMarks().end(),
                    [](const SimpleTextModelTextItem::ReviewMark& _reviewMark) {
                        return !_reviewMark.isDone;
                    });
                break;
            }
            }

            break;
        }
        }
    }
}

} // namespace BusinessLayer
