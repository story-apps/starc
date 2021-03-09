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
     * @brief Получить фильтр конкретного типа
     */
    static QString starcProjectFilter();
    static QString kitScenaristFilter();
    static QString finalDraftFilter();
    static QString finalDraftTemplateFilter();
    static QString trelbyFilter();
    static QString msWordFilter();
    static QString openDocumentXmlFilter();
    static QString fountainFilter();
    static QString celtxFilter();
    static QString plainTextFilter();
    static QString pdfFilter();

    /**
     * @brief Получить список фильтров файлов которые приложение может импортировать
     */
    static QString importFilters();
};
