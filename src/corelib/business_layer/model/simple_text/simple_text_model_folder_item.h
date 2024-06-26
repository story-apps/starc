#pragma once

#include <business_layer/model/text/text_model_folder_item.h>


namespace BusinessLayer {

class SimpleTextModel;

class CORE_LIBRARY_EXPORT SimpleTextModelFolderItem : public TextModelFolderItem
{
public:
    SimpleTextModelFolderItem(const SimpleTextModel* _model, TextFolderType _type);
    ~SimpleTextModelFolderItem() override;

protected:
    /**
     * @brief Обновляем текст папки при изменении кого-то из детей
     */
    void handleChange() override;
};

} // namespace BusinessLayer
