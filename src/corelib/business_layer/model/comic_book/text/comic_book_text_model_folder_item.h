#pragma once

#include <business_layer/model/text/text_model_folder_item.h>


namespace BusinessLayer {

class ComicBookTextModel;

class CORE_LIBRARY_EXPORT ComicBookTextModelFolderItem : public TextModelFolderItem
{
public:
    explicit ComicBookTextModelFolderItem(const ComicBookTextModel* _model, TextFolderType _type);
    ~ComicBookTextModelFolderItem() override;

protected:
    /**
     * @brief Обновляем текст папки при изменении кого-то из детей
     */
    void handleChange() override;
};

} // namespace BusinessLayer
