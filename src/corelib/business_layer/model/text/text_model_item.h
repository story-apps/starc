#pragma once

#include <business_layer/model/abstract_model_item.h>


namespace BusinessLayer {

class TextModel;

/**
 * @brief Перечисление типов элементов модели текста
 */
enum class TextModelItemType {
    //
    // Элементы имеющие открывающий и закрывающий блоки (Акты и папки)
    //
    Folder,
    //
    // Элементы имеющие только открывающий блок (сцена, бит, страница, панель, глава)
    //
    Group,
    //
    // Текстовый элемент - параграф текста
    //
    Text,
    //
    // Разделитель страницы на таблицу
    //
    Splitter,
};


/**
 * @brief Базовый класс элемента модели текста
 */
class TextModelItem : public AbstractModelItem
{
public:
    TextModelItem(TextModelItemType _type, const TextModel* _model);
    ~TextModelItem() override;

    /**
     * @brief Получить тип элемента
     */
    const TextModelItemType& type() const;

    /**
     * @brief Получить модель, в которой находится данный элемент
     */
    const TextModel* model() const;

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
