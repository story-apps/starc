#pragma once

#include <business_layer/model/text/text_model_group_item.h>


namespace BusinessLayer {

class SimpleTextModel;

/**
 * @brief Класс элементов глав модели текста
 */
class CORE_LIBRARY_EXPORT SimpleTextModelChapterItem : public TextModelGroupItem
{
public:
    enum {
        ChapterWordsCountRole = TextModelGroupItem::GroupUserRole + 1,
    };

public:
    explicit SimpleTextModelChapterItem(const SimpleTextModel* _model, TextGroupType _type);
    ~SimpleTextModelChapterItem() override;

    /**
     * @brief Определяем интерфейс получения данных главы
     */
    QVariant data(int _role) const override;

protected:
    /**
     * @brief Обновляем текст главы при изменении кого-то из детей
     */
    void handleChange() override;
};

} // namespace BusinessLayer
