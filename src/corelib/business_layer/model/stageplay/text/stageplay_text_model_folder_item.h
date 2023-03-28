#pragma once

#include <business_layer/model/text/text_model_folder_item.h>


namespace BusinessLayer {

class StageplayTextModel;

/**
 * @brief Класс элементов папок модели пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTextModelFolderItem : public TextModelFolderItem
{
public:
    explicit StageplayTextModelFolderItem(const StageplayTextModel* _model, TextFolderType _type);
    ~StageplayTextModelFolderItem() override;

protected:
    /**
     * @brief Обновляем текст папки при изменении кого-то из детей
     */
    void handleChange() override;
};

} // namespace BusinessLayer
