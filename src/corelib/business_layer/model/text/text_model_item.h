#pragma once

#include <business_layer/model/abstract_model_item.h>

#include <corelib_global.h>


namespace BusinessLayer {


/**
 * @brief Перечисление типов элементов модели текста
 */
enum class TextModelItemType { Chapter, Text };


/**
 * @brief Базовый класс элемента модели текста
 */
class CORE_LIBRARY_EXPORT TextModelItem : public AbstractModelItem
{
public:
    explicit TextModelItem(TextModelItemType _type);
    ~TextModelItem() override;

    /**
     * @brief Получить тип элемента
     */
    TextModelItemType type() const;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    TextModelItem* parent() const override;
    TextModelItem* childAt(int _index) const override;

    /**
     * @brief Определяем интерфейс получения данных элемента
     */
    QVariant data(int _role) const override;

    /**
     * @brief Сформировать xml блока
     */
    virtual QByteArray toXml() const = 0;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    virtual void copyFrom(TextModelItem* _item) = 0;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    virtual bool isEqual(TextModelItem* _item) const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
