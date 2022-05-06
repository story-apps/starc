#pragma once

class QString;


namespace BusinessLayer {

/**
 * @brief Специальный обработчик майм данных с текстом сценария
 */
class ScreenplayTextMimeHandler
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
};

} // namespace BusinessLayer
