#include "stageplay_synopsis_model.h"

#include "stageplay_information_model.h"


namespace BusinessLayer {

class StageplaySynopsisModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    StageplayInformationModel* informationModel = nullptr;
};


// ****


StageplaySynopsisModel::StageplaySynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
    setName(tr("Synopsis"));
}

StageplaySynopsisModel::~StageplaySynopsisModel() = default;

QString StageplaySynopsisModel::documentName() const
{
    return QString("%1 | %2").arg(name(), d->informationModel->name());
}

void StageplaySynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void StageplaySynopsisModel::setInformationModel(StageplayInformationModel* _model)
{
    d->informationModel = _model;
}

StageplayInformationModel* StageplaySynopsisModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
