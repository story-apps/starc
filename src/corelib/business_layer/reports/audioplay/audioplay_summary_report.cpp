#include "audioplay_summary_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/audioplay/text/audioplay_text_document.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QCoreApplication>
#include <QPdfWriter>
#include <QPointer>
#include <QStandardItemModel>

#include <set>

namespace BusinessLayer {

class AudioplaySummaryReport::Implementation
{
public:
    /**
     * @brief Модель аудиопостановки
     */
    QPointer<AudioplayTextModel> audioplayModel;

    std::chrono::milliseconds duration;
    int pagesCount = 0;
    int wordsCount = 0;
    CharactersCount charactersCount;

    QScopedPointer<QStandardItemModel> textInfoModel;
    QScopedPointer<QStandardItemModel> charactersInfoModel;
};


// ****


AudioplaySummaryReport::AudioplaySummaryReport()
    : d(new Implementation)
{
}

AudioplaySummaryReport::~AudioplaySummaryReport() = default;

void AudioplaySummaryReport::build(QAbstractItemModel* _model)
{
    if (_model == nullptr) {
        return;
    }

    //
    // Подготовим необходимые структуры для сбора статистики
    //
    struct Counters {
        int occurrences = 0;
        int words = 0;
    };
    QHash<TextParagraphType, Counters> paragraphsToCounters;
    const QVector<TextParagraphType> paragraphTypes = {
        TextParagraphType::SceneHeading, TextParagraphType::Character, TextParagraphType::Dialogue,
        TextParagraphType::Sound,        TextParagraphType::Music,     TextParagraphType::Cue,
    };
    for (const auto type : paragraphTypes) {
        paragraphsToCounters.insert(type, {});
    }
    int totalWords = 0;
    CharactersCount totalCharacters;
    // - список сцен
    QVector<QString> scenes;
    // - персонаж - кол-во реплик
    QHash<QString, int> charactersToDialogues;
    QString lastCharacter;

    //
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport
        = [&includeInReport, &paragraphsToCounters, &totalWords, &totalCharacters, &scenes,
           &charactersToDialogues, &lastCharacter](const TextModelItem* _item) {
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
                      // ... счётчики
                      //
                      if (paragraphsToCounters.contains(textItem->paragraphType())) {
                          auto& paragraphCounters = paragraphsToCounters[textItem->paragraphType()];
                          ++paragraphCounters.occurrences;
                          const auto paragraphWords = TextHelper::wordsCount(textItem->text());
                          paragraphCounters.words += paragraphWords;
                          totalWords += paragraphWords;
                          totalCharacters.withSpaces += textItem->text().length();
                          totalCharacters.withoutSpaces
                              += textItem->text().length() - textItem->text().count(' ');
                      }

                      //
                      // ... стата по объектам
                      //
                      switch (textItem->paragraphType()) {
                      case TextParagraphType::SceneHeading: {
                          scenes.append(TextHelper::smartToUpper(textItem->text()));
                          break;
                      }

                      case TextParagraphType::Character: {
                          lastCharacter = textItem->text();
                          if (!charactersToDialogues.contains(lastCharacter)) {
                              charactersToDialogues.insert(lastCharacter, 0);
                          }
                          break;
                      }

                      case TextParagraphType::Dialogue: {
                          charactersToDialogues[lastCharacter] += 1;
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
    d->audioplayModel = static_cast<AudioplayTextModel*>(_model);
    includeInReport(d->audioplayModel->itemForIndex({}));

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
    //
    // ... сводка
    //
    {
        d->duration = d->audioplayModel->duration();
        d->pagesCount = [this] {
            const auto& audioplayTemplate = TemplatesFacade::audioplayTemplate(
                d->audioplayModel->informationModel()->templateId());

            PageTextEdit textEdit;
            textEdit.setUsePageMode(true);
            textEdit.setPageSpacing(0);
            textEdit.setPageFormat(audioplayTemplate.pageSizeId());
            textEdit.setPageMarginsMm(audioplayTemplate.pageMargins());
            AudioplayTextDocument audioplayDocument;
            textEdit.setDocument(&audioplayDocument);

            const bool kCanChangeModel = false;
            audioplayDocument.setModel(d->audioplayModel, kCanChangeModel);

            return audioplayDocument.pageCount();
        }();
        d->wordsCount = totalWords;
        d->charactersCount = totalCharacters;
        //
        // ... иформация по тексту
        //
        if (d->textInfoModel.isNull()) {
            d->textInfoModel.reset(new QStandardItemModel);
        } else {
            d->textInfoModel->clear();
        }
        //
        for (int index = 0; index < paragraphTypes.size(); ++index) {
            const auto& paragraphType = paragraphTypes.at(index);
            const auto& paragraphCounters = paragraphsToCounters[paragraphType];
            if (paragraphCounters.occurrences == 0) {
                continue;
            }

            auto paragraphItem = createModelItem(toDisplayString(paragraphType));
            paragraphItem->setData(u8"\U000F0766", Qt::DecorationRole);
            paragraphItem->setData(ColorHelper::forNumber(index), Qt::DecorationPropertyRole);

            d->textInfoModel->appendRow(
                { paragraphItem, createModelItem(QString::number(paragraphCounters.words)),
                  createModelItem(QString::number(paragraphCounters.occurrences)),
                  createPercentModelItem(paragraphCounters.words * 100.0 / totalWords) });
        }
        //
        d->textInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Paragraph"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Words"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            3, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Percents"),
            Qt::DisplayRole);
    }
    //
    // ... персонажи
    //
    {
        //
        // Группируем персонажей
        //
        int nonspeaking = 0, speakAbout10 = 0, speakMore10 = 0;
        for (const auto& characterDialogues : std::as_const(charactersToDialogues)) {
            if (characterDialogues == 0) {
                nonspeaking += 1;
            } else if (characterDialogues <= 10) {
                speakAbout10 += 1;
            } else {
                speakMore10 += 1;
            }
        }

        //
        // Формируем таблицу
        //
        if (d->charactersInfoModel.isNull()) {
            d->charactersInfoModel.reset(new QStandardItemModel);
        } else {
            d->charactersInfoModel->clear();
        }
        //
        const auto totalCount = nonspeaking + speakAbout10 + speakMore10;
        int index = 0;
        auto addCharacterItemToReport = [this, &createModelItem, &createPercentModelItem,
                                         &totalCount, &index](const QString& _name, int _count) {
            auto characterItem = createModelItem(_name);
            characterItem->setData(u8"\U000F0766", Qt::DecorationRole);
            characterItem->setData(ColorHelper::forNumber(index++), Qt::DecorationPropertyRole);

            d->charactersInfoModel->appendRow(
                { characterItem, createModelItem(QString::number(_count)),
                  createPercentModelItem(_count * 100.0 / totalCount) });
        };
        addCharacterItemToReport(
            QCoreApplication::translate("BusinessLogic::AudioplaySummaryReport",
                                        "More than 10 dialogues"),
            speakMore10);
        addCharacterItemToReport(QCoreApplication::translate(
                                     "BusinessLogic::AudioplaySummaryReport", "About 10 dialogues"),
                                 speakAbout10);
        //
        d->charactersInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Character type"),
            Qt::DisplayRole);
        d->charactersInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->charactersInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Percents"),
            Qt::DisplayRole);
    }
}

std::chrono::milliseconds AudioplaySummaryReport::duration() const
{
    return d->duration;
}

int AudioplaySummaryReport::pagesCount() const
{
    return d->pagesCount;
}

int AudioplaySummaryReport::wordsCount() const
{
    return d->wordsCount;
}

AudioplaySummaryReport::CharactersCount AudioplaySummaryReport::charactersCount() const
{
    return d->charactersCount;
}

QAbstractItemModel* AudioplaySummaryReport::textInfoModel() const
{
    return d->textInfoModel.data();
}

QAbstractItemModel* AudioplaySummaryReport::charactersInfoModel() const
{
    return d->charactersInfoModel.data();
}

void AudioplaySummaryReport::saveToPdf(const QString& _fileName) const
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
    report.setDefaultFont(exportTemplate.defaultFont());
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
    cursor.insertText(QString("%1 - %2").arg(
        d->audioplayModel->informationModel()->name(),
        QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Summary report")));
    cursor.insertBlock();
    cursor.insertBlock();

    cursor.insertText(
        QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Duration") + ": "
        + TimeHelper::toString(duration()));
    cursor.insertBlock();
    cursor.insertText(QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Pages")
                      + ": " + QString::number(pagesCount()));
    cursor.insertBlock();
    cursor.insertText(QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport", "Words")
                      + ": " + QString::number(wordsCount()));
    cursor.insertBlock();
    cursor.insertText(
        QCoreApplication::translate("BusinessLayer::AudioplaySummaryReport",
                                    "Characters with/without spaces")
        + ": "
        + QString("%1/%2").arg(charactersCount().withSpaces).arg(charactersCount().withoutSpaces));
    cursor.insertBlock();
    cursor.insertBlock();

    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    tableFormat.setColumnWidthConstraints({
        QTextLength{ QTextLength::PercentageLength, 52 },
    });
    auto beforeTablePosition = cursor.position();
    cursor.insertTable(textInfoModel()->rowCount() + 1, textInfoModel()->columnCount(),
                       tableFormat);
    cursor.setPosition(beforeTablePosition);
    cursor.movePosition(QTextCursor::NextBlock);
    //
    for (int column = 0; column < textInfoModel()->columnCount(); ++column) {
        QTextTableCellFormat cellFormat;
        cellFormat.setBottomBorder(1);
        cellFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        cellFormat.setBottomBorderBrush(Qt::black);
        cursor.mergeBlockCharFormat(cellFormat);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
        cursor.setBlockFormat(blockFormat);
        cursor.insertText(textInfoModel()->headerData(column, Qt::Horizontal).toString());

        cursor.movePosition(QTextCursor::NextBlock);
    }
    for (int row = 0; row < textInfoModel()->rowCount(); ++row) {
        for (int column = 0; column < textInfoModel()->columnCount(); ++column) {
            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
            cursor.setBlockFormat(blockFormat);
            cursor.insertText(textInfoModel()->index(row, column).data().toString());
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
    cursor.endEditBlock();


    //
    // Печатаем
    //
    QPdfWriter printer(_fileName);
    printer.setPageSize(QPageSize(exportTemplate.pageSizeId()));
    printer.setPageMargins({});
    report.print(&printer);
}

void AudioplaySummaryReport::saveToXlsx(const QString& _fileName) const
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
    writeTitle(QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport",
                                           "Summary statistics"));
    int reportColumn = firstColumn;
    writeHeader(reportColumn++,
                QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Duration"));
    writeText(reportColumn, TimeHelper::toString(duration()));
    ++reportRow;
    reportColumn = firstColumn;
    writeHeader(reportColumn++,
                QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Pages"));
    writeText(reportColumn, QString::number(pagesCount()));
    ++reportRow;
    reportColumn = firstColumn;
    writeHeader(reportColumn++,
                QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Words"));
    writeText(reportColumn, QString::number(wordsCount()));
    ++reportRow;
    reportColumn = firstColumn;
    writeHeader(reportColumn++,
                QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport",
                                            "Characters with/without spaces"));
    writeText(
        reportColumn,
        QString("%1/%2").arg(charactersCount().withSpaces).arg(charactersCount().withoutSpaces));

    //
    // Статистика по тексту
    //
    reportRow += 2;
    writeTitle(
        QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Text statistics"));
    for (int column = firstColumn; column < firstColumn + textInfoModel()->columnCount();
         ++column) {
        writeHeader(column, textInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < textInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 4; ++column) {
            writeText(column, textInfoModel()->index(row, column - firstColumn).data());
        }
    }

    //
    // Статистика по персонажам
    //
    reportRow += 2;
    writeTitle(QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport",
                                           "Characters statistics"));
    for (int column = firstColumn; column < firstColumn + charactersInfoModel()->columnCount();
         ++column) {
        writeHeader(column,
                    charactersInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < charactersInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 3; ++column) {
            writeText(column, charactersInfoModel()->index(row, column - firstColumn).data());
        }
    }

    xlsx.saveAs(_fileName);
}

} // namespace BusinessLayer
