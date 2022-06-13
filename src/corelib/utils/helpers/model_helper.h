#pragma once

#include <corelib_global.h>

namespace BusinessLayer {
class SimpleTextModel;
}


/**
 * @brief Вспомогательные методы для моделей данных
 */
class CORE_LIBRARY_EXPORT ModelHelper
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
