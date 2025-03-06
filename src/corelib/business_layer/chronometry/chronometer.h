#pragma once

#include <chrono>
#include <corelib_global.h>

class QTextBlock;


namespace BusinessLayer {

enum class TextParagraphType;

/**
 * @brief Тип счётчика хронометража
 */
enum class CORE_LIBRARY_EXPORT ChronometerType {
    Page,
    Characters,
    Words,
    Sophocles,
};

/**
 * @brief Параметры хронометража
 */
struct ChronometerOptions {
    /**
     * @brief Тип хронометража
     */
    ChronometerType type = ChronometerType::Page;

    /**
     * @brief Хронометраж по страницам
     */
    struct {
        int seconds = 60;
    } page;

    /**
     * @brief Хронометраж по символам
     */
    struct {
        int characters = 1350;
        bool considerSpaces = true;
        int seconds = 60;
    } characters;

    /**
     * @brief Хронометраж по словам
     */
    struct {
        int words = 140;
        int seconds = 60;
    } words;

    /**
     * @brief Конфигурируемый хронометраж а-ля Софокл
     */
    struct {
        qreal secsPerAction = 1.0;
        qreal secsPerEvery50Action = 1.5;
        qreal secsPerDialogue = 2.0;
        qreal secsPerEvery50Dialogue = 2.4;
        qreal secsPerSceneHeading = 2.0;
        qreal secsPerEvery50SceneHeading = 0.0;
    } sophocles;
};

/**
 * @brief Определяем оператор сравнения опций хронометража
 */
inline bool operator==(const ChronometerOptions& _lhs, const ChronometerOptions& _rhs)
{
    return _lhs.type == _rhs.type
        //
        && _lhs.page.seconds == _rhs.page.seconds
        //
        && _lhs.characters.characters == _rhs.characters.characters
        && _lhs.characters.considerSpaces == _rhs.characters.considerSpaces
        && _lhs.characters.seconds == _rhs.characters.seconds
        //
        && _lhs.words.words == _rhs.words.words
        && _lhs.words.seconds == _rhs.words.seconds
        //
        && _lhs.sophocles.secsPerAction == _rhs.sophocles.secsPerAction
        && _lhs.sophocles.secsPerEvery50Action == _rhs.sophocles.secsPerEvery50Action
        && _lhs.sophocles.secsPerDialogue == _rhs.sophocles.secsPerDialogue
        && _lhs.sophocles.secsPerEvery50Dialogue == _rhs.sophocles.secsPerEvery50Dialogue
        && _lhs.sophocles.secsPerSceneHeading == _rhs.sophocles.secsPerSceneHeading
        && _lhs.sophocles.secsPerEvery50SceneHeading == _rhs.sophocles.secsPerEvery50SceneHeading;
}

/**
 * @brief Фасад для вычисления хронометража способом, настроенным пользователем
 */
class CORE_LIBRARY_EXPORT ScreenplayChronometer
{
public:
    /**
     * @brief Определить длительность заданного блока
     */
    static std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                              const QString& _templateId,
                                              const ChronometerOptions& _options);
};

/**
 * @brief Фасад для вычисления хронометража способом, настроенным пользователем
 */
class CORE_LIBRARY_EXPORT AudioplayChronometer
{
public:
    /**
     * @brief Определить длительность заданного блока
     */
    static std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                              const QString& _templateId,
                                              const ChronometerOptions& _options);
};

} // namespace BusinessLayer
