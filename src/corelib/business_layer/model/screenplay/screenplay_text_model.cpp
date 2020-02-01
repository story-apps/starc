#include "screenplay_text_model.h"


namespace BusinessLayer
{

ScreenplayTextModel::ScreenplayTextModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplayTextModel::initDocument()
{
}

void ScreenplayTextModel::clearDocument()
{
}

QByteArray ScreenplayTextModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer
