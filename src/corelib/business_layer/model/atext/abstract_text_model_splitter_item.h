#pragma once

#include "abstract_text_model_item.h"

#include <QHash>

class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Тип разделителя
 */
enum class AbstractTextModelSplitterItemType {
    Undefined,
    Start,
    End,
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(AbstractTextModelSplitterItemType _type)
{
    return ::qHash(static_cast<int>(_type));
}


/**
 * @brief Класс элемента разделителя модели текста
 */
class CORE_LIBRARY_EXPORT AbstractTextModelSplitterItem : public AbstractTextModelItem
{
public:
    AbstractTextModelSplitterItem(const AbstractTextModel* _model,
                                  AbstractTextModelSplitterItemType _type);
    AbstractTextModelSplitterItem(const AbstractTextModel* _model,
                                  QXmlStreamReader& _contentReader);
    ~AbstractTextModelSplitterItem() override;

    /**
     * @brief Тип разделителя
     */
    AbstractTextModelSplitterItemType splitterType() const;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(AbstractTextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(AbstractTextModelItem* _item) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
