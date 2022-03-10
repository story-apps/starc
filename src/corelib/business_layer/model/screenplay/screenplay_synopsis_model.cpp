#include "screenplay_synopsis_model.h"


namespace BusinessLayer {

ScreenplaySynopsisModel::ScreenplaySynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
{
    setName(tr("Synopsis"));
}

void ScreenplaySynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

} // namespace BusinessLayer
