#include "screenplay_summary_report.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
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

class ScreenplaySummaryReport::Implementation
{
public:
    std::chrono::milliseconds duration;
    int pagesCount = 0;
    int wordsCount = 0;
    CharactersCount charactersCount;

    QScopedPointer<QStandardItemModel> textInfoModel;
    QScopedPointer<QStandardItemModel> scenesInfoModel;
    QScopedPointer<QStandardItemModel> locationsInfoModel;
    QScopedPointer<QStandardItemModel> charactersInfoModel;
};


// ****


ScreenplaySummaryReport::ScreenplaySummaryReport()
    : d(new Implementation)
{
}

ScreenplaySummaryReport::~ScreenplaySummaryReport() = default;

void ScreenplaySummaryReport::build(QAbstractItemModel* _model)
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
    QHash<ScreenplayParagraphType, Counters> paragraphsToCounters;
    const QVector<ScreenplayParagraphType> paragraphTypes
        = { ScreenplayParagraphType::SceneHeading,  ScreenplayParagraphType::SceneCharacters,
            ScreenplayParagraphType::Action,        ScreenplayParagraphType::Character,
            ScreenplayParagraphType::Parenthetical, ScreenplayParagraphType::Dialogue,
            ScreenplayParagraphType::Lyrics,        ScreenplayParagraphType::Transition,
            ScreenplayParagraphType::Shot };
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
    std::function<void(const ScreenplayTextModelItem*)> includeInReport;
    includeInReport
        = [&includeInReport, &paragraphsToCounters, &totalWords, &totalCharacters, &scenes,
           &charactersToDialogues, &lastCharacter](const ScreenplayTextModelItem* _item) {
              for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                  auto childItem = _item->childAt(childIndex);
                  switch (childItem->type()) {
                  case ScreenplayTextModelItemType::Folder:
                  case ScreenplayTextModelItemType::Scene: {
                      includeInReport(childItem);
                      break;
                  }

                  case ScreenplayTextModelItemType::Text: {
                      auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
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
                      case ScreenplayParagraphType::SceneHeading: {
                          scenes.append(TextHelper::smartToUpper(textItem->text()));
                          break;
                      }

                      case ScreenplayParagraphType::SceneCharacters: {
                          const auto sceneCharacters
                              = SceneCharactersParser::characters(textItem->text());
                          for (const auto& character : sceneCharacters) {
                              if (!charactersToDialogues.contains(character)) {
                                  charactersToDialogues.insert(character, 0);
                              }
                          }
                          break;
                      }

                      case ScreenplayParagraphType::Character: {
                          lastCharacter = CharacterParser::name(textItem->text());
                          if (!charactersToDialogues.contains(lastCharacter)) {
                              charactersToDialogues.insert(lastCharacter, 0);
                          }
                          break;
                      }

                      case ScreenplayParagraphType::Dialogue:
                      case ScreenplayParagraphType::Lyrics: {
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
    auto screenplayModel = static_cast<ScreenplayTextModel*>(_model);
    includeInReport(screenplayModel->itemForIndex({}));

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
        d->duration = screenplayModel->duration();
        d->pagesCount = [screenplayModel] {
            const auto screenplayTemplate = TemplatesFacade::screenplayTemplate();

            PageTextEdit textEdit;
            textEdit.setUsePageMode(true);
            textEdit.setPageSpacing(0);
            textEdit.setPageFormat(screenplayTemplate.pageSizeId());
            textEdit.setPageMarginsMm(screenplayTemplate.pageMargins());
            ScreenplayTextDocument screenplayDocument;
            screenplayDocument.setTemplateId(screenplayTemplate.id());
            textEdit.setDocument(&screenplayDocument);

            const bool kCanChangeModel = false;
            screenplayDocument.setModel(screenplayModel, kCanChangeModel);

            return screenplayDocument.pageCount();
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
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Paragraph"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Words"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->textInfoModel->setHeaderData(
            3, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Percents"),
            Qt::DisplayRole);
    }
    //
    // ... сцены
    //
    {
        QHash<QString, int> sceneTimesToCount;
        for (const auto& scene : scenes) {
            const QString time = SceneHeadingParser::sceneTime(scene);
            if (!sceneTimesToCount.contains(time)) {
                sceneTimesToCount.insert(time, 0);
            }
            sceneTimesToCount[time] += 1;
        }

        //
        // Сортируем по наибольшему вхождению и удаляем дубликаты
        //
        const std::set<int, std::greater<int>> counts(sceneTimesToCount.begin(),
                                                      sceneTimesToCount.end());

        //
        // Формируем таблицу
        //
        if (d->scenesInfoModel.isNull()) {
            d->scenesInfoModel.reset(new QStandardItemModel);
        } else {
            d->scenesInfoModel->clear();
        }
        //
        const auto totalCount
            = std::accumulate(sceneTimesToCount.begin(), sceneTimesToCount.end(), 0);
        int index = 0;
        for (const auto count : counts) {
            auto sceneTimes = sceneTimesToCount.keys(count);
            std::sort(sceneTimes.begin(), sceneTimes.end());
            for (const auto& time : sceneTimes) {
                auto timeName
                    = createModelItem(time.isEmpty() ? QCoreApplication::translate(
                                          "BusinessLogic::ScreenplaySummaryReport", "[UNDEFINED]")
                                                     : time);
                timeName->setData(u8"\U000F0766", Qt::DecorationRole);
                timeName->setData(makeColor(index++), Qt::DecorationPropertyRole);

                d->scenesInfoModel->appendRow(
                    { timeName, createModelItem(QString::number(count)),
                      createPercentModelItem(count * 100.0 / totalCount) });
            }
        }
        //
        d->scenesInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Scene time"),
            Qt::DisplayRole);
        d->scenesInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->scenesInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Percents"),
            Qt::DisplayRole);
    }
    //
    // ... локации
    //
    {
        QMap<QString, int> locationPlacesToCount;
        for (const auto& scene : scenes) {
            const auto place = SceneHeadingParser::sceneIntro(scene);
            if (!locationPlacesToCount.contains(place)) {
                locationPlacesToCount.insert(place, 0);
            }
            locationPlacesToCount[place] += 1;
        }

        //
        // Сортируем по наибольшему вхождению и удаляем дубликаты
        //
        const std::set<int, std::greater<int>> counts(locationPlacesToCount.begin(),
                                                      locationPlacesToCount.end());

        //
        // Формируем таблицу
        //
        if (d->locationsInfoModel.isNull()) {
            d->locationsInfoModel.reset(new QStandardItemModel);
        } else {
            d->locationsInfoModel->clear();
        }
        //
        const auto totalCount
            = std::accumulate(locationPlacesToCount.begin(), locationPlacesToCount.end(), 0);
        int index = 0;
        for (const auto count : counts) {
            auto locationPlaces = locationPlacesToCount.keys(count);
            std::sort(locationPlaces.begin(), locationPlaces.end());
            for (const auto& place : locationPlaces) {
                auto placeName
                    = createModelItem(place.isEmpty() ? QCoreApplication::translate(
                                          "BusinessLogic::ScreenplaySummaryReport", "[UNDEFINED]")
                                                      : place);
                placeName->setData(u8"\U000F0766", Qt::DecorationRole);
                placeName->setData(makeColor(index++), Qt::DecorationPropertyRole);

                d->locationsInfoModel->appendRow(
                    { placeName, createModelItem(QString::number(count)),
                      createPercentModelItem(count * 100.0 / totalCount) });
            }
        }
        //
        d->locationsInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Scene intro"),
            Qt::DisplayRole);
        d->locationsInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->locationsInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Percents"),
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
            QCoreApplication::translate("BusinessLogic::ScreenplaySummaryReport",
                                        "More then 10 dialogues"),
            speakMore10);
        addCharacterItemToReport(
            QCoreApplication::translate("BusinessLogic::ScreenplaySummaryReport",
                                        "About 10 dialogues"),
            speakAbout10);
        addCharacterItemToReport(
            QCoreApplication::translate("BusinessLogic::ScreenplaySummaryReport", "Nonspeaking"),
            nonspeaking);
        //
        d->charactersInfoModel->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Character type"),
            Qt::DisplayRole);
        d->charactersInfoModel->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Occurrences"),
            Qt::DisplayRole);
        d->charactersInfoModel->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Percents"),
            Qt::DisplayRole);
    }
}

std::chrono::milliseconds ScreenplaySummaryReport::duration() const
{
    return d->duration;
}

int ScreenplaySummaryReport::pagesCount() const
{
    return d->pagesCount;
}

int ScreenplaySummaryReport::wordsCount() const
{
    return d->wordsCount;
}

ScreenplaySummaryReport::CharactersCount ScreenplaySummaryReport::charactersCount() const
{
    return d->charactersCount;
}

QAbstractItemModel* ScreenplaySummaryReport::textInfoModel() const
{
    return d->textInfoModel.data();
}

QAbstractItemModel* ScreenplaySummaryReport::scenesInfoModel() const
{
    return d->scenesInfoModel.data();
}

QAbstractItemModel* ScreenplaySummaryReport::locationsInfoModel() const
{
    return d->locationsInfoModel.data();
}

QAbstractItemModel* ScreenplaySummaryReport::charactersInfoModel() const
{
    return d->charactersInfoModel.data();
}

} // namespace BusinessLayer
