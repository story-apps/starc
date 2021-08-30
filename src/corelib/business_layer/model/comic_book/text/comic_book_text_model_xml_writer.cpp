#include "comic_book_text_model_xml_writer.h"

#include "comic_book_text_model_text_item.h"

#include <QString>

namespace BusinessLayer {
namespace xml {

class ComicBookTextModelXmlWriter::Implementation
{
public:
    /**
     * @brief Записать данные текстового элемента в xml, объединяя с последним незаписанным
     * элементом
     */
    void writeTextItemData(const TextItemData& _data = {});


    /**
     * @brief Собственно xml
     */
    QByteArray data;

    /**
     * @brief Последний текстовый блок к добавлению
     */
    TextItemData lastTextItemData;
};

void ComicBookTextModelXmlWriter::Implementation::writeTextItemData(const TextItemData& _data)
{
    auto writeLastTextItemDataAndDestroy = [this] {
        data += lastTextItemData.item->toXml(lastTextItemData.fromPosition,
                                             lastTextItemData.toPosition);
        delete lastTextItemData.item;
        lastTextItemData = {};
    };

    if (_data.item == nullptr) {
        if (lastTextItemData.item == nullptr) {
            return;
        }

        writeLastTextItemDataAndDestroy();
        return;
    }

    if (lastTextItemData.item == nullptr) {
        data += _data.item->toXml(_data.fromPosition, _data.toPosition);
        return;
    }

    lastTextItemData.item->setText(lastTextItemData.item->text() + " ");
    lastTextItemData.item->mergeWith(_data.item);
    Q_ASSERT(_data.fromPosition == 0);
    lastTextItemData.toPosition += _data.toPosition + 1; // +1 - за пробел
    writeLastTextItemDataAndDestroy();
}


// ****


ComicBookTextModelXmlWriter::ComicBookTextModelXmlWriter(bool _addHeader)
    : d(new Implementation)
{
    if (_addHeader) {
        d->data = "<?xml version=\"1.0\"?>\n";
    }
}

void ComicBookTextModelXmlWriter::operator+=(const char* _data)
{
    //
    // Если был незаписанный элемент, записываем его
    //
    d->writeTextItemData();

    d->data += _data;
}

void ComicBookTextModelXmlWriter::operator+=(const QByteArray& _data)
{
    //
    // Если был незаписанный элемент, записываем его
    //
    d->writeTextItemData();

    d->data += _data;
}

void ComicBookTextModelXmlWriter::operator+=(const QString& _data)
{
    //
    // Если был незаписанный элемент, записываем его
    //
    d->writeTextItemData();

    d->data += _data.toUtf8();
}

void ComicBookTextModelXmlWriter::operator+=(ComicBookTextModelItem* _item)
{
    //
    // Текстовые элементы пишем в специальном методе, т.к. возможно понадобится отложенная запись
    //
    if (_item->type() == ComicBookTextModelItemType::Text) {
        auto textItem = static_cast<ComicBookTextModelTextItem*>(_item);
        operator+=({ textItem, 0, textItem->text().length() });
    }
    //
    // Остальные элементы пишем как есть
    //
    else {
        operator+=(_item->toXml());
    }
}

void ComicBookTextModelXmlWriter::operator+=(const TextItemData& _data)
{
    if (_data.item->isCorrection()) {
        return;
    }

    //
    // Если элемент разорван, то сохраняем его и пока не записываем
    //
    if (_data.item->isBreakCorrectionStart()) {
        auto textItem = new ComicBookTextModelTextItem;
        textItem->copyFrom(_data.item);
        d->lastTextItemData = { textItem, _data.fromPosition, _data.toPosition };
    }
    //
    // А если элемент не разорван, запишем его контент, при необходимости соединив с предыдущим
    //
    else {
        d->writeTextItemData(_data);
    }
}

QByteArray ComicBookTextModelXmlWriter::data() const
{
    //
    // Если был незаписанный элемент, записываем его
    //
    d->writeTextItemData();

    return d->data;
}

ComicBookTextModelXmlWriter::~ComicBookTextModelXmlWriter() = default;


} // namespace xml
} // namespace BusinessLayer
