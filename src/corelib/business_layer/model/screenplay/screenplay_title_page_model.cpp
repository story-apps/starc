#include "screenplay_title_page_model.h"


namespace BusinessLayer
{

ScreenplayTitlePageModel::ScreenplayTitlePageModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplayTitlePageModel::initDocument()
{
}

void ScreenplayTitlePageModel::clearDocument()
{
}

QByteArray ScreenplayTitlePageModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer
