#pragma once

#include <corelib_global.h>

class QString;

namespace BusinessLayer {
enum class ScreenplayParagraphType;
}


/**
 * @brief Вспомогательный класс для работы с горячими клавишами
 */
class CORE_LIBRARY_EXPORT ShortcutsHelper
{
public:
    //
    // Сценарий
    //

    static QString screenplayShortcut(BusinessLayer::ScreenplayParagraphType _type);
    static void setScreenplayShortcut(BusinessLayer::ScreenplayParagraphType _type,
                                      const QString& _shortcut);

    static QString screenplayJumpByTab(BusinessLayer::ScreenplayParagraphType _type);
    static void setScreenplayJumpByTab(BusinessLayer::ScreenplayParagraphType _fromType,
                                       BusinessLayer::ScreenplayParagraphType _toType);

    static QString screenplayJumpByEnter(BusinessLayer::ScreenplayParagraphType _type);
    static void setScreenplayJumpByEnter(BusinessLayer::ScreenplayParagraphType _fromType,
                                         BusinessLayer::ScreenplayParagraphType _toType);

    static QString screenplayChangeByTab(BusinessLayer::ScreenplayParagraphType _type);
    static void setScreenplayChangeByTab(BusinessLayer::ScreenplayParagraphType _fromType,
                                         BusinessLayer::ScreenplayParagraphType _toType);

    static QString screenplayChangeByEnter(BusinessLayer::ScreenplayParagraphType _type);
    static void setScreenplayChangeByEnter(BusinessLayer::ScreenplayParagraphType _fromType,
                                           BusinessLayer::ScreenplayParagraphType _toType);
};
