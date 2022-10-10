#include "screenplay_characters_activity_plot.h"

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


class ScreenplayCharactersActivityPlot::Implementation
{
public:
    Plot plot;
    QVector<QString> characters;
    std::optional<QVector<QString>> visibleCharacters;
};


// ****


ScreenplayCharactersActivityPlot::ScreenplayCharactersActivityPlot()
    : d(new Implementation)
{
}

ScreenplayCharactersActivityPlot::~ScreenplayCharactersActivityPlot() = default;

void ScreenplayCharactersActivityPlot::build(QAbstractItemModel* _model) const
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
        QSet<QString> characters;
    };
    QVector<SceneData> scenes;
    SceneData lastScene;
    QVector<QString> characters;

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    QString rxPattern;
    auto charactersModel = screenplayModel->charactersModel();
    for (int index = 0; index < charactersModel->rowCount(); ++index) {
        auto characterName = charactersModel->index(index, 0).data().toString();
        if (!rxPattern.isEmpty()) {
            rxPattern.append("|");
        }
        rxPattern.append(characterName);
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
    includeInReport = [&includeInReport, &scenes, &lastScene, &characters, &rxCharacterFinder,
                       textItemPage, invalidPage](const TextModelItem* _item) {
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
                        if (!characters.contains(character)) {
                            characters.append(character);
                        }
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    const auto character = ScreenplayCharacterParser::name(textItem->text());
                    lastScene.characters.insert(character);
                    if (!characters.contains(character)) {
                        characters.append(character);
                    }
                    break;
                }

                case TextParagraphType::Action: {
                    if (rxCharacterFinder.pattern().isEmpty()) {
                        break;
                    }

                    auto match = rxCharacterFinder.match(textItem->text());
                    while (match.hasMatch()) {
                        const auto character = TextHelper::smartToUpper(match.captured(2));
                        lastScene.characters.insert(character);
                        if (!characters.contains(character)) {
                            characters.append(character);
                        }

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
    QList<QVector<qreal>> charactersY;
    for (int characterIndex = 0; characterIndex < characters.size(); ++characterIndex) {
        charactersY.append({ std::numeric_limits<qreal>::quiet_NaN() });
    }
    //
    const auto millisecondsInMinute = 60000.0;
    auto lastX = 0.0;
    QMap<qreal, QStringList> info;
    info.insert(lastX, {});
    for (int sceneIndex = 0; sceneIndex < scenes.size(); ++sceneIndex) {
        const auto& scene = scenes.at(sceneIndex);
        //
        // Информация
        //
        QString infoTitle = QString("%1 %2").arg(scene.number, scene.name);
        QString infoText;
        for (const auto& character : scene.characters) {
            if (!infoText.isEmpty()) {
                infoText.append("\n");
            }
            infoText.append(character);
        }
        info.insert(lastX, { infoTitle, infoText });

        //
        // По иксу откладываем длительность
        //
        x << lastX + 1000 / millisecondsInMinute;
        x << lastX + scene.duration.count() / millisecondsInMinute;
        lastX = x.last();
        //
        // По игрику активность персонажей
        //
        // Позицию по игрику наращиваем, потому что мы будем строить графики один над другим
        //
        int lastY = 0;
        for (int characterIndex = 0; characterIndex < characters.size(); ++characterIndex) {
            const auto characterName = characters.at(characterIndex);
            if (d->visibleCharacters.has_value()
                && !d->visibleCharacters->contains(characterName)) {
                continue;
            }

            auto currentY = std::numeric_limits<qreal>::quiet_NaN();
            if (scene.characters.contains(characterName)) {
                currentY = lastY;
            }
            charactersY[characterIndex] << currentY << currentY;

            lastY += 1;
        }
    }
    info.insert(lastX, {});
    //
    d->plot = {};
    d->plot.info = info;
    //
    // ... Формируем список графиков снизу вверх, чтоби они не закрашивались при выводе
    //
    int plotColorIndex = 0;
    for (int characterIndex = characters.size() - 1; characterIndex >= 0; --characterIndex) {
        const auto characterName = characters.at(characterIndex);
        if (d->visibleCharacters.has_value() && !d->visibleCharacters->contains(characterName)) {
            continue;
        }

        PlotData data;
        data.name = characters.at(characterIndex);
        data.color = ColorHelper::forNumber(plotColorIndex++);
        data.x = x;
        data.y = charactersY.at(characterIndex);
        d->plot.data.append(data);
    }
    //
    d->characters = characters;
}

Plot ScreenplayCharactersActivityPlot::plot() const
{
    return d->plot;
}

QVector<QString> ScreenplayCharactersActivityPlot::characters() const
{
    return d->characters;
}

void ScreenplayCharactersActivityPlot::setParameters(const QVector<QString>& _characters)
{
    d->visibleCharacters = _characters;
}

} // namespace BusinessLayer
