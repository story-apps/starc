#include "shortcuts_helper.h"

#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>


QString ShortcutsHelper::screenplayShortcut(BusinessLayer::ScreenplayParagraphType _type)
{

    return settingsValue(QString("%1/shortcuts/%2")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

QString ShortcutsHelper::screenplayJumpByTab(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

QString ShortcutsHelper::screenplayJumpByEnter(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

QString ShortcutsHelper::screenplayChangeByTab(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}

QString ShortcutsHelper::screenplayChangeByEnter(BusinessLayer::ScreenplayParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                                  BusinessLayer::toString(_type)))
        .toString();
}
