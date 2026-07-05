#include "compliance_checker_impl.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_synopsis_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QRegularExpression>


namespace BusinessLayer {
class ComplianceCheckerImpl::Implementation
{
public:
    Implementation(ComplianceCheckerImpl* _q);


    ComplianceCheckerImpl* q = nullptr;

    QScopedPointer<Domain::DocumentObject> informationDocument;
    BusinessLayer::ScreenplayInformationModel* informationModel;
    QScopedPointer<Domain::DocumentObject> titlePageDocument;
    BusinessLayer::ScreenplayTitlePageModel* titlePageModel;
    QScopedPointer<Domain::DocumentObject> synopsisDocument;
    BusinessLayer::ScreenplaySynopsisModel* synopsisModel;
    QScopedPointer<Domain::DocumentObject> screenplayDocument;
    BusinessLayer::ScreenplayTextModel* screenplayModel;

    QVector<ComplianceRule> rules;
};

ComplianceCheckerImpl::Implementation::Implementation(ComplianceCheckerImpl* _q)
    : q(_q)
    , informationDocument(Domain::ObjectsBuilder::createDocument(
          {}, {}, Domain::DocumentObjectType::Screenplay, {}, {}))
    , informationModel(new BusinessLayer::ScreenplayInformationModel(q))
    , titlePageDocument(Domain::ObjectsBuilder::createDocument(
          {}, {}, Domain::DocumentObjectType::ScreenplayTitlePage, {}, {}))
    , titlePageModel(new BusinessLayer::ScreenplayTitlePageModel(q))
    , synopsisDocument(Domain::ObjectsBuilder::createDocument(
          {}, {}, Domain::DocumentObjectType::ScreenplayTitlePage, {}, {}))
    , synopsisModel(new BusinessLayer::ScreenplaySynopsisModel(q))
    , screenplayDocument(Domain::ObjectsBuilder::createDocument(
          {}, {}, Domain::DocumentObjectType::ScreenplayText, {}, {}))
    , screenplayModel(new BusinessLayer::ScreenplayTextModel(q))
{
    informationModel->blockSignals(true);
    informationModel->setDocument(informationDocument.data());

    titlePageModel->blockSignals(true);
    titlePageModel->setInformationModel(informationModel);
    titlePageModel->setDocument(titlePageDocument.data());

    synopsisModel->blockSignals(true);
    synopsisModel->setInformationModel(informationModel);
    synopsisModel->setDocument(synopsisDocument.data());

    screenplayModel->blockSignals(true);
    screenplayModel->setInformationModel(informationModel);
    screenplayModel->setTitlePageModel(titlePageModel);
    screenplayModel->setSynopsisModel(synopsisModel);
    screenplayModel->setDocument(screenplayDocument.data());
}


// ****


ComplianceCheckerImpl::ComplianceCheckerImpl(QObject* _parent)
    : QObject(_parent)
{
    qRegisterMetaType<QVector<ComplianceCheckResult>>("QVector<RuleResult>");
}

void ComplianceCheckerImpl::init()
{
    d.reset(new Implementation(this));
}

ComplianceCheckerImpl::~ComplianceCheckerImpl() = default;

void ComplianceCheckerImpl::setScreenplay(const QByteArray& _information,
                                          const QByteArray& _screenplay)
{
    d->informationModel->setDocumentContent(_information);
    d->screenplayModel->setDocumentContent(_screenplay);
}

void ComplianceCheckerImpl::setRules(const QVector<ComplianceRule>& _rules)
{
    d->rules = _rules;
}

void ComplianceCheckerImpl::startChecking()
{
    if (d->rules.isEmpty()) {
        return;
    }

    //
    // Подготовим текстовый документ, для определения страниц сцен
    //
    const auto& screenplayTemplate
        = TemplatesFacade::screenplayTemplate(d->screenplayModel->informationModel()->templateId());
    PageTextEdit screenplayTextEdit;
    screenplayTextEdit.setUsePageMode(true);
    screenplayTextEdit.setPageSpacing(0);
    screenplayTextEdit.setPageFormat(screenplayTemplate.pageSizeId());
    screenplayTextEdit.setPageMarginsMm(screenplayTemplate.pageMargins());
    ScreenplayTextDocument screenplayDocument;
    screenplayTextEdit.setDocument(&screenplayDocument);
    screenplayDocument.setModel(d->screenplayModel);
    //

    QVector<ComplianceCheckResult> results;
    for (const auto& rule : std::as_const(d->rules)) {
        ComplianceCheckResult result;
        switch (rule.type) {
        case BusinessLayer::ComplianceRuleType::TotalDuration: {
            result.title
                = tr("Script duration from %1 to %2")
                      .arg(TimeHelper::toString(std::chrono::seconds{ rule.minimumValue }),
                           TimeHelper::toString(std::chrono::seconds{ rule.maximumValue }));
            const auto totalDuration = d->screenplayModel->duration();
            const int totalDurationSecs
                = std::chrono::duration_cast<std::chrono::seconds>(totalDuration).count();
            if (totalDurationSecs >= rule.minimumValue && totalDurationSecs <= rule.maximumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = ComplianceCheckResultStatus::Failed;
                result.subtitle = tr("Duration is %1").arg(TimeHelper::toString(totalDuration));
                if (totalDurationSecs < rule.minimumValue) {
                    result.subtitle += " "
                        + tr("(%1 less)")
                              .arg(TimeHelper::toString(std::chrono::seconds{ rule.minimumValue }
                                                        - totalDuration));
                } else {
                    result.subtitle += " "
                        + tr("(%1 more)")
                              .arg(TimeHelper::toString(
                                  totalDuration - std::chrono::seconds{ rule.minimumValue }));
                }
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::ScenesCount: {
            result.title
                = tr("Scenes count from %1 to %2")
                      .arg(QString::number(rule.minimumValue), QString::number(rule.maximumValue));
            const int scenesCount = d->screenplayModel->scenesCount();
            if (scenesCount >= rule.minimumValue && scenesCount <= rule.maximumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = ComplianceCheckResultStatus::Failed;
                result.subtitle = tr("%n scenes", nullptr, scenesCount);
                if (scenesCount < rule.minimumValue) {
                    result.subtitle += " " + tr("(%1 less)").arg(rule.minimumValue - scenesCount);
                } else {
                    result.subtitle += " " + tr("(%1 more)").arg(scenesCount - rule.maximumValue);
                }
            }
            break;
        }

        case BusinessLayer::ComplianceRuleType::SceneDuration: {
            result.title
                = tr("Scene duration from %1 to %2")
                      .arg(TimeHelper::toString(std::chrono::seconds{ rule.minimumValue }),
                           TimeHelper::toString(std::chrono::seconds{ rule.maximumValue }));
            QVector<ComplianceCheckResultItemScene> scenes;
            ComplianceCheckResultItemScene lastScene;
            std::function<void(const TextModelItem*)> includeInReport;
            includeInReport = [&includeInReport, &scenes, &lastScene](const TextModelItem* _item) {
                for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                    auto childItem = _item->childAt(childIndex);
                    switch (childItem->type()) {
                    case TextModelItemType::Folder:
                    case TextModelItemType::Group: {
                        includeInReport(childItem);
                        break;
                    }

                    case TextModelItemType::Text: {
                        auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                        switch (textItem->paragraphType()) {
                        case TextParagraphType::SceneHeading: {
                            if (!lastScene.heading.isEmpty()) {
                                scenes.append(lastScene);
                                lastScene = ComplianceCheckResultItemScene();
                            }

                            const auto sceneItem
                                = static_cast<ScreenplayTextModelSceneItem*>(textItem->parent());
                            lastScene.uuid = sceneItem->uuid();
                            lastScene.heading = sceneItem->heading();
                            lastScene.number = sceneItem->number()->text;
                            lastScene.duration = sceneItem->duration();
                            break;
                        }

                        default:
                            break;
                        }

                        break;
                    }

                    default:
                        break;
                    }
                }
            };
            includeInReport(d->screenplayModel->itemForIndex({}));
            if (!lastScene.heading.isEmpty()) {
                scenes.append(lastScene);
            }

            ComplianceCheckResultItem sceneItem;
            sceneItem.type = ComplianceCheckResultItemType::Scene;
            for (const auto& scene : std::as_const(scenes)) {
                const int sceneDurationSecs
                    = std::chrono::duration_cast<std::chrono::seconds>(scene.duration).count();
                if (sceneDurationSecs < rule.minimumValue
                    || sceneDurationSecs > rule.maximumValue) {
                    sceneItem.scenes.append(scene);
                }
            }

            if (sceneItem.scenes.isEmpty()) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = ComplianceCheckResultStatus::Warning;
                result.items.append(sceneItem);
                result.subtitle
                    = tr("%1% scenes out of range (%2 from %3)")
                          .arg(QString::number(sceneItem.scenes.size() * 100 / scenes.size()),
                               QString::number(sceneItem.scenes.size()),
                               QString::number(scenes.size()));
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::CharacterShouldSpeakInEveryScene: {
            result.title = tr("Character (%1) should speak in every scene").arg(rule.textValue);
            QVector<ComplianceCheckResultItemScene> scenes;
            ComplianceCheckResultItemScene lastScene;
            QSet<QString> characters;

            //
            // Собираем статистику
            //
            std::function<void(const TextModelItem*)> includeInReport;
            includeInReport = [&includeInReport, &scenes, &lastScene,
                               &characters](const TextModelItem* _item) {
                for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                    auto childItem = _item->childAt(childIndex);
                    switch (childItem->type()) {
                    case TextModelItemType::Folder:
                    case TextModelItemType::Group: {
                        includeInReport(childItem);
                        break;
                    }

                    case TextModelItemType::Text: {
                        auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                        switch (textItem->paragraphType()) {
                        case TextParagraphType::SceneHeading: {
                            if (!lastScene.heading.isEmpty()) {
                                scenes.append(lastScene);
                                lastScene = ComplianceCheckResultItemScene();
                            }

                            const auto sceneItem
                                = static_cast<ScreenplayTextModelSceneItem*>(textItem->parent());
                            lastScene.uuid = sceneItem->uuid();
                            lastScene.heading = sceneItem->heading();
                            lastScene.number = sceneItem->number()->text;
                            lastScene.duration = sceneItem->duration();
                            break;
                        }

                        case TextParagraphType::SceneCharacters: {
                            const auto sceneCharacters
                                = ScreenplaySceneCharactersParser::characters(textItem->text());
                            for (const auto& character : sceneCharacters) {
                                auto& characterData = lastScene.character(character);
                                if (!characters.contains(character)) {
                                    characters.insert(character);
                                    characterData.isFirstAppearance = true;
                                }
                            }
                            break;
                        }

                        case TextParagraphType::Character: {
                            if (!textItem->isCorrection()) {
                                const auto character
                                    = ScreenplayCharacterParser::name(textItem->text());
                                auto& characterData = lastScene.character(character);
                                ++characterData.totalDialogues;
                                if (!characters.contains(character)) {
                                    characters.insert(character);
                                    characterData.isFirstAppearance = true;
                                }
                            }
                            break;
                        }

                        default:
                            break;
                        }

                        break;
                    }

                    default:
                        break;
                    }
                }
            };
            includeInReport(d->screenplayModel->itemForIndex({}));
            if (lastScene.isValid()) {
                scenes.append(lastScene);
            }

            for (const auto& characterNameDirty : rule.textValue.split(',')) {
                const auto characterName = TextHelper::smartToUpper(characterNameDirty.trimmed());

                ComplianceCheckResultItem characterItem;
                characterItem.type = ComplianceCheckResultItemType::Character;
                characterItem.title = characterName;
                for (const auto& scene : std::as_const(scenes)) {
                    if (const auto character = scene.character(characterName);
                        character.isValid()) {
                        if (character.totalDialogues == 0) {
                            characterItem.scenes.append(scene);
                        }
                    }
                }

                if (!characterItem.scenes.isEmpty()) {
                    characterItem.title
                        += " " + tr("(%n scenes)", nullptr, characterItem.scenes.size());
                    result.items.append(characterItem);
                }
            }

            if (result.items.isEmpty()) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = ComplianceCheckResultStatus::Failed;
            }

            break;
        }

        default: {
            Q_ASSERT(false);
            break;
        }
        }

        results.append(result);
    }

    emit checkingFinished(results);
}

} // namespace BusinessLayer
