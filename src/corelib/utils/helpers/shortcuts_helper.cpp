#include "shortcuts_helper.h"

#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>


QString ShortcutsHelper::screenplayTreatmentShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/%2").arg(
                             DataStorageLayer::kComponentsScreenplayTreatmentEditorShortcutsKey,
                             BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayTreatmentShortcut(BusinessLayer::TextParagraphType _type,
                                                     const QString& _shortcut)
{
    setSettingsValue(
        QString("%1/%2").arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorShortcutsKey,
                             BusinessLayer::toString(_type)),
        _shortcut);
}

QString ShortcutsHelper::screenplayTreatmentJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayTreatmentJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                                      BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayTreatmentJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayTreatmentJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                                        BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayTreatmentChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayTreatmentChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                                        BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayTreatmentChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayTreatmentChangeByEnter(
    BusinessLayer::TextParagraphType _fromType, BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

//

QString ShortcutsHelper::screenplayShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(
               QString("%1/%2").arg(DataStorageLayer::kComponentsScreenplayEditorShortcutsKey,
                                    BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayShortcut(BusinessLayer::TextParagraphType _type,
                                            const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsScreenplayEditorShortcutsKey,
                                          BusinessLayer::toString(_type)),
                     _shortcut);
}

QString ShortcutsHelper::screenplayJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                             BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

//

QString ShortcutsHelper::audioplayShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(
               QString("%1/%2").arg(DataStorageLayer::kComponentsAudioplayEditorShortcutsKey,
                                    BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setAudioplayShortcut(BusinessLayer::TextParagraphType _type,
                                           const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsAudioplayEditorShortcutsKey,
                                          BusinessLayer::toString(_type)),
                     _shortcut);
}

//

QString ShortcutsHelper::comicBookShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(
               QString("%1/%2").arg(DataStorageLayer::kComponentsComicBookEditorShortcutsKey,
                                    BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setComicBookShortcut(BusinessLayer::TextParagraphType _type,
                                           const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsComicBookEditorShortcutsKey,
                                          BusinessLayer::toString(_type)),
                     _shortcut);
}

//

QString ShortcutsHelper::stageplayShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(
               QString("%1/%2").arg(DataStorageLayer::kComponentsStageplayEditorShortcutsKey,
                                    BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setStageplayShortcut(BusinessLayer::TextParagraphType _type,
                                           const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsStageplayEditorShortcutsKey,
                                          BusinessLayer::toString(_type)),
                     _shortcut);
}

//

QString ShortcutsHelper::novelOutlineShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(
               QString("%1/%2").arg(DataStorageLayer::kComponentsNovelOutlineEditorShortcutsKey,
                                    BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelOutlineShortcut(BusinessLayer::TextParagraphType _type,
                                              const QString& _shortcut)
{
    setSettingsValue(
        QString("%1/%2").arg(DataStorageLayer::kComponentsNovelOutlineEditorShortcutsKey,
                             BusinessLayer::toString(_type)),
        _shortcut);
}

QString ShortcutsHelper::novelOutlineJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelOutlineJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::novelOutlineJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelOutlineJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::novelOutlineChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelOutlineChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::novelOutlineChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelOutlineChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                   BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

//

QString ShortcutsHelper::novelShortcut(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsNovelEditorShortcutsKey,
                                              BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelShortcut(BusinessLayer::TextParagraphType _type,
                                       const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsNovelEditorShortcutsKey,
                                          BusinessLayer::toString(_type)),
                     _shortcut);
}

QString ShortcutsHelper::novelJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsNovelEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(
        QString("%1/styles-jumping/from-%2-by-tab")
            .arg(DataStorageLayer::kComponentsNovelEditorKey, BusinessLayer::toString(_fromType)),
        BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::novelJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsNovelEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(
        QString("%1/styles-jumping/from-%2-by-enter")
            .arg(DataStorageLayer::kComponentsNovelEditorKey, BusinessLayer::toString(_fromType)),
        BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::novelChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsNovelEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(
        QString("%1/styles-changing/from-%2-by-tab")
            .arg(DataStorageLayer::kComponentsNovelEditorKey, BusinessLayer::toString(_fromType)),
        BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::novelChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsNovelEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setNovelChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                            BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(
        QString("%1/styles-changing/from-%2-by-enter")
            .arg(DataStorageLayer::kComponentsNovelEditorKey, BusinessLayer::toString(_fromType)),
        BusinessLayer::toString(_toType));
}
