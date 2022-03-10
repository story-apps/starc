#pragma once

#include "text_model_item.h"

#include <QHash>

class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Тип разделителя
 */
enum class TextModelSplitterItemType {
    Undefined,
    Start,
    End,
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(TextModelSplitterItemType _type)
{
    return ::qHash(static_cast<int>(_type));
}


/**
 * @brief Класс элемента разделителя модели текста
 */
class CORE_LIBRARY_EXPORT TextModelSplitterItem : public TextModelItem
{
public:
    TextModelSplitterItem(const TextModel* _model,
                                  TextModelSplitterItemType _type);
    TextModelSplitterItem(const TextModel* _model,
                                  QXmlStreamReader& _contentReader);
    ~TextModelSplitterItem() override;

    /**
     * @brief Тип разделителя
     */
    TextModelSplitterItemType splitterType() const;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelItem* _item) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
