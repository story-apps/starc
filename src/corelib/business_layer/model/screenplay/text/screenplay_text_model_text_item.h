#pragma once

#include "screenplay_text_model_item.h"

class QDomElement;


namespace BusinessLayer
{

enum class ScreenplayParagraphType;

/**
 * @brief Класс элемента текста модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelTextItem : public ScreenplayTextModelItem
{
public:
    ScreenplayTextModelTextItem();
    explicit ScreenplayTextModelTextItem(const QDomElement& _node);
    ~ScreenplayTextModelTextItem() override;

    /**
     * @brief Тип параграфа
     */
    ScreenplayParagraphType paragraphType() const;
    void setParagraphType(ScreenplayParagraphType _type);

    /**
     * @brief Текст элемента
     */
    const QString& text() const;
    void setText(const QString& _text);

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
