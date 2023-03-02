#pragma once

#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

class ComicBookTextModel;

/**
 * @brief Класс элемента текста модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelTextItem : public TextModelTextItem
{

public:
    explicit ComicBookTextModelTextItem(const ComicBookTextModel* _model);

protected:
    /**
     * @brief Текст элемента, который будет сохранён
     * @note Убираем номер для имени персонажа
     */
    virtual QString textToSave() const;
};


} // namespace BusinessLayer
