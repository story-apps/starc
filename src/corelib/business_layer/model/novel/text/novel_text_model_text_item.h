#pragma once

#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

class NovelTextModel;

/**
 * @brief Класс элемента текста модели сценария
 */
class CORE_LIBRARY_EXPORT NovelTextModelTextItem : public TextModelTextItem
{
public:
    explicit NovelTextModelTextItem(const NovelTextModel* _model);
    ~NovelTextModelTextItem() override;

    /**
     * @brief Обновить счётчики
     */
    void updateCounters(bool _force = false) override;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;
};


} // namespace BusinessLayer
