#include "project_models_factory.h"

#include <business_layer/model/project_information/project_information_model.h>

#include <domain/document_object.h>


namespace ManagementLayer
{

class ProjectModelsFactory::Implementation
{
public:
    QHash<Domain::DocumentObject*, BusinessLayer::AbstractModel*> documentsToModels;
};


// ****


ProjectModelsFactory::ProjectModelsFactory()
    : d(new Implementation)
{
}

ProjectModelsFactory::~ProjectModelsFactory()
{
    clear();
}

void ProjectModelsFactory::clear()
{
    for (auto model : d->documentsToModels.values()) {
        model->disconnect();
        model->clear();
    }

    qDeleteAll(d->documentsToModels.values());
    d->documentsToModels.clear();
}

BusinessLayer::AbstractModel* ProjectModelsFactory::modelFor(Domain::DocumentObject* _document)
{
    if (_document == nullptr) {
        return nullptr;
    }

    if (!d->documentsToModels.contains(_document)) {
        BusinessLayer::AbstractModel* model = nullptr;
        switch (_document->type()) {
            case Domain::DocumentObjectType::Project: {
                model = new BusinessLayer::ProjectInformationModel;
                break;
            }

            default: {
                return nullptr;
            }
        }

        model->setDocument(_document);
        d->documentsToModels.insert(_document, model);
    }

    return d->documentsToModels.value(_document);
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsFactory::models() const
{
    return d->documentsToModels.values().toVector();
}

} // namespace ManagementLayer
