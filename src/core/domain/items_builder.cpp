#include "items_builder.h"

//#include "CharacterObject.h"
//#include "LocationObject.h"
//#include "ProjectObject.h"
//#include "StoryObject.h"
//#include "StoryPlotObject.h"
//#include "StorySceneObject.h"
//#include "WorldObject.h"
//#include "WorldRaceObject.h"
//#include "WorldFloraObject.h"
//#include "WorldAnimalObject.h"
//#include "WorldNaturalResourceObject.h"
//#include "WorldClimateObject.h"
//#include "WorldReligionAndBeliefObject.h"
//#include "WorldEthicsAndValuesObject.h"
//#include "WorldLanguageObject.h"
//#include "WorldClassCasteSystemObject.h"
//#include "WorldMagicTypeObject.h"


namespace Domain
{

ItemObject* ItemsBuilder::create(const Identifier& _id, ItemObject* _parent, ItemObjectType _type,
    const QString& _name, const QString& _description, const QColor& _color, int _sortOrder,
    bool _isDeleted, AbstractImageWrapper* _imageWrapper)
{
    switch (_type) {
//        case ItemObjectType::Project: {
//            return new ProjectObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::Character: {
//            return new CharacterObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::Location: {
//            return new LocationObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::World: {
//            return new WorldObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldRace: {
//            return new WorldRaceObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldFlora: {
//            return new WorldFloraObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldAnimal: {
//            return new WorldAnimalObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldNaturalResource: {
//            return new WorldNaturalResourceObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldClimate: {
//            return new WorldClimateObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldReligionAndBelief: {
//            return new WorldReligionAndBeliefObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldEthicsAndValues: {
//            return new WorldEthicsAndValuesObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldLanguage: {
//            return new WorldLanguageObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldClassCasteSystem: {
//            return new WorldClassCasteSystemObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::WorldMagicType: {
//            return new WorldMagicTypeObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::Story: {
//            return new StoryObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::StoryPlot: {
//            return new StoryPlotObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

//        case ItemObjectType::StoryScene: {
//            return new StorySceneObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
//        }

        default: {
            return new ItemObject(_id, _parent, _type, _name, _description, _color, _sortOrder, _isDeleted, _imageWrapper);
        }
    }
}

} // namespace Domain
