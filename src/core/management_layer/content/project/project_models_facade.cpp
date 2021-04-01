#include "project_models_facade.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/project/project_information_model.h>
#include <business_layer/model/recycle_bin/recycle_bin_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_treatment_model.h>
#include <business_layer/model/screenplay/screenplay_statistics_model.h>
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
    for (auto model : std::as_const(d->documentsToModels)) {
        model->disconnect();
        model->clear();
    }

    qDeleteAll(d->documentsToModels);
    d->documentsToModels.clear();
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(const QUuid& _uuid, const QUuid& _parentUuid)
{
    return modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(_uuid), _parentUuid);
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(Domain::DocumentObjectType _type)
{
    return modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(_type));
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(Domain::DocumentObject* _document, const QUuid& _parentUuid)
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
                auto screenplayModel = new BusinessLayer::ScreenplayInformationModel;
                connect(screenplayModel, &BusinessLayer::ScreenplayInformationModel::titlePageVisibleChanged, this,
                        [this, screenplayModel] (bool _visible) {
                   emit screenplayTitlePageVisibilityChanged(screenplayModel, _visible);
                });
                connect(screenplayModel, &BusinessLayer::ScreenplayInformationModel::synopsisVisibleChanged, this,
                        [this, screenplayModel] (bool _visible) {
                   emit screenplaySynopsisVisibilityChanged(screenplayModel, _visible);
                });
                connect(screenplayModel, &BusinessLayer::ScreenplayInformationModel::treatmentVisibleChanged, this,
                        [this, screenplayModel] (bool _visible) {
                   emit screenplayTreatmentVisibilityChanged(screenplayModel, _visible);
                });
                connect(screenplayModel, &BusinessLayer::ScreenplayInformationModel::screenplayTextVisibleChanged, this,
                        [this, screenplayModel] (bool _visible) {
                   emit screenplayTextVisibilityChanged(screenplayModel, _visible);
                });
                connect(screenplayModel, &BusinessLayer::ScreenplayInformationModel::screenplayStatisticsVisibleChanged, this,
                        [this, screenplayModel] (bool _visible) {
                   emit screenplayStatisticsVisibilityChanged(screenplayModel, _visible);
                });

                model = screenplayModel;
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

            case Domain::DocumentObjectType::ScreenplayTreatment: {
                model = new BusinessLayer::ScreenplayTreatmentModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayText: {
                auto screenplayModel = new BusinessLayer::ScreenplayTextModel;

                //
                // Добавляем в модель сценария, модель информации о сценарие
                //
                auto informationModel
                        = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(
                              modelFor(_parentUuid));
                screenplayModel->setInformationModel(informationModel);
                //
                // ... модель справочников сценариев
                //
                auto dictionariesModel
                        = qobject_cast<BusinessLayer::ScreenplayDictionariesModel*>(
                              modelFor(Domain::DocumentObjectType::ScreenplayDictionaries));
                screenplayModel->setDictionariesModel(dictionariesModel);
                //
                // ... модель персонажей
                //
                auto charactersModel
                        = qobject_cast<BusinessLayer::CharactersModel*>(
                              modelFor(Domain::DocumentObjectType::Characters));
                screenplayModel->setCharactersModel(charactersModel);
                //
                // ... и модель локаций
                //
                auto locationsModel
                        = qobject_cast<BusinessLayer::LocationsModel*>(
                              modelFor(Domain::DocumentObjectType::Locations));
                screenplayModel->setLocationsModel(locationsModel);

                model = screenplayModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayDictionaries: {
                model = new BusinessLayer::ScreenplayDictionariesModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayStatistics: {
                model = new BusinessLayer::ScreenplayStatisticsModel;
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
                        qobject_cast<BusinessLayer::CharacterModel*>(characterModel));
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
                        qobject_cast<BusinessLayer::LocationModel*>(locationModel));
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

            case Domain::DocumentObjectType::Folder:
            case Domain::DocumentObjectType::Text: {
                model = new BusinessLayer::TextModel;
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
        connect(model, &BusinessLayer::AbstractModel::undoRequested, this,
                [this, model] (int _undoStep) {
           emit modelUndoRequested(model, _undoStep);
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
    switch (_document->type()) {
        case Domain::DocumentObjectType::Character: {
            const auto charactersDocuments
                    = DataStorageLayer::StorageFacade::documentStorage()->documents(
                          Domain::DocumentObjectType::Characters);
            Q_ASSERT(charactersDocuments.size() == 1);
            auto charactersModel = modelFor(charactersDocuments.first());
            auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
            characters->removeCharacterModel(qobject_cast<BusinessLayer::CharacterModel*>(model));
            break;
        }

        case Domain::DocumentObjectType::Location: {
            const auto locationsDocuments
                    = DataStorageLayer::StorageFacade::documentStorage()->documents(
                          Domain::DocumentObjectType::Locations);
            Q_ASSERT(locationsDocuments.size() == 1);
            auto locationsModel = modelFor(locationsDocuments.first());
            auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
            locations->removeLocationModel(qobject_cast<BusinessLayer::LocationModel*>(model));
            break;
        }

        default: {
            break;
        }
    }

    model->disconnect();
    model->clear();
    model->deleteLater();
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsFacade::models() const
{
    return d->documentsToModels.values().toVector();
}

} // namespace ManagementLayer
