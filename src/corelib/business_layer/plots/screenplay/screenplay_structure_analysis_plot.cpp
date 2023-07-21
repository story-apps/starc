#include "screenplay_structure_analysis_plot.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QCoreApplication>
#include <QRegularExpression>


namespace BusinessLayer {


class ScreenplayStructureAnalysisPlot::Implementation
{
public:
    Plot plot;

    bool sceneDuration = true;
    bool actionDuration = true;
    bool dialoguesDuration = true;
    bool charactersCount = true;
    bool dialoguesCount = true;
};


// ****


ScreenplayStructureAnalysisPlot::ScreenplayStructureAnalysisPlot()
    : d(new Implementation)
{
}

ScreenplayStructureAnalysisPlot::~ScreenplayStructureAnalysisPlot() = default;

void ScreenplayStructureAnalysisPlot::build(QAbstractItemModel* _model) const
{
    if (_model == nullptr) {
        return;
    }

    auto screenplayModel = qobject_cast<ScreenplayTextModel*>(_model);
    if (screenplayModel == nullptr) {
        return;
    }

    //
    // Подготовим необходимые структуры для сбора статистики
    //
    const int invalidPage = 0;
    struct SceneData {
        QString name;
        int page = invalidPage;
        QString number;
        std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
        std::chrono::milliseconds actionDuration = std::chrono::milliseconds{ 0 };
        std::chrono::milliseconds dialoguesDuration = std::chrono::milliseconds{ 0 };
        QSet<QString> characters;
        int dialoguesCount = 0;
    };
    QVector<SceneData> scenes;
    SceneData lastScene;

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    QString rxPattern;
    auto charactersModel = screenplayModel->charactersList();
    for (int index = 0; index < charactersModel->rowCount(); ++index) {
        auto characterName = charactersModel->index(index, 0).data().toString();
        if (!rxPattern.isEmpty()) {
            rxPattern.append("|");
        }
        rxPattern.append(TextHelper::toRxEscaped(characterName));
    }
    if (!rxPattern.isEmpty()) {
        rxPattern.prepend("(^|\\W)(");
        rxPattern.append(")($|\\W)");
    }
    const QRegularExpression rxCharacterFinder(
        rxPattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);

    //
    // Подготовим текстовый документ, для определения страниц сцен
    //
    const auto& screenplayTemplate
        = TemplatesFacade::screenplayTemplate(screenplayModel->informationModel()->templateId());
    PageTextEdit screenplayTextEdit;
    screenplayTextEdit.setUsePageMode(true);
    screenplayTextEdit.setPageSpacing(0);
    screenplayTextEdit.setPageFormat(screenplayTemplate.pageSizeId());
    screenplayTextEdit.setPageMarginsMm(screenplayTemplate.pageMargins());
    ScreenplayTextDocument screenplayDocument;
    screenplayTextEdit.setDocument(&screenplayDocument);
    const bool kCanChangeModel = false;
    screenplayDocument.setModel(screenplayModel, kCanChangeModel);
    QTextCursor screenplayCursor(&screenplayDocument);
    auto textItemPage = [&screenplayTextEdit, &screenplayDocument,
                         &screenplayCursor](TextModelTextItem* _item) {
        screenplayCursor.setPosition(
            screenplayDocument.itemPosition(screenplayDocument.model()->indexForItem(_item), true));
        return screenplayTextEdit.cursorPage(screenplayCursor);
    };

    //
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &scenes, &lastScene, &rxCharacterFinder, textItemPage,
                       invalidPage](const TextModelItem* _item) {
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
                //
                // ... стата по объектам
                //
                switch (textItem->paragraphType()) {
                case TextParagraphType::SceneHeading: {
                    //
                    // Началась новая сцена
                    //
                    if (lastScene.page != invalidPage) {
                        scenes.append(lastScene);
                        lastScene = SceneData();
                    }

                    const auto sceneItem
                        = static_cast<ScreenplayTextModelSceneItem*>(textItem->parent());
                    lastScene.name = sceneItem->heading();
                    lastScene.number = sceneItem->number()->text;
                    lastScene.duration = sceneItem->duration();
                    lastScene.page = textItemPage(textItem);
                    break;
                }

                case TextParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        lastScene.characters.insert(character);
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    lastScene.dialoguesDuration += textItem->duration();
                    lastScene.characters.insert(ScreenplayCharacterParser::name(textItem->text()));
                    ++lastScene.dialoguesCount;
                    break;
                }

                case TextParagraphType::Parenthetical:
                case TextParagraphType::Dialogue:
                case TextParagraphType::Lyrics: {
                    lastScene.dialoguesDuration += textItem->duration();
                    break;
                }

                case TextParagraphType::Action: {
                    lastScene.actionDuration += textItem->duration();

                    if (rxCharacterFinder.pattern().isEmpty()) {
                        break;
                    }

                    auto match = rxCharacterFinder.match(textItem->text());
                    while (match.hasMatch()) {
                        lastScene.characters.insert(TextHelper::smartToUpper(match.captured(2)));

                        //
                        // Ищем дальше
                        //
                        match = rxCharacterFinder.match(textItem->text(), match.capturedEnd());
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
    includeInReport(screenplayModel->itemForIndex({}));
    if (lastScene.page != invalidPage) {
        scenes.append(lastScene);
    }

    //
    // Формируем данные для визуализации
    //
    QVector<qreal> initializedVector = { 0.0 };
    // ... х - общий для всех
    auto x = initializedVector;
    // ... y
    auto sceneChronY = initializedVector;
    auto actionChronY = initializedVector;
    auto dialogsChronY = initializedVector;
    auto charactersCountY = initializedVector;
    auto dialogsCountY = initializedVector;
    //
    const auto millisecondsInMinute = 60000.0;
    auto lastX = 0.0;
    QMap<qreal, QStringList> info;
    info.insert(lastX, {});
    for (const auto& scene : scenes) {
        //
        // Информация
        //
        QString infoTitle = QString("%1 %2").arg(scene.number, scene.name);
        QString infoText;
        {
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                            "Scene duration"),
                TimeHelper::toString(scene.duration)));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                            "Action duration"),
                TimeHelper::toString(scene.actionDuration)));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                            "Dialogues duration"),
                TimeHelper::toString(scene.dialoguesDuration)));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                            "Characters count"),
                QString::number(scene.characters.size())));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                            "Dialogues count"),
                QString::number(scene.dialoguesCount)));
        }
        info.insert(lastX, { infoTitle, infoText });

        //
        // По иксу откладываем длительность
        //
        x << lastX + scene.duration.count() / millisecondsInMinute;
        lastX = x.last();
        //
        // Хронометраж считаем в минутах
        //
        sceneChronY << scene.duration.count() / millisecondsInMinute;
        actionChronY << scene.actionDuration.count() / millisecondsInMinute;
        dialogsChronY << scene.dialoguesDuration.count() / millisecondsInMinute;
        //
        // Количества как есть
        //
        charactersCountY << scene.characters.size();
        dialogsCountY << scene.dialoguesCount;
    }
    info.insert(lastX, {});
    //
    d->plot = {};
    d->plot.info = info;
    //
    // ... хронометраж сцены
    //
    int plotIndex = 0;
    if (d->sceneDuration) {
        PlotData data;
        data.name = QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                "Scene duration");
        data.color = ColorHelper::forNumber(plotIndex);
        data.x = x;
        data.y = sceneChronY;
        d->plot.data.append(data);
    }
    //
    // ... хронометраж действий
    //
    ++plotIndex;
    if (d->actionDuration) {
        PlotData data;
        data.name = QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                "Action duration");
        data.color = ColorHelper::forNumber(plotIndex);
        data.x = x;
        data.y = actionChronY;
        d->plot.data.append(data);
    }
    //
    // ... хронометраж реплик
    //
    ++plotIndex;
    if (d->dialoguesDuration) {
        PlotData data;
        data.name = QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                "Dialogues duration");
        data.color = ColorHelper::forNumber(plotIndex);
        data.x = x;
        data.y = dialogsChronY;
        d->plot.data.append(data);
    }
    //
    // ... количество персонажей
    //
    ++plotIndex;
    if (d->charactersCount) {
        PlotData data;
        data.name = QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                "Characters count");
        data.color = ColorHelper::forNumber(plotIndex);
        data.x = x;
        data.y = charactersCountY;
        d->plot.data.append(data);
    }
    //
    // ... количество реплик
    //
    ++plotIndex;
    if (d->dialoguesCount) {
        PlotData data;
        data.name = QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                "Dialogues count");
        data.color = ColorHelper::forNumber(plotIndex);
        data.x = x;
        data.y = dialogsCountY;
        d->plot.data.append(data);
    }
}

