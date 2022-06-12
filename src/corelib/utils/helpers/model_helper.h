#pragma once

namespace BusinessLayer {
class SimpleTextModel;
}

class ModelHelper
{
public:
    /**
     * @brief Инициилизировать модель титульной страницы
     */
    static void initTitlePageModel(BusinessLayer::SimpleTextModel* _model);

    /**
     * @brief Сбросить модель титульной страницы на дефолтную
     */
    static void resetTitlePageModel(BusinessLayer::SimpleTextModel* _model);
};
