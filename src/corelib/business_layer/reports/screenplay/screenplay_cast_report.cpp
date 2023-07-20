#include "screenplay_cast_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QPdfWriter>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardItemModel>


namespace BusinessLayer {

class ScreenplayCastReport::Implementation
{
public:
    /**
     * @brief Модель аудиопостановки
     */
    QPointer<ScreenplayTextModel> screenplayModel;

    /**
     * @brief Модель персонажей
     */
    QScopedPointer<QStandardItemModel> castModel;

    /**
     * @brief Нужно ли показывать расширенную стату по сценам
     */
    bool showSceneDetails = true;

    /**
     * @brief Нужно ли показывать стату по словам
     */
    bool showWords = false;

    /**
     * @brief Порядок сортировки
     */
    int sortBy = 0;
};


// ****


ScreenplayCastReport::ScreenplayCastReport()
    : d(new Implementation)
{
}

ScreenplayCastReport::~ScreenplayCastReport() = default;

void ScreenplayCastReport::build(QAbstractItemModel* _model)
{
    if (_model == nullptr) {
        return;
    }

    d->screenplayModel = qobject_cast<ScreenplayTextModel*>(_model);
    if (d->screenplayModel == nullptr) {
        return;
    }

    //
    // Подготовим необходимые структуры для сбора статистики
    //
    struct CharacterData {
        int totalWords = 0;
        int totalDialogues = 0;
        int speakingScenesCount = 0;
        int nonspeakingScenesCount = 0;
        int totalScenes() const
        {
            return speakingScenesCount + nonspeakingScenesCount;
        }
    };
    // - персонаж - кол-во реплик
    QHash<QString, CharacterData> charactersData;
    QSet<QString> lastSceneNonspeakingCharacters;
    QSet<QString> lastSceneSpeakingCharacters;
    QVector<QString> charactersOrder;
    QString lastSpeakingCharacter;

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    QString rxPattern;
    auto charactersModel = d->screenplayModel->charactersList();
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
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &charactersData, &lastSceneNonspeakingCharacters,
                       &lastSceneSpeakingCharacters, &charactersOrder, &lastSpeakingCharacter,
                       &rxCharacterFinder](const TextModelItem* _item) {
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
                    lastSceneNonspeakingCharacters.clear();
                    lastSceneSpeakingCharacters.clear();
                    break;
                }

                case TextParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        lastSceneNonspeakingCharacters.insert(character);
                        //
                        // Первое упоминание персонажа - первая молчаливая сцена
                        //
                        if (!charactersData.contains(character)) {
                            charactersData.insert(character, { 0, 0, 0, 1 });
                            charactersOrder.append(character);
                        }
                        //
                        // Не первое упоминание - плюс одна молчаливая сцена
                        //
                        else {
                            ++charactersData[character].nonspeakingScenesCount;
                        }
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    const auto character = ScreenplayCharacterParser::name(textItem->text());
                    if (character.isEmpty()) {
                        break;
                    }

