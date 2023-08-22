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
    // Простой текст
    //
    static QString simpleTextShortcut(BusinessLayer::TextParagraphType _type);
    static void setSimpleTextShortcut(BusinessLayer::TextParagraphType _type,
                                      const QString& _shortcut);
    //
    static QString simpleTextJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setSimpleTextJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                       BusinessLayer::TextParagraphType _toType);
    //
    static QString simpleTextJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setSimpleTextJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                         BusinessLayer::TextParagraphType _toType);
    //
    static QString simpleTextChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setSimpleTextChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                         BusinessLayer::TextParagraphType _toType);
    //
    static QString simpleTextChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setSimpleTextChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                           BusinessLayer::TextParagraphType _toType);

    //
    // Сценарий
    //
    // ... тритмент
    //
    static QString screenplayTreatmentShortcut(BusinessLayer::TextParagraphType _type);
    static void setScreenplayTreatmentShortcut(BusinessLayer::TextParagraphType _type,
                                               const QString& _shortcut);
    //
    static QString screenplayTreatmentJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setScreenplayTreatmentJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                                BusinessLayer::TextParagraphType _toType);
    //
    static QString screenplayTreatmentJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setScreenplayTreatmentJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                                  BusinessLayer::TextParagraphType _toType);
    //
    static QString screenplayTreatmentChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setScreenplayTreatmentChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                                  BusinessLayer::TextParagraphType _toType);
    //
    static QString screenplayTreatmentChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setScreenplayTreatmentChangeByEnter(BusinessLayer::TextParagraphType _fromType,
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

    //
    // Комикс
    //
    // ... текст
    //
    static QString comicBookShortcut(BusinessLayer::TextParagraphType _type);
    static void setComicBookShortcut(BusinessLayer::TextParagraphType _type,
                                     const QString& _shortcut);
    //
    static QString comicBookJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setComicBookJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                      BusinessLayer::TextParagraphType _toType);
    //
    static QString comicBookJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setComicBookJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString comicBookChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setComicBookChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString comicBookChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setComicBookChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType);

    //
    // Аудиопостановка
    //
    // ... текст
    //
    static QString audioplayShortcut(BusinessLayer::TextParagraphType _type);
    static void setAudioplayShortcut(BusinessLayer::TextParagraphType _type,
                                     const QString& _shortcut);
    //
    static QString audioplayJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setAudioplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                      BusinessLayer::TextParagraphType _toType);
    //
    static QString audioplayJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setAudioplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString audioplayChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setAudioplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString audioplayChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setAudioplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType);

    //
    // Пьеса
    //
    // ... текст
    //
    static QString stageplayShortcut(BusinessLayer::TextParagraphType _type);
    static void setStageplayShortcut(BusinessLayer::TextParagraphType _type,
                                     const QString& _shortcut);
    //
    static QString stageplayJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setStageplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                      BusinessLayer::TextParagraphType _toType);
    //
    static QString stageplayJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setStageplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString stageplayChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setStageplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType);
    //
    static QString stageplayChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setStageplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType);

    //
    // Роман
    //
    // ... план
    //
    static QString novelOutlineShortcut(BusinessLayer::TextParagraphType _type);
    static void setNovelOutlineShortcut(BusinessLayer::TextParagraphType _type,
                                        const QString& _shortcut);
    //
    static QString novelOutlineJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setNovelOutlineJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                         BusinessLayer::TextParagraphType _toType);
    //
    static QString novelOutlineJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setNovelOutlineJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                           BusinessLayer::TextParagraphType _toType);
    //
    static QString novelOutlineChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setNovelOutlineChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                           BusinessLayer::TextParagraphType _toType);
    //
    static QString novelOutlineChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setNovelOutlineChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                             BusinessLayer::TextParagraphType _toType);
    //
    // ... текст
    //
    static QString novelShortcut(BusinessLayer::TextParagraphType _type);
    static void setNovelShortcut(BusinessLayer::TextParagraphType _type, const QString& _shortcut);
    //
    static QString novelJumpByTab(BusinessLayer::TextParagraphType _type);
    static void setNovelJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                  BusinessLayer::TextParagraphType _toType);
    //
    static QString novelJumpByEnter(BusinessLayer::TextParagraphType _type);
    static void setNovelJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                    BusinessLayer::TextParagraphType _toType);
    //
    static QString novelChangeByTab(BusinessLayer::TextParagraphType _type);
    static void setNovelChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                    BusinessLayer::TextParagraphType _toType);
    //
    static QString novelChangeByEnter(BusinessLayer::TextParagraphType _type);
    static void setNovelChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                      BusinessLayer::TextParagraphType _toType);
};
