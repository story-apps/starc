#include "screenplay_treatment_model.h"


namespace BusinessLayer
{

ScreenplayTreatmentModel::ScreenplayTreatmentModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplayTreatmentModel::initDocument()
{
}

void ScreenplayTreatmentModel::clearDocument()
{
}

QByteArray ScreenplayTreatmentModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer
