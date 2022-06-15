#include "comic_book_synopsis_model.h"


namespace BusinessLayer {

class ComicBookSynopsisModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    ComicBookInformationModel* informationModel = nullptr;
};


// ****


ComicBookSynopsisModel::ComicBookSynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
}

ComicBookSynopsisModel::~ComicBookSynopsisModel() = default;

void ComicBookSynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void ComicBookSynopsisModel::setInformationModel(ComicBookInformationModel* _model)
{
    d->informationModel = _model;
}

ComicBookInformationModel* ComicBookSynopsisModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
