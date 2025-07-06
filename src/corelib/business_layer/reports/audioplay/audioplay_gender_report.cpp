#include "audioplay_gender_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/audioplay/text/audioplay_text_document.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_block_parser.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QPdfWriter>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardItemModel>

#include <set>

namespace BusinessLayer {

class AudioplayGenderReport::Implementation
{
public:
    /**
     * @brief Модель аудиопостановки
     */
    QPointer<AudioplayTextModel> audioplayModel;

    int bechdelTest = 0;
    int reverseBeckdelTest = 0;

    QScopedPointer<QStandardItemModel> scenesInfoModel;
    QScopedPointer<QStandardItemModel> dialoguesInfoModel;
    QScopedPointer<QStandardItemModel> charactersInfoModel;
};


// ****


AudioplayGenderReport::AudioplayGenderReport()
    : d(new Implementation)
{
}

AudioplayGenderReport::~AudioplayGenderReport() = default;

bool AudioplayGenderReport::isValid() const
{
    return d->charactersInfoModel->rowCount() > 0;
}

void AudioplayGenderReport::build(QAbstractItemModel* _model)
{
    if (_model == nullptr) {
        return;
    }

    d->audioplayModel = qobject_cast<AudioplayTextModel*>(_model);
    if (d->audioplayModel == nullptr) {
        return;
    }

    //
    // Подготовим модели к наполнению
    //
    if (d->scenesInfoModel.isNull()) {
        d->scenesInfoModel.reset(new QStandardItemModel);
    } else {
        d->scenesInfoModel->clear();
    }
    if (d->dialoguesInfoModel.isNull()) {
        d->dialoguesInfoModel.reset(new QStandardItemModel);
    } else {
        d->dialoguesInfoModel->clear();
    }
    if (d->charactersInfoModel.isNull()) {
        d->charactersInfoModel.reset(new QStandardItemModel);
    } else {
        d->charactersInfoModel->clear();
    }

    //
    // Подготовим необходимые структуры для сбора статистики
    //
    int bechdelTest = 0;
    int reverseBechdelTest = 0;
    QSet<QString> male, female, other, undefined;
    struct GenderCounter {
        int male = 0;
        int female = 0;
        int other = 0;
        int undefined = 0;
        bool hasDialogues = false;
    };
    GenderCounter scenes;
    GenderCounter lastScene;
    int totalScenes = 0;
    GenderCounter dialogues;
    QSet<QString> lastSceneCharacters;

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    QString rxPattern;
    auto charactersModel = d->audioplayModel->charactersList();
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
    // Соберём список персонажей
    //
    for (int index = 0; index < d->audioplayModel->charactersList()->rowCount(); ++index) {
        const auto characterName
            = d->audioplayModel->charactersList()->index(index, 0).data().toString();
        if (const auto character = d->audioplayModel->character(characterName)) {
            switch (character->gender()) {
            case 0: {
                male.insert(character->name());
                break;
            }
            case 1: {
                female.insert(character->name());
                break;
            }
            case 2: {
                other.insert(character->name());
                break;
            }
            case 3: {
                undefined.insert(character->name());
                break;
            }
            }
        } else {
            undefined.insert(characterName);
        }
    }

    //
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &bechdelTest, &reverseBechdelTest, &male, &female, &other,
                       &undefined, &scenes, &lastScene, &totalScenes, &dialogues,
                       &lastSceneCharacters, &rxCharacterFinder](const TextModelItem* _item) {
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
                    ++totalScenes;
                    //
                    scenes.male += std::min(1, lastScene.male);
                    scenes.female += std::min(1, lastScene.female);
                    scenes.other += std::min(1, lastScene.other);
                    scenes.undefined += std::min(1, lastScene.undefined);
                    //
                    if (lastScene.male > 1 && lastScene.female == 0 && lastScene.other == 0
                        && lastScene.undefined == 0 && lastScene.hasDialogues) {
                        ++reverseBechdelTest;
                    } else if (lastScene.male == 0 && lastScene.female > 1 && lastScene.other == 0
                               && lastScene.undefined == 0 && lastScene.hasDialogues) {
                        ++bechdelTest;
                    }

                    lastScene = GenderCounter();
                    lastSceneCharacters.clear();
                    break;
                }

