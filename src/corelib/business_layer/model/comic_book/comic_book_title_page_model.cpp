#include "comic_book_title_page_model.h"

#include <business_layer/model/simple_text/simple_text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>


namespace BusinessLayer {

ComicBookTitlePageModel::ComicBookTitlePageModel(QObject* _parent)
    : SimpleTextModel(_parent)
{
}

void ComicBookTitlePageModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void ComicBookTitlePageModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        //
        // FIXME: подгрузка структуры из шаблона сценария
        //
        auto textItem = new SimpleTextModelTextItem;
        textItem->setParagraphType(TextParagraphType::Text);
        appendItem(textItem);
    }
    //
    // А если данные есть, то загрузим их из документа
    //
    else {
        SimpleTextModel::initDocument();
    }
}

} // namespace BusinessLayer
