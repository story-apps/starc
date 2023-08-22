#include "shortcuts_helper.h"

#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>


namespace {
static QString moduleShortcut(const QString& _module, BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/%2").arg(_module, BusinessLayer::toString(_type))).toString();
}
static void setModuleShortcut(const QString& _module, BusinessLayer::TextParagraphType _type,
                              const QString& _shortcut)
{
    setSettingsValue(QString("%1/%2").arg(_module, BusinessLayer::toString(_type)), _shortcut);
}
//
static QString moduleJumpByTab(const QString& _module, BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                             .arg(_module, BusinessLayer::toString(_type)))
        .toString();
}
static void setModuleJumpByTab(const QString& _module, BusinessLayer::TextParagraphType _fromType,
                               BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-tab")
                         .arg(_module, BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}
//
static QString moduleJumpByEnter(const QString& _module, BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                             .arg(_module, BusinessLayer::toString(_type)))
        .toString();
}
static void setModuleJumpByEnter(const QString& _module, BusinessLayer::TextParagraphType _fromType,
                                 BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-jumping/from-%2-by-enter")
                         .arg(_module, BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}
//
static QString moduleChangeByTab(const QString& _module, BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-tab")
                             .arg(_module, BusinessLayer::toString(_type)))
        .toString();
}
static void setModuleChangeByTab(const QString& _module, BusinessLayer::TextParagraphType _fromType,
                                 BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-tab")
                         .arg(_module, BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}
//
static QString moduleChangeByEnter(const QString& _module, BusinessLayer::TextParagraphType _type)
{
    return settingsValue(QString("%1/styles-changing/from-%2-by-enter")
                             .arg(_module, BusinessLayer::toString(_type)))
        .toString();
}
static void setModuleChangeByEnter(const QString& _module,
                                   BusinessLayer::TextParagraphType _fromType,
                                   BusinessLayer::TextParagraphType _toType)
{
    setSettingsValue(QString("%1/styles-changing/from-%2-by-enter")
                         .arg(_module, BusinessLayer::toString(_fromType)),
                     BusinessLayer::toString(_toType));
}
} // namespace


QString ShortcutsHelper::simpleTextShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsSimpleTextEditorShortcutsKey, _type);
}

void ShortcutsHelper::setSimpleTextShortcut(BusinessLayer::TextParagraphType _type,
                                            const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsSimpleTextEditorShortcutsKey, _type, _shortcut);
}

QString ShortcutsHelper::simpleTextJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsSimpleTextEditorKey, _type);
}

void ShortcutsHelper::setSimpleTextJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                             BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsSimpleTextEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::simpleTextJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsSimpleTextEditorKey, _type);
}

void ShortcutsHelper::setSimpleTextJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsSimpleTextEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::simpleTextChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsSimpleTextEditorKey, _type);
}

void ShortcutsHelper::setSimpleTextChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsSimpleTextEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::simpleTextChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsSimpleTextEditorKey, _type);
}

void ShortcutsHelper::setSimpleTextChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsSimpleTextEditorKey, _fromType, _toType);
}

//

QString ShortcutsHelper::screenplayTreatmentShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsScreenplayTreatmentEditorShortcutsKey,
                          _type);
}

void ShortcutsHelper::setScreenplayTreatmentShortcut(BusinessLayer::TextParagraphType _type,
                                                     const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsScreenplayTreatmentEditorShortcutsKey, _type,
                      _shortcut);
}

QString ShortcutsHelper::screenplayTreatmentJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _type);
}

void ShortcutsHelper::setScreenplayTreatmentJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                                      BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _fromType,
                       _toType);
}

QString ShortcutsHelper::screenplayTreatmentJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _type);
}

void ShortcutsHelper::setScreenplayTreatmentJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                                        BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _fromType,
                         _toType);
}

QString ShortcutsHelper::screenplayTreatmentChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _type);
}

void ShortcutsHelper::setScreenplayTreatmentChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                                        BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _fromType,
                         _toType);
}

QString ShortcutsHelper::screenplayTreatmentChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _type);
}

void ShortcutsHelper::setScreenplayTreatmentChangeByEnter(
    BusinessLayer::TextParagraphType _fromType, BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsScreenplayTreatmentEditorKey, _fromType,
                           _toType);
}

//

QString ShortcutsHelper::screenplayShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsScreenplayEditorShortcutsKey, _type);
}

void ShortcutsHelper::setScreenplayShortcut(BusinessLayer::TextParagraphType _type,
                                            const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsScreenplayEditorShortcutsKey, _type, _shortcut);
}

QString ShortcutsHelper::screenplayJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsScreenplayEditorKey, _type);
}

void ShortcutsHelper::setScreenplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                             BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsScreenplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::screenplayJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsScreenplayEditorKey, _type);
}

void ShortcutsHelper::setScreenplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsScreenplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::screenplayChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsScreenplayEditorKey, _type);
}

void ShortcutsHelper::setScreenplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsScreenplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::screenplayChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsScreenplayEditorKey, _type);
}

void ShortcutsHelper::setScreenplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsScreenplayEditorKey, _fromType, _toType);
}

//

QString ShortcutsHelper::comicBookShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsComicBookEditorShortcutsKey, _type);
}

void ShortcutsHelper::setComicBookShortcut(BusinessLayer::TextParagraphType _type,
                                           const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsComicBookEditorShortcutsKey, _type, _shortcut);
}