                case TextParagraphType::Character: {
                    if (!textItem->isCorrection()) {
                        const auto character = AudioplayCharacterParser::name(textItem->text());
                        if (male.contains(character)) {
                            ++dialogues.male;
                        } else if (female.contains(character)) {
                            ++dialogues.female;
                        } else if (other.contains(character)) {
                            ++dialogues.other;
                        } else {
                            undefined.insert(character);
                            ++dialogues.undefined;
                        }
                        if (!lastSceneCharacters.contains(character)) {
                            if (male.contains(character)) {
                                ++lastScene.male;
                            } else if (female.contains(character)) {
                                ++lastScene.female;
                            } else if (other.contains(character)) {
                                ++lastScene.other;
                            } else {
                                ++lastScene.undefined;
                            }
                            lastSceneCharacters.insert(character);
                        }
                        lastScene.hasDialogues = true;
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
    includeInReport(d->audioplayModel->itemForIndex({}));
    //
    // ... и последняя сцена
    //
    scenes.male += std::min(1, lastScene.male);
    scenes.female += std::min(1, lastScene.female);
    scenes.other += std::min(1, lastScene.other);
    scenes.undefined += std::min(1, lastScene.undefined);
    //
    if (lastScene.male > 1 && lastScene.female == 0 && lastScene.other == 0
        && lastScene.undefined == 0 && lastScene.hasDialogues) {
        ++reverseBechdelTest;
    } else if (lastScene.male == 0 && lastScene.female > 1 && lastScene.other == 0
               && lastScene.undefined == 0 && lastScene.hasDialogues) {
        ++bechdelTest;
    }

    //
    // Прерываем выполнение, если в сценарии нет сцен
    //
    if (male.isEmpty() && female.isEmpty() && other.isEmpty() && undefined.isEmpty()) {
        return;
    }

    //
    // Формируем отчёт
    //
    auto createModelItem = [](const QString& _text) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };
    auto createPercentModelItem = [&createModelItem](qreal _percent) {
        auto item = createModelItem(QString::number(_percent, 'g', 3) + "%");
        item->setData(_percent);
        return item;
    };
    auto makeRow = [createModelItem, createPercentModelItem](int _index, int _count, int _total,
                                                             QStandardItemModel* _model) {
        const QHash<int, QString> genders = {
            { 0, QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Male") },
            { 1, QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Female") },
            { 2, QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Other") },
            { 3, QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Undefined") },
        };
        auto genderName = createModelItem(genders.value(_index));
        genderName->setData(u8"\U000F0766", Qt::DecorationRole);
        genderName->setData(ColorHelper::forNumber(_index++), Qt::DecorationPropertyRole);

        _model->appendRow({ genderName, createModelItem(QString::number(_count)),
                            createPercentModelItem(_total > 0 ? (_count * 100.0 / _total) : 0) });
    };
    auto makeHeader = [](QStandardItemModel* _model) {
        _model->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Gender"),
            Qt::DisplayRole);
        _model->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Occurrences"),
            Qt::DisplayRole);
        _model->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Percents"),
            Qt::DisplayRole);
    };
    //
    // ... сводка
    //
    {
        d->bechdelTest = bechdelTest;
        d->reverseBeckdelTest = reverseBechdelTest;
    }
    //
    // ... сцены
    //
    {
        makeRow(0, scenes.male, totalScenes, d->scenesInfoModel.data());
        makeRow(1, scenes.female, totalScenes, d->scenesInfoModel.data());
        makeRow(2, scenes.other, totalScenes, d->scenesInfoModel.data());
        makeRow(3, scenes.undefined, totalScenes, d->scenesInfoModel.data());
        makeHeader(d->scenesInfoModel.data());
    }

    //
    // ... диалоги
    //
    {
        const auto totalDialogues
            = dialogues.male + dialogues.female + dialogues.other + dialogues.undefined;
        makeRow(0, dialogues.male, totalDialogues, d->dialoguesInfoModel.data());
        makeRow(1, dialogues.female, totalDialogues, d->dialoguesInfoModel.data());
        makeRow(2, dialogues.other, totalDialogues, d->dialoguesInfoModel.data());
        makeRow(3, dialogues.undefined, totalDialogues, d->dialoguesInfoModel.data());
        makeHeader(d->dialoguesInfoModel.data());
    }
    //
    // ... персонажи
    //
    {
        const auto totalCharacters = male.size() + female.size() + other.size() + undefined.size();
        makeRow(0, male.size(), totalCharacters, d->charactersInfoModel.data());
        makeRow(1, female.size(), totalCharacters, d->charactersInfoModel.data());
        makeRow(2, other.size(), totalCharacters, d->charactersInfoModel.data());
        makeRow(3, undefined.size(), totalCharacters, d->charactersInfoModel.data());
        makeHeader(d->charactersInfoModel.data());
    }
}

int AudioplayGenderReport::bechdelTest() const
{
    return d->bechdelTest;
}

int AudioplayGenderReport::reverseBechdelTest() const
{
    return d->reverseBeckdelTest;
}

QAbstractItemModel* AudioplayGenderReport::scenesInfoModel() const
{
    return d->scenesInfoModel.data();
}

QAbstractItemModel* AudioplayGenderReport::dialoguesInfoModel() const
{
    return d->dialoguesInfoModel.data();
}

QAbstractItemModel* AudioplayGenderReport::charactersInfoModel() const
{
    return d->charactersInfoModel.data();
}

void AudioplayGenderReport::saveToPdf(const QString& _fileName) const
{
    const auto& exportTemplate
        = TemplatesFacade::audioplayTemplate(d->audioplayModel->informationModel()->templateId());

    //
    // Настраиваем документ
    //
    PageTextEdit textEdit;
    textEdit.setUsePageMode(true);
    textEdit.setPageSpacing(0);
    QTextDocument report;
    report.setDefaultFont(exportTemplate.baseFont());
    textEdit.setDocument(&report);
    //
    // ... параметры страницы
    //
    textEdit.setPageFormat(exportTemplate.pageSizeId());
    textEdit.setPageMarginsMm(exportTemplate.pageMargins());
    textEdit.setPageNumbersAlignment(exportTemplate.pageNumbersAlignment());

    //
    // Формируем отчёт
    //
    QTextCursor cursor(&report);
    cursor.beginEditBlock();
    QTextCharFormat titleFormat;
    auto titleFont = report.defaultFont();
    titleFont.setBold(true);
    titleFormat.setFont(titleFont);
    cursor.setCharFormat(titleFormat);
    cursor.insertText(
        QString("%1 - %2").arg(d->audioplayModel->informationModel()->name(),
                               QCoreApplication::translate("BusinessLayer::AudioplayCastReport",
                                                           "Gender analysis report")));
    cursor.insertBlock();
    cursor.insertBlock();

    auto testResult = [](int _passedTimes) {
        return _passedTimes == 0
            ? QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Does not pass")
            : QCoreApplication::translate("BusinessLayer::AudioplayGenderReport",
                                          "Passed %n time(s)", "", _passedTimes);
    };
    cursor.insertText(
        QCoreApplication::translate("BusinessLayer::AudioplayCastReport", "Bechdel test") + ": "
        + testResult(bechdelTest()));
    cursor.insertBlock();
    cursor.insertText(
        QCoreApplication::translate("BusinessLayer::AudioplayCastReport", "Reverse Bechdel test")
        + ": " + testResult(reverseBechdelTest()));
    cursor.insertBlock();
    cursor.insertBlock();

    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    tableFormat.setColumnWidthConstraints({
        QTextLength{ QTextLength::PercentageLength, 68 },
        QTextLength{ QTextLength::PercentageLength, 16 },
        QTextLength{ QTextLength::PercentageLength, 16 },
    });
    auto beforeTablePosition = cursor.position();
    cursor.insertTable(charactersInfoModel()->rowCount() + 1, charactersInfoModel()->columnCount(),
                       tableFormat);
    cursor.setPosition(beforeTablePosition);
    cursor.movePosition(QTextCursor::NextBlock);
    //
    for (int column = 0; column < charactersInfoModel()->columnCount(); ++column) {
        QTextTableCellFormat cellFormat;
        cellFormat.setBottomBorder(1);
        cellFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        cellFormat.setBottomBorderBrush(Qt::black);
        cursor.mergeBlockCharFormat(cellFormat);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
        cursor.setBlockFormat(blockFormat);
        cursor.insertText(charactersInfoModel()->headerData(column, Qt::Horizontal).toString());

        cursor.movePosition(QTextCursor::NextBlock);
    }
    for (int row = 0; row < charactersInfoModel()->rowCount(); ++row) {
        for (int column = 0; column < charactersInfoModel()->columnCount(); ++column) {
            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
            cursor.setBlockFormat(blockFormat);
            cursor.insertText(charactersInfoModel()->index(row, column).data().toString());
            cursor.movePosition(QTextCursor::NextBlock);
        }
    }
    cursor.insertBlock();
    cursor.insertBlock();

    tableFormat.setColumnWidthConstraints({
        QTextLength{ QTextLength::PercentageLength, 68 },
        QTextLength{ QTextLength::PercentageLength, 16 },
        QTextLength{ QTextLength::PercentageLength, 16 },
    });
    beforeTablePosition = cursor.position();
    cursor.insertTable(dialoguesInfoModel()->rowCount() + 1, dialoguesInfoModel()->columnCount(),
                       tableFormat);
    cursor.setPosition(beforeTablePosition);
    cursor.movePosition(QTextCursor::NextBlock);
    //
    for (int column = 0; column < dialoguesInfoModel()->columnCount(); ++column) {
        QTextTableCellFormat cellFormat;
        cellFormat.setBottomBorder(1);
        cellFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        cellFormat.setBottomBorderBrush(Qt::black);
        cursor.mergeBlockCharFormat(cellFormat);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
        cursor.setBlockFormat(blockFormat);
        cursor.insertText(dialoguesInfoModel()->headerData(column, Qt::Horizontal).toString());

        cursor.movePosition(QTextCursor::NextBlock);
    }
    for (int row = 0; row < dialoguesInfoModel()->rowCount(); ++row) {
        for (int column = 0; column < dialoguesInfoModel()->columnCount(); ++column) {
            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
            cursor.setBlockFormat(blockFormat);
            cursor.insertText(dialoguesInfoModel()->index(row, column).data().toString());
            cursor.movePosition(QTextCursor::NextBlock);
        }
    }
    cursor.insertBlock();
    cursor.insertBlock();

    tableFormat.setColumnWidthConstraints({
        QTextLength{ QTextLength::PercentageLength, 68 },
        QTextLength{ QTextLength::PercentageLength, 16 },
        QTextLength{ QTextLength::PercentageLength, 16 },
    });
    beforeTablePosition = cursor.position();
    cursor.insertTable(scenesInfoModel()->rowCount() + 1, scenesInfoModel()->columnCount(),
                       tableFormat);
    cursor.setPosition(beforeTablePosition);
    cursor.movePosition(QTextCursor::NextBlock);
    //
    for (int column = 0; column < scenesInfoModel()->columnCount(); ++column) {
        QTextTableCellFormat cellFormat;
        cellFormat.setBottomBorder(1);
        cellFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        cellFormat.setBottomBorderBrush(Qt::black);
        cursor.mergeBlockCharFormat(cellFormat);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
        cursor.setBlockFormat(blockFormat);
        cursor.insertText(scenesInfoModel()->headerData(column, Qt::Horizontal).toString());

        cursor.movePosition(QTextCursor::NextBlock);
    }
    for (int row = 0; row < scenesInfoModel()->rowCount(); ++row) {
        for (int column = 0; column < scenesInfoModel()->columnCount(); ++column) {
            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
            cursor.setBlockFormat(blockFormat);
            cursor.insertText(scenesInfoModel()->index(row, column).data().toString());
            cursor.movePosition(QTextCursor::NextBlock);
        }
    }
    cursor.endEditBlock();


    //
    // Печатаем
    //
    QPdfWriter printer(_fileName);
    printer.setPageSize(QPageSize(exportTemplate.pageSizeId()));
    printer.setPageMargins({});
    report.print(&printer);
}

void AudioplayGenderReport::saveToXlsx(const QString& _fileName) const
{
    QXlsx::Document xlsx;
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);

    constexpr int firstRow = 1;
    constexpr int firstColumn = 1;
    int reportRow = firstRow;
    auto writeTitle = [&xlsx, &headerFormat, &reportRow](const QString& _text) {
        xlsx.write(reportRow++, 1, _text, headerFormat);
        ++reportRow;
    };
    auto writeHeader = [&xlsx, &headerFormat, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text, headerFormat);
    };
    auto writeText = [&xlsx, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text);
    };

    //
    // Сводка
    //
    writeTitle(
        QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Gender analysis"));
    int reportColumn = firstColumn;
    auto testResult = [](int _passedTimes) {
        return _passedTimes == 0
            ? QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Does not pass")
            : QCoreApplication::translate("BusinessLayer::AudioplayGenderReport",
                                          "Passed %n time(s)", "", _passedTimes);
    };
    writeHeader(
        reportColumn++,
        QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Bechdel test"));
    writeText(reportColumn, testResult(bechdelTest()));
    ++reportRow;
    reportColumn = firstColumn;
    writeHeader(reportColumn++,
                QCoreApplication::translate("BusinessLayer::AudioplayGenderReport",
                                            "Reverse Bechdel test"));
    writeText(reportColumn, testResult(reverseBechdelTest()));

    //
    // Статистика по персонажам
    //
    reportRow += 2;
    writeTitle(QCoreApplication::translate("BusinessLayer::AudioplayGenderReport",
                                           "Characters statistics"));
    for (int column = firstColumn; column < firstColumn + charactersInfoModel()->columnCount();
         ++column) {
        writeHeader(column,
                    charactersInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < charactersInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 4; ++column) {
            writeText(column, charactersInfoModel()->index(row, column - firstColumn).data());
        }
    }

    //
    // Статистика по репликам
    //
    reportRow += 2;
    writeTitle(QCoreApplication::translate("BusinessLayer::AudioplayGenderReport",
                                           "Dialogues statistics"));
    for (int column = firstColumn; column < firstColumn + dialoguesInfoModel()->columnCount();
         ++column) {
        writeHeader(column, dialoguesInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < dialoguesInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 3; ++column) {
            writeText(column, dialoguesInfoModel()->index(row, column - firstColumn).data());
        }
    }

    //
    // Статистика по сценам
    //
    reportRow += 2;
    writeTitle(
        QCoreApplication::translate("BusinessLayer::AudioplayGenderReport", "Scenes statistics"));
    for (int column = firstColumn; column < firstColumn + scenesInfoModel()->columnCount();
         ++column) {
        writeHeader(column, scenesInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < scenesInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 3; ++column) {
            writeText(column, scenesInfoModel()->index(row, column - firstColumn).data());
        }
    }

    xlsx.saveAs(_fileName);
}

} // namespace BusinessLayer
