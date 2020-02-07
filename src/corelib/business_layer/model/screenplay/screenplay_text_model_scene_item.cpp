#include "screenplay_text_model_scene_item.h"

#include <QUuid>
#include <QVariant>

#include <optional>


namespace BusinessLayer
{

class ScreenplayTextModelSceneItem::Implementation
{
public:
    /**
     * @brief Идентификатор сцены
     */
    QUuid uuid;

    /**
     * @brief Пропущена ли сцена
     */
    bool isOmited = false;

    /**
     * @brief Номер сцены
     */
    struct SceneNumber {
        QString value;
        int group = 0;
        int groupIndex = 0;
    };
    std::optional<SceneNumber> number;

    /**
     * @brief Штамп на сцене
     */
    QString stamp;

    /**
     * @brief Запланированная длительность сцены
     */
    std::optional<int> plannedDuration;
};


// ****


ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Scene),
      d(new Implementation)
{
}

QVariant ScreenplayTextModelSceneItem::data(int _role) const
{
    return {};
}

ScreenplayTextModelSceneItem::~ScreenplayTextModelSceneItem() = default;

} // namespace BusinessLayer
