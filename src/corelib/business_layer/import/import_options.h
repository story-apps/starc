#pragma once

#include <domain/document_object.h>

#include <QString>

#include <corelib_global.h>

namespace BusinessLayer {

/**
 * @brief Опции импорта
 */
struct CORE_LIBRARY_EXPORT ImportOptions {
    /**
     * @brief Путь файла для импорта
     */
    QString filePath;

    /**
     * @brief Тип импортируемого документа
     */
    Domain::DocumentObjectType documentType = Domain::DocumentObjectType::Undefined;

    /**
     * @brief Нужно ли импортировать персонажей
     */
    bool importCharacters = true;

    /**
     * @brief Нужно ли импортировать локации
     */
    bool importLocations = true;

    /**
     * @brief Нужно ли импортировать основной текст документа
     */
    bool importText = true;
};

} // namespace BusinessLayer
