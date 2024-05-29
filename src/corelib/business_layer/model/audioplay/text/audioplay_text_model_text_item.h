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

    /**
     * @brief Обновить счётчики
     */
    void updateCounters(bool _force = false) override;

    /**
     * @brief Определяем интерфейс получения данных блока
     */
    QVariant data(int _role) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


} // namespace BusinessLayer
