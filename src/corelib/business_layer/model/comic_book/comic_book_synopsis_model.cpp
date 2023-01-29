#include "comic_book_synopsis_model.h"

#include "comic_book_information_model.h"


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
    setName(tr("Synopsis"));
}

ComicBookSynopsisModel::~ComicBookSynopsisModel() = default;

QString ComicBookSynopsisModel::documentName() const
{
    return QString("%1 | %2").arg(name(), d->informationModel->name());
}

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
