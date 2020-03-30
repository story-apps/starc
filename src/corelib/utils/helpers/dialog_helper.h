#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Вспомогательные функции для работы с диалогами
 */
class CORE_LIBRARY_EXPORT DialogHelper
{
public:
    /**
     * @brief Получить фильтр проектов приложения
     */
    static QString starcProjectFilter();

    /**
     * @brief Получить список фильтров файлов которые приложение может импортировать
     */
    static QString importFilters();
};
