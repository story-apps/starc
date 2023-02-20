#include "novel_synopsis_model.h"

#include "novel_information_model.h"


namespace BusinessLayer {

class NovelSynopsisModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    NovelInformationModel* informationModel = nullptr;
};

NovelSynopsisModel::NovelSynopsisModel(QObject* _parent)
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
    setName(tr("Synopsis"));
}

NovelSynopsisModel::~NovelSynopsisModel() = default;

QString NovelSynopsisModel::documentName() const
{
    return QString("%1 | %2").arg(name(), d->informationModel->name());
}

void NovelSynopsisModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
}

void NovelSynopsisModel::setInformationModel(NovelInformationModel* _model)
{
    d->informationModel = _model;
}

NovelInformationModel* NovelSynopsisModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
