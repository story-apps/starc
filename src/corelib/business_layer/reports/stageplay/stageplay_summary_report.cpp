#include "stageplay_summary_report.h"

#include <business_layer/document/stageplay/text/stageplay_text_document.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QStandardItemModel>

#include <set>

namespace BusinessLayer {

namespace {

QColor makeColor(int _index)
{
    const int kMax = 9;
    while (_index > kMax) {
        _index = _index % kMax;
    }
    switch (_index) {
    default:
    case 0:
        return "#41AEFF";
    case 1:
        return "#B2E82A";
    case 2:
        return "#1E844F";
    case 3:
        return "#3D2AA7";
    case 4:
        return "#E64C4D";
    case 5:
        return "#F8B50D";
    case 6:
        return "#A245E0";
    case 7:
        return "#41D089";
    case 8:
        return "#6843FE";
    case 9:
        return "#EB0E4D";
    }
}

} // namespace

class StageplaySummaryReport::Implementation
{
public:
    int pagesCount = 0;
    int wordsCount = 0;
    CharactersCount charactersCount;

    QScopedPointer<QStandardItemModel> textInfoModel;
    QScopedPointer<QStandardItemModel> charactersInfoModel;
};


// ****


StageplaySummaryReport::StageplaySummaryReport()
    : d(new Implementation)
{
}

StageplaySummaryReport::~StageplaySummaryReport() = default;

bool StageplaySummaryReport::isValid() const
{
    return d->wordsCount > 0;
}

void StageplaySummaryReport::build(QAbstractItemModel* _model)
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
        TextParagraphType::SceneHeading,
        TextParagraphType::Character,
        TextParagraphType::Dialogue,
        TextParagraphType::Action,
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
                      auto textItem = static_cast<TextModelTextItem*>(childItem);
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
    auto stageplayModel = static_cast<StageplayTextModel*>(_model);
    includeInReport(stageplayModel->itemForIndex({}));

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
        d->pagesCount = [stageplayModel] {
            const auto& stageplayTemplate = TemplatesFacade::stageplayTemplate(
                stageplayModel->informationModel()->templateId());

            PageTextEdit textEdit;
            textEdit.setUsePageMode(true);
            textEdit.setPageSpacing(0);
            textEdit.setPageFormat(stageplayTemplate.pageSizeId());
            textEdit.setPageMarginsMm(stageplayTemplate.pageMargins());
            StageplayTextDocument stageplayDocument;
            textEdit.setDocument(&stageplayDocument);

            const bool kCanChangeModel = false;
            stageplayDocument.setModel(stageplayModel, kCanChangeModel);

            return stageplayDocument.pageCount();
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
            paragraphItem->setData(makeColor(index), Qt::DecorationPropertyRole);

            d->textInfoModel->appendRow(
                { paragraphItem, createModelItem(QString::number(paragraphCounters.words)),
                  createModelItem(QString::number(paragraphCounters.occurrences)),
                  createPercentModelItem(paragraphCounters.words * 100.0 / totalWords) });
        }
        //
        d->textInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Paragraph"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Words"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            3, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Percents"),
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
            characterItem->setData(makeColor(index++), Qt::DecorationPropertyRole);

            d->charactersInfoModel->appendRow(
                { characterItem, createModelItem(QString::number(_count)),
                  createPercentModelItem(_count * 100.0 / totalCount) });
        };
        addCharacterItemToReport(
            QCoreApplication::translate("BusinessLogic::StageplaySummaryReport",
                                        "More than 10 dialogues"),
            speakMore10);
        addCharacterItemToReport(QCoreApplication::translate(
                                     "BusinessLogic::StageplaySummaryReport", "About 10 dialogues"),
                                 speakAbout10);
        addCharacterItemToReport(
            QCoreApplication::translate("BusinessLogic::StageplaySummaryReport", "Nonspeaking"),
            nonspeaking);
        //
        d->charactersInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Character type"),
            Qt::DisplayRole);
        d->charactersInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->charactersInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::StageplaySummaryReport", "Percents"),
            Qt::DisplayRole);
    }
}

void StageplaySummaryReport::saveToXlsx(const QString& _fileName) const
{
    Q_UNUSED(_fileName)
}

int StageplaySummaryReport::pagesCount() const
{
    return d->pagesCount;
}

int StageplaySummaryReport::wordsCount() const
{
    return d->wordsCount;
}

StageplaySummaryReport::CharactersCount StageplaySummaryReport::charactersCount() const
{
    return d->charactersCount;
}

QAbstractItemModel* StageplaySummaryReport::textInfoModel() const
{
    return d->textInfoModel.data();
}

QAbstractItemModel* StageplaySummaryReport::charactersInfoModel() const
{
    return d->charactersInfoModel.data();
}

} // namespace BusinessLayer
