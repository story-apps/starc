#pragma once

#include "screenplay_text_model_item.h"


namespace BusinessLayer
{

/**
 * @brief Тип разделителя
 */
enum class ScreenplayTextModelSplitterItemType {
    Start,
    Splitter,
    End
};


/**
 * @brief Класс элемента разделителя модели сценария
 */
class ScreenplayTextModelSplitterItem : public ScreenplayTextModelItem
{
public:
    explicit ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType _type);
    ~ScreenplayTextModelSplitterItem() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
