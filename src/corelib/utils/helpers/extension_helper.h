#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Вспомогательный класс для работы с возможными разрешениями файлов
 */
class CORE_LIBRARY_EXPORT ExtensionHelper
{
public:
    static QString starc();
    static QString kitScenarist();
    static QString finalDraft();
    static QString finalDraftTemplate();
    static QString trelby();
    static QString msOfficeBinary();
    static QString msOfficeOpenXml();
    static QString openDocumentXml();
    static QString fountain();
    static QString celtx();
    static QString plainText();
    static QString pdf();
};
