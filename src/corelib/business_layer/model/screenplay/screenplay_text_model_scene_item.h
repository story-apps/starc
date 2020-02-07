#pragma once

#include "screenplay_text_model_item.h"


namespace BusinessLayer
{

/**
 * @brief Класс элементов сцен модели сценария
 */
class ScreenplayTextModelSceneItem : public ScreenplayTextModelItem
{
public:
    ScreenplayTextModelSceneItem();
    ~ScreenplayTextModelSceneItem() override;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
