#include "comic_book_text_model_panel_item.h"

#include "comic_book_text_block_parser.h"
#include "comic_book_text_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QXmlStreamReader>


namespace BusinessLayer {

class ComicBookTextModelPanelItem::Implementation
{
public:
    /**
     * @brief Номер панели
     */
    std::optional<PanelNumber> number;

    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Количество слов в диалогах
     */
    int dialoguesWordsCount = 0;
};


// ****


ComicBookTextModelPanelItem::ComicBookTextModelPanelItem(const ComicBookTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Panel);
    setLevel(1);
}

ComicBookTextModelPanelItem::~ComicBookTextModelPanelItem() = default;

std::optional<ComicBookTextModelPanelItem::PanelNumber> ComicBookTextModelPanelItem::panelNumber()
    const
{
    return d->number;
}

void ComicBookTextModelPanelItem::setPanelNumber(int& _fromNumber,
                                                 const QStringList& _singlePanelIntros,
                                                 const QStringList& _multiplePanelIntros)
{
    Q_UNUSED(_singlePanelIntros)

    const auto panelNameData = heading().split(' ');
    const auto panelName = panelNameData.constFirst().trimmed();
    const auto panelNumbersText = panelNameData.size() > 1 ? panelNameData.at(1) : QString();

    //
    // TODO: Парсить номера сцен, которые задал пользователь вручную
    //

    PanelNumber newNumber;
    newNumber.fromPanel = _fromNumber;
    if (_multiplePanelIntros.contains(panelName, Qt::CaseInsensitive)) {
        if (panelNumbersText.isEmpty() || !panelNumbersText.contains('-')) {
            newNumber.panelCount = 2;
        } else {
            const auto panelNumbers = panelNumbersText.split('-');
            if (panelNumbers.size() != 2) {
                newNumber.panelCount = 2;
            } else {
                const auto firstNumber = panelNumbers.constFirst().toInt();
                const auto lastNumber = panelNumbers.constLast().toInt();
                if (firstNumber < 1 || lastNumber < 2 || firstNumber >= lastNumber) {
                    newNumber.panelCount = 2;
                } else {
                    newNumber.panelCount = lastNumber - firstNumber + 1;
                }
            }
        }
        newNumber.text = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1-%2" : "%2-%1")
                             .arg(_fromNumber)
                             .arg(_fromNumber + newNumber.panelCount - 1);
    } else {
        newNumber.panelCount = 1;
        newNumber.text = QString::number(_fromNumber);
    }
    _fromNumber += newNumber.panelCount;
    if (d->number.has_value() && d->number->text == newNumber.text) {
        return;
    }

    d->number = { newNumber };
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
    //    setChanged(true);
}

int ComicBookTextModelPanelItem::dialoguesWordsCount() const
{
    return d->dialoguesWordsCount;
}

QVariant ComicBookTextModelPanelItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return !customIcon().isEmpty() ? customIcon() : u8"\U000F0B77";
    }

    case PanelDialoguesWordsSizeRole: {
        return d->dialoguesWordsCount;
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

QStringRef ComicBookTextModelPanelItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray ComicBookTextModelPanelItem::customContent() const
{
    return {};
}

void ComicBookTextModelPanelItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    QVector<TextModelTextItem::ReviewMark> reviewMarks;
    d->dialoguesWordsCount = 0;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != TextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<TextModelTextItem*>(child);

        //
        // Собираем текст
        //
        switch (childTextItem->paragraphType()) {
        case TextParagraphType::PanelHeading: {
            heading
                = TextHelper::smartToUpper(ComicBookPanelParser::panelTitle(childTextItem->text()));
            const auto panelDescription
                = ComicBookPanelParser::panelDescription(childTextItem->text());
            text = panelDescription;
            break;
        }

        case TextParagraphType::Dialogue: {
            d->dialoguesWordsCount += TextHelper::wordsCount(childTextItem->text());
            break;
        }

        case TextParagraphType::InlineNote: {
            ++inlineNotesSize;
            break;
        }

        default: {
            if (!text.isEmpty() && !childTextItem->text().isEmpty()) {
                text.append(" ");
            }
            text.append(childTextItem->text());
            break;
        }
        }

        //
        // Собираем редакторские заметки
        //
        if (!reviewMarks.isEmpty() && !childTextItem->reviewMarks().isEmpty()
            && reviewMarks.constLast().isPartiallyEqual(
                childTextItem->reviewMarks().constFirst())) {
            reviewMarks.removeLast();
        }
        reviewMarks.append(childTextItem->reviewMarks());
    }

    setHeading(heading);
    setText(text);
    setInlineNotesSize(inlineNotesSize);
    setReviewMarksSize(std::count_if(
        reviewMarks.begin(), reviewMarks.end(),
        [](const TextModelTextItem::ReviewMark& _reviewMark) { return !_reviewMark.isDone; }));
}

} // namespace BusinessLayer
