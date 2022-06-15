#include "screenplay_synopsis_model.h"


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
