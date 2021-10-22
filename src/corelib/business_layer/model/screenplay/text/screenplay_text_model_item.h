#pragma once

#include <business_layer/model/abstract_model_item.h>

#include <corelib_global.h>


namespace BusinessLayer {

class ScreenplayTextModel;

/**
 * @brief Перечисление типов элементов модели сценария
 */
enum class ScreenplayTextModelItemType { Folder, Scene, Text, Splitter };


/**
 * @brief Базовый класс элемента модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelItem : public AbstractModelItem
{
public:
    explicit ScreenplayTextModelItem(ScreenplayTextModelItemType _type,
                                     const ScreenplayTextModel* _model);
    ~ScreenplayTextModelItem() override;

    /**
     * @brief Получить тип элемента
     */
    ScreenplayTextModelItemType type() const;

    /**
     * @brief Получить модель сценария, в которой находится данный элемент
     */
    const ScreenplayTextModel* model() const;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    ScreenplayTextModelItem* parent() const override;
    ScreenplayTextModelItem* childAt(int _index) const override;

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
    virtual void copyFrom(ScreenplayTextModelItem* _item) = 0;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    virtual bool isEqual(ScreenplayTextModelItem* _item) const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
