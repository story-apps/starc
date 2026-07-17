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
#include <utils/helpers/eighths_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtMath>


namespace BusinessLayer {

namespace {
/**
 * @brief Проверяющий по списку локаций
 */
struct LocationChecker {
    /**
     * @brief Входит ли сцена в список локаций данной проверки
     */
    bool contains(const ComplianceCheckResultItemScene& _scene) const
    {
        const auto sceneLocation = ScreenplaySceneHeadingParser::location(_scene.heading);
        return locationsFull.contains(sceneLocation)
            || (!locationsStartsWith.isEmpty()
                && sceneLocation.contains(QRegularExpression(locationsStartsWith)));
    }

    QSet<QString> locationsFull;
    QString locationsStartsWith;
};

/**
 * @brief Параметры линии
 */
struct Line {
    /**
     * @brief Валидна ли линия
     */
    bool isValid() const
    {
        return !name.isEmpty();
    }

    QString name;
    LocationChecker locations;
    QSet<QString> characters;
};
} // namespace

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
    // ... и собираем статистику
    //
    QVector<ComplianceCheckResultItemScene> scenes;
    ComplianceCheckResultItemScene lastScene;
    QSet<QString> characters;
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
                    lastScene.eighths = sceneItem->eighths();
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
                        const auto character = ScreenplayCharacterParser::name(textItem->text());
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

    //
    // Вспомогательные вещи
    //
    auto failedStatus = [](const ComplianceRule& _rule) {
        return _rule.isStrict ? ComplianceCheckResultStatus::Failed
                              : ComplianceCheckResultStatus::Warning;
    };
    auto fillLocations = [](const QStringList& _values) {
        LocationChecker result;
        for (const auto& location : _values) {
            if (location.endsWith("*")) {
                if (result.locationsStartsWith.isEmpty()) {
                    result.locationsStartsWith += "^(";
                } else {
                    result.locationsStartsWith += "|";
                }
                result.locationsStartsWith += location.chopped(1);
            } else {
                result.locationsFull.insert(TextHelper::smartToUpper(location.trimmed()));
            }
        }
        if (!result.locationsStartsWith.isEmpty()) {
            result.locationsStartsWith += ")";
            result.locationsStartsWith.replace(".", "[.]");
        }
        return result;
    };
    auto fillLines = [&fillLocations](const ComplianceRule& _rule) {
        QVector<Line> lines;
        for (const auto& lineJsonData :
             QJsonDocument::fromJson(_rule.textValues.constLast().toUtf8()).array()) {
            const auto lineJson = lineJsonData.toObject();
            Line line;
            line.name = lineJson["name"].toString();
            QStringList locations;
            for (const auto& locationJsonData : lineJson["locations"].toArray()) {
                locations.append(locationJsonData.toString());
            }
            line.locations = fillLocations(locations);
            for (const auto& characterJsonData : lineJson["characters"].toArray()) {
                line.characters.insert(characterJsonData.toString());
            }

            lines.append(line);
        }
        return lines;
    };
    auto scenesEighths = [](const QVector<ComplianceCheckResultItemScene>& _scenes) {
        qreal eighths = 0.0;
        for (const auto& scene : _scenes) {
            eighths += scene.eighths;
        }
        return eighths;
    };


