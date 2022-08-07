#include "comic_book_text_model_page_item.h"

#include "comic_book_text_model.h"
#include "comic_book_text_model_panel_item.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QXmlStreamReader>


namespace BusinessLayer {

class ComicBookTextModelPageItem::Implementation
{
public:
    /**
     * @brief Номер страницы
     */
    std::optional<PageNumber> number;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Количество панелей на странице
     */
    int panelsCount = 0;

    /**
     * @brief Количество слов в диалогах
     */
    int dialoguesWordsCount = 0;
};


// ****


ComicBookTextModelPageItem::ComicBookTextModelPageItem(const ComicBookTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Page);
    setLevel(0);
}

ComicBookTextModelPageItem::~ComicBookTextModelPageItem() = default;

std::optional<ComicBookTextModelPageItem::PageNumber> ComicBookTextModelPageItem::pageNumber() const
{
    return d->number;
}

void ComicBookTextModelPageItem::setPageNumber(int& _fromNumber,
                                               const QStringList& _singlePageIntros,
                                               const QStringList& _multiplePageIntros)
{
    Q_UNUSED(_singlePageIntros)

    const auto pageName = heading().split(' ').constFirst().trimmed();

    PageNumber newNumber;
    newNumber.fromPage = _fromNumber;
    if (_multiplePageIntros.contains(pageName, Qt::CaseInsensitive)) {
        newNumber.pageCount = 2;
        newNumber.text = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1-%2" : "%2-%1")
                             .arg(_fromNumber)
                             .arg(_fromNumber + newNumber.pageCount - 1);
    } else {
        newNumber.pageCount = 1;
        newNumber.text = QString::number(_fromNumber);
    }
    _fromNumber += newNumber.pageCount;
    if (d->number.has_value() && d->number->text == newNumber.text) {
        return;
    }

    d->number = { newNumber };
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
    //    setChanged(true);
}

int ComicBookTextModelPageItem::panelsCount() const
{
    return d->panelsCount;
}

int ComicBookTextModelPageItem::dialoguesWordsCount() const
{
    return d->dialoguesWordsCount;
}

QVariant ComicBookTextModelPageItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return (d->number.has_value() && d->number->text.contains('-')) ? u8"\U000F0AB7"
                                                                        : u8"\U000f021a";
    }

    case PagePanelsCountRole: {
        return d->panelsCount;
    }

    case PageDialoguesWordsCountRole: {
        return d->dialoguesWordsCount;
    }

    case PageHasNumberingErrorRole: {
        return d->number->pageCount > 1 && (d->number->fromPage % 2 == 0);
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

QStringRef ComicBookTextModelPageItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray ComicBookTextModelPageItem::customContent() const
{
    return {};
}

void ComicBookTextModelPageItem::handleChange()
{
    QString heading;
    d->panelsCount = 0;
    d->dialoguesWordsCount = 0;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Group: {
            auto childItem = static_cast<ComicBookTextModelPanelItem*>(child);
            d->panelsCount += childItem->panelNumber()->panelCount;
            d->dialoguesWordsCount += childItem->dialoguesWordsCount();
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<TextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::PageHeading) {
                heading = TextHelper::smartToUpper(childItem->text());
            }
            break;
        }

        default:
            break;
        }
    }

    setHeading(heading);
}

} // namespace BusinessLayer
