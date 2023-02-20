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
     * @brief Количество слов
     */
    int wordsCount() const;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount() const;

    /**
     * @brief Обновить счётчики
     */
    void updateCounters();

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
