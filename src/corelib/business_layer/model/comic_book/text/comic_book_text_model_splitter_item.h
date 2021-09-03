#pragma once

#include "comic_book_text_model_item.h"

#include <QHash>

class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Тип разделителя
 */
enum class ComicBookTextModelSplitterItemType { Undefined, Start, End };

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(ComicBookTextModelSplitterItemType _type)
{
    return ::qHash(static_cast<int>(_type));
}


/**
 * @brief Класс элемента разделителя модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelSplitterItem : public ComicBookTextModelItem
{
public:
    explicit ComicBookTextModelSplitterItem(ComicBookTextModelSplitterItemType _type);
    explicit ComicBookTextModelSplitterItem(QXmlStreamReader& _contentReader);
    ~ComicBookTextModelSplitterItem() override;

    /**
     * @brief Тип разделителя
     */
    ComicBookTextModelSplitterItemType splitterType() const;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(ComicBookTextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(ComicBookTextModelItem* _item) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
