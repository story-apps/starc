#include "comic_book_synopsis_model.h"


namespace BusinessLayer {

ComicBookSynopsisModel::ComicBookSynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
{
}

void ComicBookSynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

} // namespace BusinessLayer
