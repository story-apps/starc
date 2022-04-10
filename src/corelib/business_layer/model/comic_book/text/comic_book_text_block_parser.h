#pragma once

#include <QtContainerFwd>

#include <corelib_global.h>

class QString;


namespace BusinessLayer {

/**
 * @brief Парсер текста блока страница
 */
class CORE_LIBRARY_EXPORT ComicBookPageParser
{
public:
    /**
     * @brief Получить номер страницы
     */
    static QString pageNumber(const QString& _text);
};

/**
 * @brief Парсер текста блока панель
 */
class CORE_LIBRARY_EXPORT ComicBookPanelParser
{
public:
    /**
     * @brief Получить заголовок панели
     */
    static QString panelTitle(const QString& _text);

    /**
     * @brief Получить описание панели
     */
    static QString panelDescription(const QString& _text);
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
     * @note [Номер реплики]. [ИМЯ ПЕРСОНАЖА] ([СОСТОЯНИЕ])
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
     * @brief Получить номер реплики
     */
    static QString number(const QString& _text);

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
