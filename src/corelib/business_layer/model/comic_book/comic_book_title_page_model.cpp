#include "comic_book_title_page_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>


namespace BusinessLayer {

class ComicBookTitlePageModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    ComicBookInformationModel* informationModel = nullptr;
};


// ****


ComicBookTitlePageModel::ComicBookTitlePageModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
}

ComicBookTitlePageModel::~ComicBookTitlePageModel() = default;

void ComicBookTitlePageModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void ComicBookTitlePageModel::setInformationModel(ComicBookInformationModel* _model)
{
    d->informationModel = _model;
}

ComicBookInformationModel* ComicBookTitlePageModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