Plot ScreenplayStructureAnalysisPlot::plot() const
{
    return d->plot;
}

void ScreenplayStructureAnalysisPlot::saveToFile(const QString& _fileName) const
{
    QXlsx::Document xlsx;
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);

    constexpr int firstRow = 1;
    constexpr int firstColumn = 1;
    int reportRow = firstRow;
    auto writeHeader = [&xlsx, &headerFormat, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text, headerFormat);
    };
    auto writeText = [&xlsx, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text);
    };

    int column = firstColumn;
    writeHeader(
        column++,
        QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot", "Scene"));
    for (const auto& plotData : std::as_const(d->plot.data)) {
        writeHeader(column++, plotData.name);
    }

    ++reportRow;
    int firstColumnWidth = 10;
    if (!d->plot.data.isEmpty()) {
        const auto scenePoints = d->plot.data.constFirst().x;
        QStringList lastSceneInfo;
        for (int index = 0; index < scenePoints.size(); ++index) {
            const auto& scenePoint = scenePoints.at(index);
            const auto sceneInfo = d->plot.info.lowerBound(scenePoint);
            if (sceneInfo == d->plot.info.end() || sceneInfo.value() == lastSceneInfo
                || sceneInfo.value().isEmpty()) {
                continue;
            }

            lastSceneInfo = sceneInfo.value();
            column = firstColumn;
            writeText(column++, sceneInfo->constFirst());
            for (const auto& plotData : std::as_const(d->plot.data)) {
                const auto isDuration = QSet<QString>{QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                                                  "Scene duration"),
                                        QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                                                  "Action duration"),
                                        QCoreApplication::translate("BusinessLayer::ScreenplayStructureAnalysisPlot",
                                                                                  "Dialogues duration"),}.contains(plotData.name);
                //
                // +1, чтобы не брать начальную нулевую точку графика
                //
                auto value = plotData.y.at(index + 1);
                const auto millisecondsInMinute = 60000.0;
                writeText(column++,
                          isDuration ? TimeHelper::toString(std::chrono::milliseconds{
                              static_cast<int>(value * millisecondsInMinute) })
                                     : QString::number(value));
            }
            ++reportRow;

            firstColumnWidth = std::max(firstColumnWidth, sceneInfo->constFirst().size());
        }
    }

    xlsx.setColumnWidth(firstColumn, firstColumnWidth);

    xlsx.saveAs(_fileName);
}

void ScreenplayStructureAnalysisPlot::setParameters(bool _sceneDuration, bool _actionDuration,
                                                    bool _dialoguesDuration, bool _charactersCount,
                                                    bool _dialoguesCount)
{
    d->sceneDuration = _sceneDuration;
    d->actionDuration = _actionDuration;
    d->dialoguesDuration = _dialoguesDuration;
    d->charactersCount = _charactersCount;
    d->dialoguesCount = _dialoguesCount;
}

} // namespace BusinessLayer
