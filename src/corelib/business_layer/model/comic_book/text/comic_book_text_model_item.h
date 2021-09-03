#pragma once

#include <business_layer/model/abstract_model_item.h>

#include <corelib_global.h>


namespace BusinessLayer {


/**
 * @brief Перечисление типов элементов модели комикса
 */
enum class ComicBookTextModelItemType { Folder, Page, Panel, Text, Splitter };


/**
 * @brief Базовый класс элемента модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelItem : public AbstractModelItem
{
public:
    explicit ComicBookTextModelItem(ComicBookTextModelItemType _type);
    ~ComicBookTextModelItem() override;

    /**
     * @brief Получить тип элемента
     */
    ComicBookTextModelItemType type() const;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    ComicBookTextModelItem* parent() const override;
    ComicBookTextModelItem* childAt(int _index) const override;

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
    virtual void copyFrom(ComicBookTextModelItem* _item) = 0;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    virtual bool isEqual(ComicBookTextModelItem* _item) const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