QString ShortcutsHelper::comicBookJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsComicBookEditorKey, _type);
}

void ShortcutsHelper::setComicBookJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                            BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsComicBookEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::comicBookJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsComicBookEditorKey, _type);
}

void ShortcutsHelper::setComicBookJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                              BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsComicBookEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::comicBookChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsComicBookEditorKey, _type);
}

void ShortcutsHelper::setComicBookChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                              BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsComicBookEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::comicBookChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsComicBookEditorKey, _type);
}

void ShortcutsHelper::setComicBookChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsComicBookEditorKey, _fromType, _toType);
}

//

QString ShortcutsHelper::audioplayShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsAudioplayEditorShortcutsKey, _type);
}

void ShortcutsHelper::setAudioplayShortcut(BusinessLayer::TextParagraphType _type,
                                           const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsAudioplayEditorShortcutsKey, _type, _shortcut);
}

QString ShortcutsHelper::audioplayJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsAudioplayEditorKey, _type);
}

void ShortcutsHelper::setAudioplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                            BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsAudioplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::audioplayJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsAudioplayEditorKey, _type);
}

void ShortcutsHelper::setAudioplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                              BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsAudioplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::audioplayChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsAudioplayEditorKey, _type);
}

void ShortcutsHelper::setAudioplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                              BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsAudioplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::audioplayChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsAudioplayEditorKey, _type);
}

void ShortcutsHelper::setAudioplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsAudioplayEditorKey, _fromType, _toType);
}

//

QString ShortcutsHelper::stageplayShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsStageplayEditorShortcutsKey, _type);
}

void ShortcutsHelper::setStageplayShortcut(BusinessLayer::TextParagraphType _type,
                                           const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsStageplayEditorShortcutsKey, _type, _shortcut);
}

QString ShortcutsHelper::stageplayJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsStageplayEditorKey, _type);
}

void ShortcutsHelper::setStageplayJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                            BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsStageplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::stageplayJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsStageplayEditorKey, _type);
}

void ShortcutsHelper::setStageplayJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                              BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsStageplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::stageplayChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsStageplayEditorKey, _type);
}

void ShortcutsHelper::setStageplayChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                              BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsStageplayEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::stageplayChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsStageplayEditorKey, _type);
}

void ShortcutsHelper::setStageplayChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsStageplayEditorKey, _fromType, _toType);
}

//

QString ShortcutsHelper::novelOutlineShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsNovelOutlineEditorShortcutsKey, _type);
}

void ShortcutsHelper::setNovelOutlineShortcut(BusinessLayer::TextParagraphType _type,
                                              const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsNovelOutlineEditorShortcutsKey, _type,
                      _shortcut);
}

QString ShortcutsHelper::novelOutlineJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsNovelOutlineEditorKey, _type);
}

void ShortcutsHelper::setNovelOutlineJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                               BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsNovelOutlineEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::novelOutlineJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsNovelOutlineEditorKey, _type);
}

void ShortcutsHelper::setNovelOutlineJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsNovelOutlineEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::novelOutlineChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsNovelOutlineEditorKey, _type);
}

void ShortcutsHelper::setNovelOutlineChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                                 BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsNovelOutlineEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::novelOutlineChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsNovelOutlineEditorKey, _type);
}

void ShortcutsHelper::setNovelOutlineChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                                   BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsNovelOutlineEditorKey, _fromType, _toType);
}

//

QString ShortcutsHelper::novelShortcut(BusinessLayer::TextParagraphType _type)
{
    return moduleShortcut(DataStorageLayer::kComponentsNovelEditorShortcutsKey, _type);
}

void ShortcutsHelper::setNovelShortcut(BusinessLayer::TextParagraphType _type,
                                       const QString& _shortcut)
{
    setModuleShortcut(DataStorageLayer::kComponentsNovelEditorShortcutsKey, _type, _shortcut);
}

QString ShortcutsHelper::novelJumpByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByTab(DataStorageLayer::kComponentsNovelEditorKey, _type);
}

void ShortcutsHelper::setNovelJumpByTab(BusinessLayer::TextParagraphType _fromType,
                                        BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByTab(DataStorageLayer::kComponentsNovelEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::novelJumpByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleJumpByEnter(DataStorageLayer::kComponentsNovelEditorKey, _type);
}

void ShortcutsHelper::setNovelJumpByEnter(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType)
{
    setModuleJumpByEnter(DataStorageLayer::kComponentsNovelEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::novelChangeByTab(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByTab(DataStorageLayer::kComponentsNovelEditorKey, _type);
}

void ShortcutsHelper::setNovelChangeByTab(BusinessLayer::TextParagraphType _fromType,
                                          BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByTab(DataStorageLayer::kComponentsNovelEditorKey, _fromType, _toType);
}

QString ShortcutsHelper::novelChangeByEnter(BusinessLayer::TextParagraphType _type)
{
    return moduleChangeByEnter(DataStorageLayer::kComponentsNovelEditorKey, _type);
}

void ShortcutsHelper::setNovelChangeByEnter(BusinessLayer::TextParagraphType _fromType,
                                            BusinessLayer::TextParagraphType _toType)
{
    setModuleChangeByEnter(DataStorageLayer::kComponentsNovelEditorKey, _fromType, _toType);
}
