#include "screenplay_title_page_model.h"


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
    : SimpleTextModel(_parent)
    , d(new Implementation)
{
}

ScreenplayTitlePageModel::~ScreenplayTitlePageModel() = default;

void ScreenplayTitlePageModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
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
