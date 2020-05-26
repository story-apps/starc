#pragma once

#include "screenplay_text_model_item.h"

#include <QHash>

class QDomElement;


namespace BusinessLayer
{

/**
 * @brief Тип разделителя
 */
enum class ScreenplayTextModelSplitterItemType {
    Undefined,
    Start,
    Middle,
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
    explicit ScreenplayTextModelSplitterItem(const QDomElement& _node);
    ~ScreenplayTextModelSplitterItem() override;

    /**
     * @brief Тип разделителя
     */
    ScreenplayTextModelSplitterItemType splitterType() const;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QString toXml() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
