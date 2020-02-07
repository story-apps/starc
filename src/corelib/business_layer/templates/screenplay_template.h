#pragma once


namespace BusinessLayer
{

/**
 * @brief Типы параграфов в сценарии
 */
enum class ScreenplayParagraphType {
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
    Folder
};


/**
 * @brief Класс шаблона оформления сценария
 */
class ScreenplayTemplate
{
public:
    ScreenplayTemplate();
};

} // namespace BusinessLayer
