#include "stageplay_title_page_model.h"


namespace BusinessLayer {

class StageplayTitlePageModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    StageplayInformationModel* informationModel = nullptr;
};


// ****


StageplayTitlePageModel::StageplayTitlePageModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
}

StageplayTitlePageModel::~StageplayTitlePageModel() = default;

void StageplayTitlePageModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void StageplayTitlePageModel::setInformationModel(StageplayInformationModel* _model)
{
    d->informationModel = _model;
}

StageplayInformationModel* StageplayTitlePageModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
