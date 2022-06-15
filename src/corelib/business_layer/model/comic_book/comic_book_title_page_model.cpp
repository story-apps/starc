#include "comic_book_title_page_model.h"


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
