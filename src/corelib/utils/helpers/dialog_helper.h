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
    static QString starcTemplateFilter();
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
    static QString pngFilter();

    /**
     * @brief Получить список фильтров файлов которые приложение может открыть
     */
    static QString filtersForOpenProject();

    /**
     * @brief Получить список фильтров файлов которые приложение может импортировать
     */
    static QString filtersForImport();

    /**
     * @brief Получить список фильтров файлов, в которые можно сохранять содержимое сцены
     */
    static QString filtersForSceneImage();
};
