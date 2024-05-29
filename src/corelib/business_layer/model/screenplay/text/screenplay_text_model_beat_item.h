#pragma once

#include <business_layer/model/text/text_model_group_item.h>

#include <chrono>


namespace BusinessLayer {

class ScreenplayTextModel;

/**
 * @brief Класс элементов битов модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelBeatItem : public TextModelGroupItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        BeatDurationRole = TextModelGroupItem::GroupUserRole + 1,
    };

public:
    explicit ScreenplayTextModelBeatItem(const ScreenplayTextModel* _model);
    ~ScreenplayTextModelBeatItem() override;

    /**
     * @brief Длительность бита
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Определяем интерфейс получения данных бита
     */
    QVariant data(int _role) const override;

protected:
    /**
     * @brief Обновляем текст бита при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
