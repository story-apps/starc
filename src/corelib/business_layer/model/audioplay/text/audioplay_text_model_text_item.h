#pragma once

#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

class AudioplayTextModel;

/**
 * @brief Класс элемента текста модели аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplayTextModelTextItem : public TextModelTextItem
{

public:
    explicit AudioplayTextModelTextItem(const AudioplayTextModel* _model);
    ~AudioplayTextModelTextItem() override;

    /**
     * @brief Длительность блока
     */
    std::chrono::milliseconds duration() const;
    void updateDuration();

    /**
     * @brief Определяем интерфейс получения данных блока
     */
    QVariant data(int _role) const override;

protected:
    /**
     * @brief Обновляем хронометраж, при изменении текста
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


} // namespace BusinessLayer
