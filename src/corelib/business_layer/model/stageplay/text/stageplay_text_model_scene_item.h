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
     * @brief Считываем дополнительный контент
     */
    QStringRef readCustomContent(QXmlStreamReader& _contentReader) override;

    /**
     * @brief Сформировать xml-блок с кастомными данными элемента
     */
    QByteArray customContent() const override;

    /**
     * @brief Обновляем текст сцены при изменении кого-то из детей
     */
    void handleChange() override;
};

} // namespace BusinessLayer
