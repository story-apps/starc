#include "screenplay_template.h"


namespace BusinessLayer
{

namespace {
    const QHash<ScreenplayParagraphType, QString> kScreenplayParagraphTypeToString
    = {{ ScreenplayParagraphType::UnformattedText, QLatin1String("unformatted_text") },
       { ScreenplayParagraphType::SceneName, QLatin1String("scene_name") },
       { ScreenplayParagraphType::SceneDescription, QLatin1String("scene_description") },
       { ScreenplayParagraphType::SceneHeading, QLatin1String("scene_heading") },
       { ScreenplayParagraphType::SceneCharacters, QLatin1String("scene_characters") },
       { ScreenplayParagraphType::Action, QLatin1String("action") },
       { ScreenplayParagraphType::Character, QLatin1String("character") },
       { ScreenplayParagraphType::Parenthetical, QLatin1String("parenthetical") },
       { ScreenplayParagraphType::Dialogue, QLatin1String("dialogue") },
       { ScreenplayParagraphType::Lyrics, QLatin1String("lyrics") },
       { ScreenplayParagraphType::Transition, QLatin1String("trnsition") },
       { ScreenplayParagraphType::Shot, QLatin1String("show") },
       { ScreenplayParagraphType::InlineNote, QLatin1String("inline_note") },
       { ScreenplayParagraphType::FolderHeader, QLatin1String("folder_header") },
       { ScreenplayParagraphType::FolderFooter, QLatin1String("folder_footer") }};
}


QString toString(ScreenplayParagraphType _type)
{
    return kScreenplayParagraphTypeToString.value(_type);
}

ScreenplayParagraphType screenplayParagraphTypeFromString(const QString& _text)
{
    return kScreenplayParagraphTypeToString.key(_text, ScreenplayParagraphType::Undefined);
}

ScreenplayTemplate::ScreenplayTemplate()
{

}

} // namespace BusinessLayer
