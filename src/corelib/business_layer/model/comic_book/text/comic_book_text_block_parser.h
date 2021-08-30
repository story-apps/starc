#pragma once

#include <corelib_global.h>

class QString;
class QStringList;


namespace BusinessLayer {

/**
 * @brief Парсер текста блока время и место
 */
class CORE_LIBRARY_EXPORT ComicBookPanelParser
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
        SectionStoryDay, //!< ДЕНЬ ИСТОРИИ
    };

public:
    /**
     * @brief Получить секцию блока
     */
    static ComicBookPanelParser::Section section(const QString& _text);

    /**
     * @brief Получить название места
     */
    static QString sceneIntro(const QString& _text);

    /**
     * @brief Получить название локации, если задан \p _force, то берём текст до конца
     */
    static QString location(const QString& _text, bool _force = false);

    /**
     * @brief Получить название сценарного дня
     */
    static QString storyDay(const QString& _text);

    /**
     * @brief Получить название времени
     */
    static QString sceneTime(const QString& _text);
};


/**
 * @brief Парсер текста блока персонаж
 */
class CORE_LIBRARY_EXPORT ComicBookCharacterParser
{
public:
    /**
     * @brief Секции блока персонаж
     *
     * @note [ИМЯ ПЕРСОНАЖА] ([СОСТОЯНИЕ])
     */
    enum Section {
        SectionUndefined, //!< Неопределённое
        SectionName, //!< ИМЯ
        SectionState //!< СОСТОЯНИЕ
    };

public:
    /**
     * @brief Получить секцию блока
     */
    static ComicBookCharacterParser::Section section(const QString& _text);

    /**
     * @brief Получить имя персонажа
     */
    static QString name(const QString& _text);

    /**
     * @brief Получить состояние персонажа
     */
    static QString extension(const QString& _text);
};

} // namespace BusinessLayer
