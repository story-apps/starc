#include "audioplay_structure_analysis_plot.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/audioplay/text/audioplay_text_document.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_block_parser.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_scene_item.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QCoreApplication>
#include <QRegularExpression>


namespace BusinessLayer {


class AudioplayStructureAnalysisPlot::Implementation
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


AudioplayStructureAnalysisPlot::AudioplayStructureAnalysisPlot()
    : d(new Implementation)
{
}

AudioplayStructureAnalysisPlot::~AudioplayStructureAnalysisPlot() = default;

void AudioplayStructureAnalysisPlot::build(QAbstractItemModel* _model) const
{
    if (_model == nullptr) {
        return;
    }

    auto audioplayModel = qobject_cast<AudioplayTextModel*>(_model);
    if (audioplayModel == nullptr) {
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
    auto charactersModel = audioplayModel->charactersList();
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
    const auto& audioplayTemplate
        = TemplatesFacade::audioplayTemplate(audioplayModel->informationModel()->templateId());
    PageTextEdit audioplayTextEdit;
    audioplayTextEdit.setUsePageMode(true);
    audioplayTextEdit.setPageSpacing(0);
    audioplayTextEdit.setPageFormat(audioplayTemplate.pageSizeId());
    audioplayTextEdit.setPageMarginsMm(audioplayTemplate.pageMargins());
    AudioplayTextDocument audioplayDocument;
    audioplayTextEdit.setDocument(&audioplayDocument);
    const bool kCanChangeModel = false;
    audioplayDocument.setModel(audioplayModel, kCanChangeModel);
    QTextCursor audioplayCursor(&audioplayDocument);
    auto textItemPage = [&audioplayTextEdit, &audioplayDocument,
                         &audioplayCursor](TextModelTextItem* _item) {
        audioplayCursor.setPosition(
            audioplayDocument.itemPosition(audioplayDocument.model()->indexForItem(_item), true));
        return audioplayTextEdit.cursorPage(audioplayCursor);
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
                auto textItem = static_cast<AudioplayTextModelTextItem*>(childItem);
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
                        = static_cast<AudioplayTextModelSceneItem*>(textItem->parent());
                    lastScene.name = sceneItem->heading();
                    lastScene.number = sceneItem->number()->text;
                    lastScene.duration = sceneItem->duration();
                    lastScene.page = textItemPage(textItem);
                    break;
                }

                case TextParagraphType::Character: {
                    lastScene.dialoguesDuration += textItem->duration();
                    lastScene.characters.insert(AudioplayCharacterParser::name(textItem->text()));
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
    includeInReport(audioplayModel->itemForIndex({}));
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
                QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                            "Scene duration"),
                TimeHelper::toString(scene.duration)));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                            "Action duration"),
                TimeHelper::toString(scene.actionDuration)));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                            "Dialogues duration"),
                TimeHelper::toString(scene.dialoguesDuration)));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                            "Characters count"),
                QString::number(scene.characters.size())));
            infoText.append("\n");
            infoText.append(QString("%1: %2").arg(
                QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
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
        data.name = QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
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
        data.name = QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
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
        data.name = QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
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
        data.name = QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
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
        data.name = QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                                "Dialogues count");
        data.color = ColorHelper::forNumber(plotIndex);
        data.x = x;
        data.y = dialogsCountY;
        d->plot.data.append(data);
    }
}

Plot AudioplayStructureAnalysisPlot::plot() const
{
    return d->plot;
}

void AudioplayStructureAnalysisPlot::saveToFile(const QString& _fileName) const
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
        QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot", "Scene"));
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
                const auto isDuration = QSet<QString>{QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                                                                  "Scene duration"),
                                        QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
                                                                                  "Action duration"),
                                        QCoreApplication::translate("BusinessLayer::AudioplayStructureAnalysisPlot",
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

            firstColumnWidth
                = std::max(firstColumnWidth, static_cast<int>(sceneInfo->constFirst().size()));
        }
    }

    xlsx.setColumnWidth(firstColumn, firstColumnWidth);

    xlsx.saveAs(_fileName);
}

void AudioplayStructureAnalysisPlot::setParameters(bool _sceneDuration, bool _actionDuration,
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
