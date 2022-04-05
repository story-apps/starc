#include "audioplay_synopsis_model.h"


namespace BusinessLayer {

AudioplaySynopsisModel::AudioplaySynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
{
    setName(tr("Synopsis"));
}

void AudioplaySynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

} // namespace BusinessLayer
