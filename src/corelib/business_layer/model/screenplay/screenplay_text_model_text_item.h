#pragma once

#include "screenplay_text_model_item.h"


namespace BusinessLayer
{

/**
 * @brief Класс элемента текста модели сценария
 */
class ScreenplayTextModelTextItem : public ScreenplayTextModelItem
{
public:
    ScreenplayTextModelTextItem();
    ~ScreenplayTextModelTextItem() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
