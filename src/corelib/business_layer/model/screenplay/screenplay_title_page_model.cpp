#include "screenplay_title_page_model.h"

#include "screenplay_information_model.h"


namespace BusinessLayer {

class ScreenplayTitlePageModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    ScreenplayInformationModel* informationModel = nullptr;
};

ScreenplayTitlePageModel::ScreenplayTitlePageModel(QObject* _parent)
    : TitlePageModel(_parent)
    , d(new Implementation)
{
}

ScreenplayTitlePageModel::~ScreenplayTitlePageModel() = default;

QString ScreenplayTitlePageModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Title page"), d->informationModel->name());
}

void ScreenplayTitlePageModel::setInformationModel(ScreenplayInformationModel* _model)
{
    d->informationModel = _model;
}

ScreenplayInformationModel* ScreenplayTitlePageModel::informationModel() const
{
    return d->informationModel;
}

} // namespace BusinessLayer
