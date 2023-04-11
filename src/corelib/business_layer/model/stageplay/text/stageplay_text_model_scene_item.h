#pragma once

#include <business_layer/model/text/text_model_group_item.h>


namespace BusinessLayer {

class StageplayTextModel;

/**
 * @brief Класс элементов сцен модели пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTextModelSceneItem : public TextModelGroupItem
{
public:
    explicit StageplayTextModelSceneItem(const StageplayTextModel* _model);
    ~StageplayTextModelSceneItem() override;

protected:
    /**
     * @brief Обновляем текст сцены при изменении кого-то из детей
     */
    void handleChange() override;
};

} // namespace BusinessLayer
