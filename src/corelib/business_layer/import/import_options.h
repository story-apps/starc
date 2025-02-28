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
     * @brief Юид документа, в который производится импорт
     */
    QUuid documentUuid;

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
     * @brief Нужно ли импортировать текст
     */
    bool importText = true;

    /**
     * @brief Нужно ли импортировать документы
     */
    bool importResearch = true;

    /**
     * @brief Сохранять номера сцен импортируемого сценария
     */
    bool keepSceneNumbers = false;
};

} // namespace BusinessLayer
