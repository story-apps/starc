#include "screenplay_dialogues_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <3rd_party/qtxlsxwriter/xlsxrichstring.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStandardItemModel>

#include <set>


namespace BusinessLayer {

class ScreenplayDialoguesReport::Implementation
{
public:
    /**
     * @brief Модель реплик
     */
    QScopedPointer<QStandardItemModel> dialoguesModel;

    /**
     * @brief Список персонажей + видимые
     */
    QVector<QString> characters;
    std::optional<QVector<QString>> visibleCharacters;
};


// ****


ScreenplayDialoguesReport::ScreenplayDialoguesReport()
    : d(new Implementation)
{
}

ScreenplayDialoguesReport::~ScreenplayDialoguesReport() = default;

void ScreenplayDialoguesReport::build(QAbstractItemModel* _model)
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
    int dialogueNumber = 1;
    struct DialogueData {
        int number = 0;
        QString character;
        QString dialogue;
        QString parenthetical;
        QString extension;
    };
    const int invalidPage = 0;
    struct SceneData {
        QString name;
        int page = invalidPage;
        QString number;
        std::chrono::milliseconds duration;
        QVector<DialogueData> dialogues;
    };
    QVector<SceneData> scenes;
    SceneData lastScene;
    QSet<QString> characters;

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
    includeInReport = [&includeInReport, &scenes, &lastScene, textItemPage, &dialogueNumber,
                       &characters, invalidPage](const TextModelItem* _item) {
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

                case TextParagraphType::Character: {
                    if (textItem->isCorrection() || textItem->text().isEmpty()) {
                        break;
                    }

                    DialogueData dialogue;
                    dialogue.number = dialogueNumber++;
                    dialogue.character = ScreenplayCharacterParser::name(textItem->text());
                    dialogue.extension = ScreenplayCharacterParser::extension(textItem->text());
                    lastScene.dialogues.append(dialogue);

                    characters.insert(dialogue.character);
                    break;
                }

                case TextParagraphType::Parenthetical: {
                    if (textItem->isCorrection() || lastScene.dialogues.isEmpty()) {
                        break;
                    }

                    //
                    // Если реплика ещё не была задана, то это ремарка перед репликой
                    //
                    if (lastScene.dialogues.constLast().dialogue.isEmpty()) {
                        lastScene.dialogues.last().parenthetical = textItem->text();
                    }
                    //
                    // А если реплика уже была, то это новая реплика с ремаркой
                    //
                    else {
                        auto dialogue = lastScene.dialogues.constLast();
                        dialogue.parenthetical = textItem->text();
                        dialogue.dialogue.clear();
                        lastScene.dialogues.append(dialogue);
                    }
                    break;
                }

                case TextParagraphType::Dialogue:
                case TextParagraphType::Lyrics: {
                    if (textItem->isCorrection() || lastScene.dialogues.isEmpty()) {
                        break;
                    }

                    //
                    // Если реплика ещё не была задана, то это ремарка перед репликой
                    //
                    if (lastScene.dialogues.constLast().dialogue.isEmpty()) {
                        lastScene.dialogues.last().dialogue = textItem->text();
                    }
                    //
                    // А если реплика уже была, то допишем в конец с пробельчиком
                    //
                    else {
                        lastScene.dialogues.last().dialogue += " " + textItem->text();
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
    // Формируем отчёт
    //
    auto createModelItem = [](const QString& _text, const QVariant& _backgroundColor = {}) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (_backgroundColor.isValid()) {
            item->setData(_backgroundColor, Qt::BackgroundRole);
        }
        return item;
    };
    //
    // ... наполняем таблицу
    //
    if (d->dialoguesModel.isNull()) {
        d->dialoguesModel.reset(new QStandardItemModel);
    } else {
        d->dialoguesModel->clear();
    }
    const auto titleBackgroundColor = QVariant::fromValue(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::elevationEndOpacity()));
    for (const auto& scene : scenes) {
        if (scene.dialogues.isEmpty()) {
            continue;
        }

        auto sceneItem
            = createModelItem(QString("%1 %2").arg(scene.number, scene.name), titleBackgroundColor);
        for (const auto& dialogue : scene.dialogues) {
            if (d->visibleCharacters.has_value()
                && !d->visibleCharacters->contains(dialogue.character)) {
                continue;
            }

            auto dialogueItem = createModelItem(dialogue.character);
            dialogueItem->setData(dialogue.number, Qt::UserRole);
            sceneItem->appendRow({
                dialogueItem,
                createModelItem(dialogue.dialogue),
                createModelItem(dialogue.parenthetical),
                createModelItem(dialogue.extension),
            });
        }

        if (!sceneItem->hasChildren()) {
            delete sceneItem;
            continue;
        }

        d->dialoguesModel->appendRow({
            sceneItem,
            createModelItem({ scene.name }, titleBackgroundColor),
            createModelItem({ scene.number }, titleBackgroundColor),
            createModelItem({}, titleBackgroundColor),
        });
    }
    //
    d->dialoguesModel->setHeaderData(
        0, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Character"),
        Qt::DisplayRole);
    d->dialoguesModel->setHeaderData(
        1, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Dialogue/lyrics"),
        Qt::DisplayRole);
    d->dialoguesModel->setHeaderData(
        2, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Parenthetical"),
        Qt::DisplayRole);
    d->dialoguesModel->setHeaderData(
        3, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Extension"),
        Qt::DisplayRole);

    //
    d->characters = { characters.begin(), characters.end() };
    std::sort(d->characters.begin(), d->characters.end());
}

void ScreenplayDialoguesReport::saveToXlsx(const QString& _fileName) const
{
    QXlsx::Document xlsx;
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);

    constexpr int firstRow = 1;
    int reportRow = firstRow;
    auto writeHeader = [&xlsx, &headerFormat, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text, headerFormat);
    };
    auto writeText = [&xlsx, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text);
    };

    writeHeader(
        1, QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Scene number"));
    writeHeader(
        2,
        QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Scene heading"));
    writeHeader(
        3,
        QCoreApplication::translate("BusinessLayer::ScreenplayDialoguesReport", "Dialogue number"));
    constexpr int firstColumn = 4;
    for (int column = firstColumn; column < firstColumn + dialoguesModel()->columnCount();
         ++column) {
        writeHeader(column, dialoguesModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    ++reportRow;
    for (int row = 0; row < dialoguesModel()->rowCount(); ++row) {
        const auto itemIndex = dialoguesModel()->index(row, 0);
        for (int childRow = 0; childRow < dialoguesModel()->rowCount(itemIndex); ++childRow) {
            writeText(1, itemIndex.siblingAtColumn(2).data());
            writeText(2, itemIndex.siblingAtColumn(1).data());

            writeText(3, dialoguesModel()->index(childRow, 0, itemIndex).data(Qt::UserRole));
            for (int column = firstColumn; column < firstColumn + dialoguesModel()->columnCount();
                 ++column) {
                writeText(
                    column,
                    dialoguesModel()->index(childRow, column - firstColumn, itemIndex).data());
            }
            ++reportRow;
        }
    }

    xlsx.saveAs(_fileName);
}

void ScreenplayDialoguesReport::setParameters(const QVector<QString>& _characters)
{
    d->visibleCharacters = _characters;
}

QAbstractItemModel* ScreenplayDialoguesReport::dialoguesModel() const
{
    return d->dialoguesModel.data();
}

QVector<QString> ScreenplayDialoguesReport::characters() const
{
    return d->characters;
}

} // namespace BusinessLayer
