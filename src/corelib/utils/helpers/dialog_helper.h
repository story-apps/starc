#pragma once

class QString;


/**
 * @brief Вспомогательные функции для работы с диалогами
 */
class DialogHelper
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