                    if (!charactersData.contains(character)) {
                        charactersData.insert(character, { 1, 1, 1, 0 });
                        charactersOrder.append(character);
                        lastSceneSpeakingCharacters.insert(character);
                    } else {
                        auto& characterData = charactersData[character];
                        if (lastSceneNonspeakingCharacters.contains(character)) {
                            lastSceneNonspeakingCharacters.remove(character);
                            lastSceneSpeakingCharacters.insert(character);
                            --characterData.nonspeakingScenesCount;
                            ++characterData.speakingScenesCount;
                        } else if (!lastSceneSpeakingCharacters.contains(character)) {
                            lastSceneSpeakingCharacters.insert(character);
                            ++characterData.speakingScenesCount;
                        }
                        ++characterData.totalDialogues;
                    }
                    lastSpeakingCharacter = character;
                    break;
                }

                case TextParagraphType::Dialogue:
                case TextParagraphType::Lyrics: {
                    if (lastSpeakingCharacter.isEmpty()) {
                        break;
                    }

                    auto& characterData = charactersData[lastSpeakingCharacter];
                    characterData.totalWords += TextHelper::wordsCount(textItem->text());
                    break;
                }

                case TextParagraphType::Action: {
                    if (rxCharacterFinder.pattern().isEmpty()) {
                        break;
                    }

                    auto match = rxCharacterFinder.match(textItem->text());
                    while (match.hasMatch()) {
                        const QString character = TextHelper::smartToUpper(match.captured(2));
                        if (!charactersData.contains(character)) {
                            charactersData.insert(character, { 0, 0, 0, 1 });
                            charactersOrder.append(character);
                            lastSceneNonspeakingCharacters.insert(character);
                        } else {
                            //
                            // Если он ещё не добавлен в текущую сцену
                            //
                            if (!lastSceneNonspeakingCharacters.contains(character)
                                && !lastSceneSpeakingCharacters.contains(character)) {
                                lastSceneNonspeakingCharacters.insert(character);
                                ++charactersData[character].nonspeakingScenesCount;
                            }
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

                //
                // Очищаем последнего говорящего персонажа, если ушли из реплики
                //
                if (!lastSpeakingCharacter.isEmpty()
                    && textItem->paragraphType() != TextParagraphType::Character
                    && textItem->paragraphType() != TextParagraphType::Parenthetical
                    && textItem->paragraphType() != TextParagraphType::Dialogue
                    && textItem->paragraphType() != TextParagraphType::Lyrics) {
                    lastSpeakingCharacter.clear();
                }

                break;
            }

            default:
                break;
            }
        }
    };
    includeInReport(d->screenplayModel->itemForIndex({}));

    //
    // Формируем отчёт
    //
    auto createModelItem = [](const QString& _text) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };

    //
    // Сортируем персонажей
    //
    QVector<QPair<QString, CharacterData>> charactersSorted;
    for (auto iter = charactersData.begin(); iter != charactersData.end(); ++iter) {
        charactersSorted.append({ iter.key(), iter.value() });
    }
    switch (d->sortBy) {
    default:
    case 0: {
        break;
    }

    case 1: {
        std::sort(
            charactersSorted.begin(), charactersSorted.end(),
            [](const QPair<QString, CharacterData>& _lhs,
               const QPair<QString, CharacterData>& _rhs) { return _lhs.first < _rhs.first; });
        break;
    }

    case 2: {
        std::sort(charactersSorted.begin(), charactersSorted.end(),
                  [](const QPair<QString, CharacterData>& _lhs,
                     const QPair<QString, CharacterData>& _rhs) {
                      return _lhs.second.totalScenes() > _rhs.second.totalScenes();
                  });
        break;
    }

    case 3: {
        std::sort(charactersSorted.begin(), charactersSorted.end(),
                  [](const QPair<QString, CharacterData>& _lhs,
                     const QPair<QString, CharacterData>& _rhs) {
                      return _lhs.second.totalScenes() < _rhs.second.totalScenes();
                  });
        break;
    }

    case 4: {
        std::sort(charactersSorted.begin(), charactersSorted.end(),
                  [](const QPair<QString, CharacterData>& _lhs,
                     const QPair<QString, CharacterData>& _rhs) {
                      return _lhs.second.totalDialogues > _rhs.second.totalDialogues;
                  });
        break;
    }

    case 5: {
        std::sort(charactersSorted.begin(), charactersSorted.end(),
                  [](const QPair<QString, CharacterData>& _lhs,
                     const QPair<QString, CharacterData>& _rhs) {
                      return _lhs.second.totalDialogues < _rhs.second.totalDialogues;
                  });
        break;
    }
    }

    //
    // Формируем таблицу
    //
    if (d->castModel.isNull()) {
        d->castModel.reset(new QStandardItemModel);
    } else {
        d->castModel->clear();
    }
    //
    auto addCharacterItemToReport
        = [this, &createModelItem](const QString& _name, const CharacterData& _count) {
              auto characterItem = createModelItem(_name);
              d->castModel->appendRow({
                  characterItem,
                  createModelItem(QString::number(_count.totalWords)),
                  createModelItem(QString::number(_count.totalDialogues)),
                  createModelItem(QString::number(_count.speakingScenesCount)),
                  createModelItem(QString::number(_count.nonspeakingScenesCount)),
                  createModelItem(QString::number(_count.totalScenes())),
              });
          };
    for (const auto& character : charactersSorted) {
        addCharacterItemToReport(character.first, character.second);
    }
    //
    d->castModel->setHeaderData(
        0, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Character name"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        1, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Total words"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        2, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Total dialogues"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        3, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Speaking scenes"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        4, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Nonspeaking scenes"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        5, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Total scenes"),
        Qt::DisplayRole);

    if (!d->showSceneDetails) {
        d->castModel->removeColumn(4);
        d->castModel->removeColumn(3);
    }
    if (!d->showWords) {
        d->castModel->removeColumn(1);
    }
}

void ScreenplayCastReport::setParameters(bool _showSceneDetails, bool _showWords, int _sortBy)
{
    d->showSceneDetails = _showSceneDetails;
    d->showWords = _showWords;
    d->sortBy = _sortBy;
}

QAbstractItemModel* ScreenplayCastReport::castModel() const
{
    return d->castModel.data();
}

void ScreenplayCastReport::saveToPdf(const QString& _fileName) const
{
    const auto& exportTemplate
        = TemplatesFacade::screenplayTemplate(d->screenplayModel->informationModel()->templateId());

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
    QTextCharFormat titleFormat;
    auto titleFont = report.defaultFont();
    titleFont.setBold(true);
    titleFormat.setFont(titleFont);
    cursor.setCharFormat(titleFormat);
    cursor.insertText(QString("%1 - %2").arg(
        d->screenplayModel->informationModel()->name(),
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Cast report")));
    cursor.insertBlock();
    cursor.insertBlock();
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    tableFormat.setColumnWidthConstraints({
        QTextLength{ QTextLength::PercentageLength,
                     d->showSceneDetails ? 34. : (d->showWords ? 50. : 66.) },
    });
    const auto beforeTablePosition = cursor.position();
    cursor.insertTable(castModel()->rowCount() + 1, castModel()->columnCount(), tableFormat);
    cursor.setPosition(beforeTablePosition);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.beginEditBlock();
    //
    for (int column = 0; column < castModel()->columnCount(); ++column) {
        QTextTableCellFormat cellFormat;
        cellFormat.setBottomBorder(1);
        cellFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        cellFormat.setBottomBorderBrush(Qt::black);
        cursor.mergeBlockCharFormat(cellFormat);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
        cursor.setBlockFormat(blockFormat);
        cursor.insertText(castModel()->headerData(column, Qt::Horizontal).toString());

        cursor.movePosition(QTextCursor::NextBlock);
    }
    for (int row = 0; row < castModel()->rowCount(); ++row) {
        for (int column = 0; column < castModel()->columnCount(); ++column) {
            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
            cursor.setBlockFormat(blockFormat);
            cursor.insertText(castModel()->index(row, column).data().toString());
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

void ScreenplayCastReport::saveToXlsx(const QString& _fileName) const
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

    for (int column = firstColumn; column < firstColumn + castModel()->columnCount(); ++column) {
        writeHeader(column, castModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < castModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + castModel()->columnCount();
             ++column) {
            writeText(column, castModel()->index(row, column - firstColumn).data());
        }
    }

    xlsx.saveAs(_fileName);
}

} // namespace BusinessLayer
