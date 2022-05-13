#include "stageplay_summary_report.h"

#include <business_layer/document/stageplay/text/stageplay_text_document.h>
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
    int panelsCount = 0;
    int wordsCount = 0;
    CharactersCount charactersCount;

    QScopedPointer<QStandardItemModel> textInfoModel;
};


// ****


StageplaySummaryReport::StageplaySummaryReport()
    : d(new Implementation)
{
}

StageplaySummaryReport::~StageplaySummaryReport() = default;

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
        TextParagraphType::PageHeading, TextParagraphType::PanelHeading,
        TextParagraphType::Description, TextParagraphType::Character,
        TextParagraphType::Dialogue,
    };
    for (const auto type : paragraphTypes) {
        paragraphsToCounters.insert(type, {});
    }
    int totalPages = 0;
    int totalPanels = 0;
    int totalWords = 0;
    CharactersCount totalCharacters;

    //
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&totalPages, &totalPanels, &includeInReport, &paragraphsToCounters,
                       &totalWords, &totalCharacters](const TextModelItem* _item) {
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
                case TextParagraphType::PageHeading: {
                    ++totalPages;
                    break;
                }

                case TextParagraphType::PanelHeading: {
                    ++totalPanels;
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
        d->pagesCount = totalPages;
        d->panelsCount = totalPanels;
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
}

int StageplaySummaryReport::pagesCount() const
{
    return d->pagesCount;
}

int StageplaySummaryReport::panelsCount() const
{
    return d->panelsCount;
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

} // namespace BusinessLayer
