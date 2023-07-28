#include "novel_title_page_model.h"

#include "novel_information_model.h"


namespace BusinessLayer {

class NovelTitlePageModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    NovelInformationModel* informationModel = nullptr;
};

NovelTitlePageModel::NovelTitlePageModel(QObject* _parent)
    : TitlePageModel(_parent)
    , d(new Implementation)
{
}

NovelTitlePageModel::~NovelTitlePageModel() = default;

QString NovelTitlePageModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Title page"), d->informationModel->name());
}

void NovelTitlePageModel::setInformationModel(NovelInformationModel* _model)
{
    d->informationModel = _model;
}

NovelInformationModel* NovelTitlePageModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
