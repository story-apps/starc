#pragma once

class QString;

namespace BusinessLayer {
enum class ScreenplayParagraphType;
}


/**
 * @brief Вспомогательный класс для работы с горячими клавишами
 */
class ShortcutsHelper
{
public:
    //
    // Сценарий
    //
    static QString screenplayShortcut(BusinessLayer::ScreenplayParagraphType _type);
    static QString screenplayJumpByTab(BusinessLayer::ScreenplayParagraphType _type);
    static QString screenplayJumpByEnter(BusinessLayer::ScreenplayParagraphType _type);
    static QString screenplayChangeByTab(BusinessLayer::ScreenplayParagraphType _type);
    static QString screenplayChangeByEnter(BusinessLayer::ScreenplayParagraphType _type);
};
