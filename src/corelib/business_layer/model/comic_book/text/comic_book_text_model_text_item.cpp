#include "comic_book_text_model_text_item.h"

#include "comic_book_text_model.h"

#include <business_layer/templates/text_template.h>

#include <QRegularExpression>

namespace BusinessLayer {

ComicBookTextModelTextItem::ComicBookTextModelTextItem(const ComicBookTextModel* _model)
    : TextModelTextItem(_model)
{
}

ComicBookTextModelTextItem::~ComicBookTextModelTextItem() = default;

QString ComicBookTextModelTextItem::textToSave() const
{
    if (paragraphType() != TextParagraphType::Character) {
        return text();
    }

    auto textCorrected = text();
    const static QRegularExpression rxNumberRemover("^\\d*[.] ");
    textCorrected = textCorrected.remove(rxNumberRemover);
    return textCorrected;
}

} // namespace BusinessLayer
