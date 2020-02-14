#pragma once

#include <QHash>


namespace BusinessLayer
{

/**
 * @brief Типы параграфов в сценарии
 */
enum class ScreenplayParagraphType {
    Undefined,
    UnformattedText,
    SceneName,
    SceneDescription,
    SceneHeading,
    SceneCharacters,
    Action,
    Character,
    Parenthetical,
    Dialogue,
    Lyrics,
    Transition,
    Shot,
    InlineNote,
    FolderHeader,
    FolderFooter
};

/**
 * @brief Определим метод для возможности использовать типы в виде ключей в словарях
 */
inline uint qHash(ScreenplayParagraphType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить текстовое представление типа блока
 */
QString toString(ScreenplayParagraphType _type);

/**
 * @brief Получить тип блока из текстового представления
 */
ScreenplayParagraphType screenplayParagraphTypeFromString(const QString& _text);


/**
 * @brief Класс шаблона оформления сценария
 */
class ScreenplayTemplate
{
public:
    ScreenplayTemplate();
};

} // namespace BusinessLayer
