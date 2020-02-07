#include "screenplay_text_model.h"

#include "screenplay_text_model_scene_item.h"


namespace BusinessLayer
{

class ScreenplayTextModel::Implementation
{
public:
    Implementation();


    /**
     * @brief Корневой элемент дерева
     */
    ScreenplayTextModelItem* rootItem = nullptr;
};

ScreenplayTextModel::Implementation::Implementation()
    : rootItem(new ScreenplayTextModelSceneItem)
{
}


// ****


ScreenplayTextModel::ScreenplayTextModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
}

ScreenplayTextModel::~ScreenplayTextModel() = default;

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
