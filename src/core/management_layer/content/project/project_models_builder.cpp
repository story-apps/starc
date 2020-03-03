#include "project_models_builder.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/project/project_information_model.h>
#include <business_layer/model/recycle_bin/recycle_bin_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_outline_model.h>
#include <business_layer/model/screenplay/screenplay_synopsis_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model.h>

#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>


namespace ManagementLayer
{

class ProjectModelsBuilder::Implementation
{
public:
    explicit Implementation(BusinessLayer::AbstractImageWrapper* _imageWrapper);

    QHash<Domain::DocumentObject*, BusinessLayer::AbstractModel*> documentsToModels;
    BusinessLayer::AbstractImageWrapper* imageWrapper = nullptr;
};

ProjectModelsBuilder::Implementation::Implementation(BusinessLayer::AbstractImageWrapper* _imageWrapper)
    : imageWrapper(_imageWrapper)
{
}


// ****


ProjectModelsBuilder::ProjectModelsBuilder(BusinessLayer::AbstractImageWrapper* _imageWrapper, QObject* _parent)
    : QObject(_parent),
      d(new Implementation(_imageWrapper))
{
}

ProjectModelsBuilder::~ProjectModelsBuilder()
{
    clear();
}

void ProjectModelsBuilder::clear()
{
    for (auto model : d->documentsToModels.values()) {
        model->disconnect();
        model->clear();
    }

    qDeleteAll(d->documentsToModels.values());
    d->documentsToModels.clear();
}

BusinessLayer::AbstractModel* ProjectModelsBuilder::modelFor(Domain::DocumentObject* _document)
{
    if (_document == nullptr) {
        return nullptr;
    }

    if (!d->documentsToModels.contains(_document)) {
        BusinessLayer::AbstractModel* model = nullptr;
        switch (_document->type()) {
            case Domain::DocumentObjectType::Project: {
                //
                // Модель параметров проекта находится в самом менеджере проекта, отсюда она не должна браться
                //
                Q_ASSERT(false);
                return nullptr;
            }

            case Domain::DocumentObjectType::RecycleBin: {
                model = new BusinessLayer::RecycleBinModel;
                break;
            }

            case Domain::DocumentObjectType::Screenplay: {
                model = new BusinessLayer::ScreenplayInformationModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayTitlePage: {
                model = new BusinessLayer::ScreenplayTitlePageModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplaySynopsis: {
                model = new BusinessLayer::ScreenplaySynopsisModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayOutline: {
                model = new BusinessLayer::ScreenplayOutlineModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayText: {
                auto screenplayModel = new BusinessLayer::ScreenplayTextModel;

                //
                // Добавляем в модель сценария, модель справочников сценариев
                //
                auto dictionariesDocument
                        = DataStorageLayer::StorageFacade::documentStorage()->document(
                              Domain::DocumentObjectType::ScreenplayDictionaries);
                auto dictionariesModel
                        = qobject_cast<BusinessLayer::ScreenplayDictionariesModel*>(
                              modelFor(dictionariesDocument));
                screenplayModel->setDictionariesModel(dictionariesModel);
                //
                // ... модель персонажей
                //
                auto charactersDocument
                        = DataStorageLayer::StorageFacade::documentStorage()->document(
                              Domain::DocumentObjectType::Characters);
                auto charactersModel
                        = qobject_cast<BusinessLayer::CharactersModel*>(
                              modelFor(charactersDocument));
                screenplayModel->setCharactersModel(charactersModel);
                //
                // ... и модель локаций
                //
                auto locationsDocument
                        = DataStorageLayer::StorageFacade::documentStorage()->document(
                              Domain::DocumentObjectType::Locations);
                auto locationsModel
                        = qobject_cast<BusinessLayer::LocationsModel*>(
                              modelFor(locationsDocument));
                screenplayModel->setLocationsModel(locationsModel);

                model = screenplayModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayDictionaries: {
                model = new BusinessLayer::ScreenplayDictionariesModel;
                break;
            }

            case Domain::DocumentObjectType::Characters: {
                model = new BusinessLayer::CharactersModel;
                break;
            }

            case Domain::DocumentObjectType::Locations: {
                model = new BusinessLayer::LocationsModel;
                break;
            }

            default: {
                return nullptr;
            }
        }
        model->setImageWrapper(d->imageWrapper);

        model->setDocument(_document);

        connect(model, &BusinessLayer::AbstractModel::contentsChanged, this,
                [this, model] (const QByteArray& _undo, const QByteArray& _redo) {
           emit modelContentChanged(model, _undo, _redo);
        });

        d->documentsToModels.insert(_document, model);
    }

    return d->documentsToModels.value(_document);
}

void ProjectModelsBuilder::removeModelFor(Domain::DocumentObject* _document)
{
    auto model = d->documentsToModels.take(_document);
    model->disconnect();
    model->clear();
    model->deleteLater();
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsBuilder::models() const
{
    return d->documentsToModels.values().toVector();
}

} // namespace ManagementLayer
