#pragma once

#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

class SimpleTextModel;

/**
 * @brief Элемент модели текстового документа
 */
class CORE_LIBRARY_EXPORT SimpleTextModelTextItem : public TextModelTextItem
{
public:
    explicit SimpleTextModelTextItem(const SimpleTextModel* _model);

    /**
     * @brief Определяем интерфейс получения данных элемента
     */
    QVariant data(int _role) const override;
};

} // namespace BusinessLayer
