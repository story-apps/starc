#pragma once

#include <corelib_global.h>

class QString;


namespace BusinessLayer {

/**
 * @brief Парсер текста блока персонаж
 */
class CORE_LIBRARY_EXPORT AudioplayCharacterParser
{
public:
    /**
     * @brief Получить имя персонажа
     */
    static QString name(const QString& _text);

    /**
     * @brief Получить состояние персонажа
     */
    static QString extension(const QString& _text);
};


/**
 * @brief Парсер текста блока время и место
 */
class CORE_LIBRARY_EXPORT AudioplaySceneHeadingParser
{
public:
    /**
     * @brief Секции блока заголовка сцены
     *
     * @note [МЕСТО]. [ЛОКАЦИЯ] - [ВРЕМЯ], [ДЕНЬ ИСТОРИИ]
     */
    enum Section {
        SectionUndefined, //!< Неопределённое
        SectionSceneIntro, //!< МЕСТО
        SectionLocation, //!< ЛОКАЦИЯ
        SectionSceneTime, //!< ВРЕМЯ
    };

public:
    /**
     * @brief Получить секцию блока
     */
    static AudioplaySceneHeadingParser::Section section(const QString& _text);

    /**
     * @brief Получить название места
     */
    static QString sceneIntro(const QString& _text);

    /**
     * @brief Получить название локации, если задан \p _force, то берём текст до конца
     */
    static QString location(const QString& _text, bool _force = false);

    /**
     * @brief Получить название времени
     */
    static QString sceneTime(const QString& _text);
};

} // namespace BusinessLayer
