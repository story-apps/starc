#pragma once

#include <corelib_global.h>

class QString;

namespace BusinessLayer {
enum class TextParagraphType;
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
    // ... тритмент
    //
    static QString treatmentShortcut(BusinessLayer::TextParagraphType _type);
    static void setTreatmentShortcut(BusinessLayer::TextParagraphType _type,
                                     const QString& _shortcut);
    //
    static QString treatmentJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setTreatmentJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                      BusinessLayer::TextParagraphType _toType);
    //
    static QString treatmentJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setTreatmentJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString treatmentChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setTreatmentChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString treatmentChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setTreatmentChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType);
    //
    // ... текст
    //
    static QString screenplayShortcut(BusinessLayer::TextParagraphType _type);
    static void setScreenplayShortcut(BusinessLayer::TextParagraphType _type,
                                      const QString& _shortcut);
    //
    static QString screenplayJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setScreenplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                       BusinessLayer::TextParagraphType _toType);
    //
    static QString screenplayJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setScreenplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                         BusinessLayer::TextParagraphType _toType);
    //
    static QString screenplayChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setScreenplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                         BusinessLayer::TextParagraphType _toType);
    //
    static QString screenplayChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setScreenplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                           BusinessLayer::TextParagraphType _toType);
};
