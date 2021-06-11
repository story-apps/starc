#include "summary_report.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QStandardItemModel>

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
    const QVector<ScreenplayParagraphType> paragraphTypes
        = { ScreenplayParagraphType::SceneHeading,  ScreenplayParagraphType::SceneCharacters,
            ScreenplayParagraphType::Action,        ScreenplayParagraphType::Character,
            ScreenplayParagraphType::Parenthetical, ScreenplayParagraphType::Dialogue,
            ScreenplayParagraphType::Lyrics,        ScreenplayParagraphType::Transition,
            ScreenplayParagraphType::Shot };
    QHash<ScreenplayParagraphType, Counters> paragraphsToCounters;
    for (const auto type : paragraphTypes) {
        paragraphsToCounters.insert(type, {});
    }
    int totalWords = 0;
    CharactersCount totalCharacters;

    //
    // Собираем статистику
    //
    std::function<void(const ScreenplayTextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &paragraphsToCounters, &totalWords,
                       &totalCharacters](const ScreenplayTextModelItem* _item) {
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
    // ... сводка
    //
    d->duration = screenplayModel->duration();
    d->pagesCount = [screenplayModel] {
        const auto screenplayTemplate = TemplatesFacade::screenplayTemplate();

        PageTextEdit textEdit;
        textEdit.setUsePageMode(true);
        textEdit.setPageSpacing(0);
        textEdit.setPageFormat(screenplayTemplate.pageSizeId());
        textEdit.setPageMargins(screenplayTemplate.pageMargins());
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
    auto createModelItem = [](const QString& _text) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };
    for (int index = 0; index < paragraphTypes.size(); ++index) {
        const auto& paragraphType = paragraphTypes.at(index);
        auto paragraphName = createModelItem(toDisplayString(paragraphType));
        paragraphName->setData(u8"\U000F0766", Qt::DecorationRole);
        paragraphName->setData(makeColor(index), Qt::DecorationPropertyRole);

        const auto& paragraphCounters = paragraphsToCounters[paragraphType];

        d->textInfoModel->appendRow(
            { paragraphName, createModelItem(QString::number(paragraphCounters.occurrences)),
              createModelItem(QString::number(paragraphCounters.words)),
              createModelItem(QString::number(paragraphCounters.words * 100.0 / totalWords, 'g', 3)
                              + "%") });
    }
    //
    d->textInfoModel->setHeaderData(
        0, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Paragraph"),
        Qt::DisplayRole);
    d->textInfoModel->setHeaderData(
        1, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Occurrences"),
        Qt::DisplayRole);
    d->textInfoModel->setHeaderData(
        2, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Words"),
        Qt::DisplayRole);
    d->textInfoModel->setHeaderData(
        3, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplaySummaryReport", "Percents"),
        Qt::DisplayRole);
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

} // namespace BusinessLayer
