#include "screenplay_synopsis_model.h"

#include "screenplay_information_model.h"


namespace BusinessLayer {

class ScreenplaySynopsisModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    ScreenplayInformationModel* informationModel = nullptr;
};

ScreenplaySynopsisModel::ScreenplaySynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
    setName(tr("Synopsis"));
}

ScreenplaySynopsisModel::~ScreenplaySynopsisModel() = default;

QString ScreenplaySynopsisModel::documentName() const
{
    return QString("%1 | %2").arg(name(), d->informationModel->name());
}

void ScreenplaySynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void ScreenplaySynopsisModel::setInformationModel(ScreenplayInformationModel* _model)
{
    d->informationModel = _model;
}

ScreenplayInformationModel* ScreenplaySynopsisModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
