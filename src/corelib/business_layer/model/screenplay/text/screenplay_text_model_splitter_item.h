#pragma once

#include "screenplay_text_model_item.h"

#include <QHash>

class QDomElement;
class QXmlStreamReader;


namespace BusinessLayer
{

/**
 * @brief Тип разделителя
 */
enum class ScreenplayTextModelSplitterItemType {
    Undefined,
    Start,
    End
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
CORE_LIBRARY_EXPORT inline uint qHash(ScreenplayTextModelSplitterItemType _type)
{
    return ::qHash(static_cast<int>(_type));
}


/**
 * @brief Класс элемента разделителя модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelSplitterItem : public ScreenplayTextModelItem
{
public:
    explicit ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType _type);
    explicit ScreenplayTextModelSplitterItem(QXmlStreamReader& _contentReader);
    ~ScreenplayTextModelSplitterItem() override;

    /**
     * @brief Тип разделителя
     */
    ScreenplayTextModelSplitterItemType splitterType() const;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(ScreenplayTextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(ScreenplayTextModelItem* _item) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
