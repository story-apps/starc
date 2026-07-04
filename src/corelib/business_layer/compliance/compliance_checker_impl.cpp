#include "compliance_checker_impl.h"

#include "compliance_checker.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_synopsis_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/time_helper.h>


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
                result.subtitle = tr("%1 scenes", nullptr, scenesCount).arg(scenesCount);
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
                    = tr("%1% scenes out of range (%2)")
                          .arg(QString::number(sceneItem.scenes.size() * 100 / scenes.count()),
                               QString::number(sceneItem.scenes.size()));
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
