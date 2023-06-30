#pragma once

#include <business_layer/model/abstract_model_item.h>


namespace BusinessLayer {

/**
 * @brief Тип элемента модели проектов
 */
enum class ProjectsModelItemType {
    //
    // Корень дерева проектов
    //
    Root,
    //
    // Команда
    //
    Team,
    //
    // Проект
    //
    Project,
};

/**
 * @brief Абстрактный класс элемента модели
 */
class ProjectsModelItem : public AbstractModelItem
{
public:
    ProjectsModelItem();

    /**
     * @brief Получить тип элемента
     */
    virtual ProjectsModelItemType type() const;

    /**
     * @brief Данные элемента
     */
    QVariant data(int _role) const override;

    /**
     * @brief Родительский элемент
     */
    ProjectsModelItem* parent() const override;
};

} // namespace BusinessLayer
