#pragma once

#include <business_layer/model/abstract_model_item.h>


namespace BusinessLayer {

/**
 * @brief Тип элемента модели проектов
 */
enum class ScreenplaySeriesEpisodesModelItemType {
    //
    // Корень дерева
    //
    Root,
    //
    // Серия
    //
    Episode,
    //
    // Сюжетная линия
    //
    StoryLine,
};

/**
 * @brief Абстрактный класс элемента модели
 */
class ScreenplaySeriesEpisodesModelItem : public AbstractModelItem
{
public:
    ScreenplaySeriesEpisodesModelItem();

    /**
     * @brief Получить тип элемента
     */
    virtual ScreenplaySeriesEpisodesModelItemType type() const;

    /**
     * @brief Данные элемента
     */
    QVariant data(int _role) const override;

    /**
     * @brief Родительский элемент
     */
    ScreenplaySeriesEpisodesModelItem* parent() const override;
};

} // namespace BusinessLayer
