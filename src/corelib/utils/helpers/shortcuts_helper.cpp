#include "shortcuts_helper.h"

#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>


QString ShortcutsHelper::screenplayShortcut(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(
               QString("%1/%2").arg(DataStorageLayer::kComponentsScreenplayEditorShortcutsKey,
                                    BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayShortcut(BusinessLayer::ScreenplayParagraphType _type,
                                            const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(DataStorageLayer::kComponentsScreenplayEditorShortcutsKey,
                                          BusinessLayer::toString(_type)),
                     _shortcut);
}

QString ShortcutsHelper::screenplayJumpByTab(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayJumpByTab(BusinessLayer::ScreenplayParagraphType _fromType,
                                             BusinessLayer::ScreenplayParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayJumpByEnter(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayJumpByEnter(BusinessLayer::ScreenplayParagraphType _fromType,
                                               BusinessLayer::ScreenplayParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayChangeByTab(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayChangeByTab(BusinessLayer::ScreenplayParagraphType _fromType,
                                               BusinessLayer::ScreenplayParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-tab")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}

QString ShortcutsHelper::screenplayChangeByEnter(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

void ShortcutsHelper::setScreenplayChangeByEnter(BusinessLayer::ScreenplayParagraphType _fromType,
                                                 BusinessLayer::ScreenplayParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-enter")
                         .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                              BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}
