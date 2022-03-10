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
class CORE_LIBRARY_EXPORT SimpleTextModelItem : public AbstractModelItem
{
public:
    explicit SimpleTextModelItem(TextModelItemType _type);
    ~SimpleTextModelItem() override;

    /**
     * @brief Получить тип элемента
     */
    const TextModelItemType& type() const;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    SimpleTextModelItem* parent() const override;
    SimpleTextModelItem* childAt(int _index) const override;

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
    virtual void copyFrom(SimpleTextModelItem* _item) = 0;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    virtual bool isEqual(SimpleTextModelItem* _item) const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
