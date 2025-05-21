#pragma once

#include <corelib_global.h>

class QString;


namespace BusinessLayer {

/**
 * @brief Специальный обработчик майм данных с текстом сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextMimeHandler
{
public:
    /**
     * @brief Удалить невидимые блоки в режиме тритмента, или сценария
     */
    static QString removeInvisibleBlocksForTreatment(const QString& _mime);

    /**
     * @brief Преобразовать все текстовые блоки в биты
     */
    static QString convertTextBlocksToBeats(const QString& _mime);

    /**
     * @brief Преобразовать все биты в текстовые блоки
     */
    static QString convertBeatsToTextBlocks(const QString& _mime);
};

} // namespace BusinessLayer
