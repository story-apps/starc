#pragma once

#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

class ScreenplayTextModel;

/**
 * @brief Класс элемента текста модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelTextItem : public TextModelTextItem
{
public:
    explicit ScreenplayTextModelTextItem(const ScreenplayTextModel* _model);
    ~ScreenplayTextModelTextItem() override;

    /**
     * @brief Количество слов
     */
    int wordsCount() const;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount() const;

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Обновить счётчики
     */
    void updateCounters(bool _force = false);

    /**
     * @brief Определяем интерфейс получения данных сцены
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
