#include "project_models_facade.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/location_model.h>
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

class ProjectModelsFacade::Implementation
{
public:
    explicit Implementation(BusinessLayer::AbstractImageWrapper* _imageWrapper);

    QHash<Domain::DocumentObject*, BusinessLayer::AbstractModel*> documentsToModels;
    BusinessLayer::AbstractImageWrapper* imageWrapper = nullptr;
};

ProjectModelsFacade::Implementation::Implementation(BusinessLayer::AbstractImageWrapper* _imageWrapper)
    : imageWrapper(_imageWrapper)
{
}


// ****


ProjectModelsFacade::ProjectModelsFacade(BusinessLayer::AbstractImageWrapper* _imageWrapper, QObject* _parent)
    : QObject(_parent),
      d(new Implementation(_imageWrapper))
{
}

ProjectModelsFacade::~ProjectModelsFacade()
{
    clear();
}

void ProjectModelsFacade::clear()
{
    for (auto model : d->documentsToModels.values()) {
        model->disconnect();
        model->clear();
    }

    qDeleteAll(d->documentsToModels.values());
    d->documentsToModels.clear();
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(Domain::DocumentObject* _document)
{
    if (_document == nullptr) {
        return nullptr;
    }

    if (!d->documentsToModels.contains(_document)) {
        BusinessLayer::AbstractModel* model = nullptr;
        switch (_document->type()) {
            case Domain::DocumentObjectType::Project: {
                auto projectInformationModel = new BusinessLayer::ProjectInformationModel;
                connect(projectInformationModel, &BusinessLayer::ProjectInformationModel::nameChanged,
                        this, &ProjectModelsFacade::projectNameChanged, Qt::UniqueConnection);
                connect(projectInformationModel, &BusinessLayer::ProjectInformationModel::loglineChanged,
                        this, &ProjectModelsFacade::projectLoglineChanged, Qt::UniqueConnection);
                connect(projectInformationModel, &BusinessLayer::ProjectInformationModel::coverChanged,
                        this, &ProjectModelsFacade::projectCoverChanged, Qt::UniqueConnection);
                model = projectInformationModel;
                break;
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
                auto charactersModel = new BusinessLayer::CharactersModel;

                const auto characterDocuments
                        = DataStorageLayer::StorageFacade::documentStorage()->documents(
                              Domain::DocumentObjectType::Character);
                for (const auto characterDocument : characterDocuments) {
                    auto characterModel = modelFor(characterDocument);
                    charactersModel->addCharacterModel(
                        static_cast<BusinessLayer::CharacterModel*>(characterModel));
                }

                connect(charactersModel, &BusinessLayer::CharactersModel::createCharacterRequested,
                        this, &ProjectModelsFacade::createCharacterRequested);

                model = charactersModel;
                break;
            }

            case Domain::DocumentObjectType::Character: {
                model = new BusinessLayer::CharacterModel;
                break;
            }

            case Domain::DocumentObjectType::Locations: {
                auto locationsModel = new BusinessLayer::LocationsModel;

                const auto locationDocuments
                        = DataStorageLayer::StorageFacade::documentStorage()->documents(
                              Domain::DocumentObjectType::Location);
                for (const auto locationDocument : locationDocuments) {
                    auto locationModel = modelFor(locationDocument);
                    locationsModel->addLocationModel(
                        static_cast<BusinessLayer::LocationModel*>(locationModel));
                }

                connect(locationsModel, &BusinessLayer::LocationsModel::createLocationRequested,
                        this, &ProjectModelsFacade::createLocationRequested);

                model = locationsModel;
                break;
            }

            case Domain::DocumentObjectType::Location: {
                model = new BusinessLayer::LocationModel;
                break;
            }

            default: {
                Q_ASSERT(false);
                return nullptr;
            }
        }
        model->setImageWrapper(d->imageWrapper);

        model->setDocument(_document);

        connect(model, &BusinessLayer::AbstractModel::documentNameChanged, this,
                [this, model] (const QString& _name) {
           emit modelNameChanged(model, _name);
        });
        connect(model, &BusinessLayer::AbstractModel::contentsChanged, this,
                [this, model] (const QByteArray& _undo, const QByteArray& _redo) {
           emit modelContentChanged(model, _undo, _redo);
        });

        d->documentsToModels.insert(_document, model);
    }

    return d->documentsToModels.value(_document);
}

void ProjectModelsFacade::removeModelFor(Domain::DocumentObject* _document)
{
    if (!d->documentsToModels.contains(_document)) {
        return;
    }

    auto model = d->documentsToModels.take(_document);
    model->disconnect();
    model->clear();
    model->deleteLater();
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsFacade::models() const
{
    return d->documentsToModels.values().toVector();
}

} // namespace ManagementLayer