    //
    // Далее формируем результаты проверок
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
                result.status = failedStatus(rule);
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
                                  totalDuration - std::chrono::seconds{ rule.maximumValue }));
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
                result.status = failedStatus(rule);
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
                result.status = failedStatus(rule);
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
            result.title
                = tr("Characters (%1) should speak in every scene").arg(rule.textValues.join(", "));

            for (const auto& characterNameDirty : rule.textValues) {
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
                result.status = failedStatus(rule);
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::SceneMaxCharactersCount: {
            result.title = tr("Scenes with more then %n characters", nullptr, rule.maximumValue);

            ComplianceCheckResultItem sceneItem;
            sceneItem.type = ComplianceCheckResultItemType::Scene;
            for (const auto& scene : std::as_const(scenes)) {
                if (scene.characters.size() > rule.maximumValue) {
                    sceneItem.scenes.append(scene);
                }
            }

            if (sceneItem.scenes.isEmpty()) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                result.items.append(sceneItem);
                result.subtitle
                    = tr("%1% scenes out of range (%2 from %3)")
                          .arg(QString::number(sceneItem.scenes.size() * 100 / scenes.size()),
                               QString::number(sceneItem.scenes.size()),
                               QString::number(scenes.size()));
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::PrimaryLocationsPercent: {
            result.title
                = tr("Primary locations (%1) should present in %2% of scenes")
                      .arg(rule.textValues.join(" | "), QString::number(rule.minimumValue));

            const auto primaryLocationsChecker = fillLocations(rule.textValues);

            ComplianceCheckResultItem sceneItem;
            sceneItem.type = ComplianceCheckResultItemType::Scene;
            for (const auto& scene : std::as_const(scenes)) {
                if (!primaryLocationsChecker.contains(scene)) {
                    sceneItem.scenes.append(scene);
                }
            }

            const auto scenesInSecondaryLocationsPercent
                = sceneItem.scenes.size() * 100 / scenes.size();
            const auto scenesInPrimaryLocationsPercent = 100 - scenesInSecondaryLocationsPercent;
            if (scenesInPrimaryLocationsPercent >= rule.minimumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                result.items.append(sceneItem);
                const auto deltaPercent = rule.minimumValue - scenesInPrimaryLocationsPercent;
                const auto deltaScenesAmount = qCeil(deltaPercent * scenes.size() / 100.0);
                result.subtitle = tr("Needed %n more scenes", nullptr, deltaScenesAmount);
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::SecondaryLocationsCount: {
            result.title = tr("Maximum %n additional locations", nullptr, rule.maximumValue);

            const auto primaryLocationsChecker = fillLocations(rule.textValues);

            QMap<QString, ComplianceCheckResultItem> additionalLocations;
            for (const auto& scene : std::as_const(scenes)) {
                if (!primaryLocationsChecker.contains(scene)) {
                    const auto sceneLocation
                        = ScreenplaySceneHeadingParser::location(scene.heading);
                    auto& locationItem = additionalLocations[sceneLocation];
                    if (locationItem.type == ComplianceCheckResultItemType::Undefined) {
                        locationItem.type = ComplianceCheckResultItemType::Location;
                        locationItem.title = sceneLocation;
                    }
                    locationItem.scenes.append(scene);
                }
            }
            for (auto& location : additionalLocations) {
                location.title += " " + tr("(%n scenes)", nullptr, location.scenes.size());
                result.items.append(location);
            }

            if (result.items.size() <= rule.maximumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                result.subtitle
                    = tr("%n additional locations (%1 more)", nullptr, result.items.size())
                          .arg(result.items.size() - rule.maximumValue);
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::SecondaryLocationsSceneCount: {
            result.title
                = tr("Minimum %n scenes for additional location", nullptr, rule.minimumValue);

            const auto primaryLocationsChecker = fillLocations(rule.textValues);

            QMap<QString, ComplianceCheckResultItem> additionalLocations;
            for (const auto& scene : std::as_const(scenes)) {
                if (!primaryLocationsChecker.contains(scene)) {
                    const auto sceneLocation
                        = ScreenplaySceneHeadingParser::location(scene.heading);
                    auto& locationItem = additionalLocations[sceneLocation];
                    if (locationItem.type == ComplianceCheckResultItemType::Undefined) {
                        locationItem.type = ComplianceCheckResultItemType::Location;
                        locationItem.title = sceneLocation;
                    }
                    locationItem.scenes.append(scene);
                }
            }
            for (auto& location : additionalLocations) {
                if (location.scenes.size() >= rule.minimumValue) {
                    continue;
                }

                location.title += " " + tr("(%n scenes)", nullptr, location.scenes.size());
                result.items.append(location);
            }

            if (result.items.isEmpty()) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                result.subtitle = tr("%n additional locations has less scenes then needed", nullptr,
                                     result.items.size());
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::SecondaryLocationsNightScenePercent: {
            result.title
                = tr("Night scenes in additional locations should be less then %1% of scenes")
                      .arg(rule.maximumValue);

            const auto primaryLocationsChecker = fillLocations(rule.textValues);

            ComplianceCheckResultItem sceneItem;
            sceneItem.type = ComplianceCheckResultItemType::Scene;
            for (const auto& scene : std::as_const(scenes)) {
                if (!primaryLocationsChecker.contains(scene)) {
                    const auto sceneTime = ScreenplaySceneHeadingParser::sceneTime(scene.heading);
                    if (sceneTime.contains(QRegularExpression("(NIGHT|НОЧЬ)"))) {
                        sceneItem.scenes.append(scene);
                    }
                }
            }

            const auto nightScenesInSecondaryLocationsPercent
                = sceneItem.scenes.size() * 100 / scenes.size();
            if (nightScenesInSecondaryLocationsPercent <= rule.maximumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                result.items.append(sceneItem);
                const auto deltaPercent
                    = nightScenesInSecondaryLocationsPercent - rule.maximumValue;
                const auto deltaScenesAmount = qCeil(deltaPercent * scenes.size() / 100.0);
                result.subtitle = tr("Needed %n less scenes", nullptr, deltaScenesAmount);
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::TotalPages: {
            result.title
                = tr("Script pages count from %1 to %2")
                      .arg(QString::number(rule.minimumValue), QString::number(rule.maximumValue));
            const auto totalEighths = d->screenplayModel->eighths();
            const int totalPages = qCeil(totalEighths / 8.0);
            if (totalPages >= rule.minimumValue && totalPages <= rule.maximumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                result.subtitle = tr("Page count is %1").arg(totalPages);
                if (totalPages < rule.minimumValue) {
                    result.subtitle += " " + tr("(%1 less)").arg(rule.minimumValue - totalPages);
                } else {
                    result.subtitle += " " + tr("(%1 more)").arg(totalPages - rule.maximumValue);
                }
            }

            break;
        }

        case BusinessLayer::ComplianceRuleType::ScenesDistributionByLocationsAndCharacters: {
            //
            // Считать параметры линий
            //
            const auto lines = fillLines(rule);
            QHash<QString, QVector<ComplianceCheckResultItemScene>> linesToScenes;
            QVector<ComplianceCheckResultItemScene> otherScenes;

            //
            // Определить распределение сцен по линиям
            //
            for (const auto& scene : std::as_const(scenes)) {
                bool sceneDistributed = false;
                //
                // ... если сцена происходит в локации конкретной линии
                //
                for (const auto& line : lines) {
                    if (line.locations.contains(scene)) {
                        linesToScenes[line.name].append(scene);
                        sceneDistributed = true;
                        break;
                    }
                }
                if (sceneDistributed) {
                    continue;
                }

                //
                // ... если в сцене преобладают персонажи конкретной линии
                //
                Line lineWithMostCharacters;
                int lineWithMostCharactersCount = 0;
                for (const auto& line : lines) {
                    int lineCharactersCount = 0;
                    for (const auto& character : line.characters) {
                        if (scene.character(character).isValid()) {
                            ++lineCharactersCount;
                        }
                    }

                    if (lineCharactersCount > lineWithMostCharactersCount) {
                        lineWithMostCharacters = line;
                    }
                }
                if (lineWithMostCharacters.isValid()) {
                    linesToScenes[lineWithMostCharacters.name].append(scene);
                    continue;
                }

                //
                // ... остальное откладываем в список несортированных сцен
                //
                otherScenes.append(scene);
            }

            //
            // Сформировать результат проверки для необходимой линии
            //
            result.title
                = tr("%1 pages amount from %2% to %3% of all pages")
                      .arg(rule.textValues.constFirst(), QString::number(rule.minimumValue),
                           QString::number(rule.maximumValue));

            const auto totalScenesEighths = scenesEighths(scenes);
            const auto targetLineScenesEighths
                = scenesEighths(linesToScenes[rule.textValues.constFirst()]);
            const auto targetLineScenesEighthsPercent
                = targetLineScenesEighths * 100 / totalScenesEighths;
            if (targetLineScenesEighthsPercent >= rule.minimumValue
                && targetLineScenesEighthsPercent <= rule.maximumValue) {
                result.status = ComplianceCheckResultStatus::Passed;
            } else {
                result.status = failedStatus(rule);
                for (const auto& line : lines) {
                    ComplianceCheckResultItem lineItem;
                    lineItem.type = ComplianceCheckResultItemType::Other;
                    lineItem.scenes = linesToScenes[line.name];
                    lineItem.title = line.name + " "
                        + QString("(%1)").arg(
                            EighthsHelper::toStringWithPostfix(scenesEighths(lineItem.scenes)));
                    result.items.append(lineItem);
                }
                if (!otherScenes.isEmpty()) {
                    ComplianceCheckResultItem lineItem;
                    lineItem.type = ComplianceCheckResultItemType::Other;
                    lineItem.scenes = otherScenes;
                    lineItem.title = tr("Other") + " "
                        + QString("(%1)").arg(
                            EighthsHelper::toStringWithPostfix(scenesEighths(otherScenes)));
                    result.items.append(lineItem);
                }
                if (targetLineScenesEighthsPercent < rule.minimumValue) {
                    const auto deltaPercent = rule.minimumValue - targetLineScenesEighthsPercent;
                    const auto deltaScenesEighths = deltaPercent * totalScenesEighths / 100.0;
                    result.subtitle
                        = tr("%1% (needed %2 more)")
                              .arg(QString::number(targetLineScenesEighthsPercent, 'f', 2))
                              .arg(EighthsHelper::toStringWithPostfix(deltaScenesEighths));
                } else {
                    const auto deltaPercent = targetLineScenesEighthsPercent - rule.maximumValue;
                    const auto deltaScenesEighths = deltaPercent * totalScenesEighths / 100.0;
                    result.subtitle
                        = tr("%1% (needed %2 less)")
                              .arg(QString::number(targetLineScenesEighthsPercent, 'f', 2))
                              .arg(EighthsHelper::toStringWithPostfix(deltaScenesEighths));
                }
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
