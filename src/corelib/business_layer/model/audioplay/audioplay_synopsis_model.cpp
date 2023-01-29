#include "audioplay_synopsis_model.h"

#include "audioplay_information_model.h"


namespace BusinessLayer {

class AudioplaySynopsisModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    AudioplayInformationModel* informationModel = nullptr;
};


// ****


AudioplaySynopsisModel::AudioplaySynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
    setName(tr("Synopsis"));
}

AudioplaySynopsisModel::~AudioplaySynopsisModel() = default;

QString AudioplaySynopsisModel::documentName() const
{
    return QString("%1 | %2").arg(name(), d->informationModel->name());
}

void AudioplaySynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void AudioplaySynopsisModel::setInformationModel(AudioplayInformationModel* _model)
{
    d->informationModel = _model;
}

AudioplayInformationModel* AudioplaySynopsisModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
