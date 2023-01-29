#include "stageplay_title_page_model.h"

#include "stageplay_information_model.h"


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

QString StageplayTitlePageModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Title page"), d->informationModel->name());
}

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
