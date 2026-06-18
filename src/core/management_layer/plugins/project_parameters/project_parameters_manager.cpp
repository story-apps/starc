#include "project_parameters_manager.h"

#include "project_parameters_view.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/project/project_information_model.h>


namespace ManagementLayer {

class ProjectParametersManager::Implementation
{
public:
    explicit Implementation(ProjectParametersManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::ProjectParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ProjectParametersView* _view);


    ProjectParametersManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ProjectParametersView* view = nullptr;
    Ui::ProjectParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ProjectParametersView> view;
        QPointer<BusinessLayer::ProjectInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

ProjectParametersManager::Implementation::Implementation(ProjectParametersManager* _q)
    : q(_q)
{
}

Ui::ProjectParametersView* ProjectParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ProjectParametersView;
    setModelForView(_model, view);

    return view;
}

void ProjectParametersManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                               Ui::ProjectParametersView* _view)
{
    constexpr int invalidIndex = -1;
    int viewIndex = invalidIndex;
    for (int index = 0; index < allViews.size(); ++index) {
        if (allViews[index].view == _view) {
            if (allViews[index].model == _model) {
                return;
            }

            viewIndex = index;
            break;
        }
    }

    //
    // Разрываем соединения со старой моделью
    //
    if (viewIndex != invalidIndex && allViews[viewIndex].model != nullptr) {
        _view->disconnect(allViews[viewIndex].model);
        allViews[viewIndex].model->disconnect(_view);
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::ProjectInformationModel*>(_model);

    //
    // Обновляем связь представления с моделью
    //
    if (viewIndex != invalidIndex) {
        allViews[viewIndex].model = model;
    }
    //
    // Или сохраняем связь представления с моделью
    //
    else {
        allViews.append({ _view, model });
    }

    //
    // Настраиваем соединения с новой моделью
    //
    if (model != nullptr) {
        _view->setOverrideCommonSettings(model->overrideCommonSettingsForScreenplay());
        _view->setScreenplayTemplate(model->templateIdForScreenplay());
        _view->setShowSceneNumbers(model->showSceneNumbersForScreenplay());
        _view->setShowSceneNumbersOnLeft(model->showSceneNumbersOnLeftForScreenplay());
        _view->setShowSceneNumbersOnRight(model->showSceneNumbersOnRightForScreenplay());
        _view->setShowDialoguesNumbers(model->showDialoguesNumbersForScreenplay());
        _view->setChronometerOptions(model->chronometerOptionsForScreenplay());

        connect(model,
                &BusinessLayer::ProjectInformationModel::overrideCommonSettingsForScreenplayChanged,
                _view, &Ui::ProjectParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::ProjectInformationModel::templateIdForScreenplayChanged,
                _view, &Ui::ProjectParametersView::setScreenplayTemplate);
        connect(model,
                &BusinessLayer::ProjectInformationModel::showSceneNumbersForScreenplayChanged,
                _view, &Ui::ProjectParametersView::setShowSceneNumbers);
        connect(model,
                &BusinessLayer::ProjectInformationModel::showSceneNumbersOnLeftForScreenplayChanged,
                _view, &Ui::ProjectParametersView::setShowSceneNumbersOnLeft);
        connect(
            model,
            &BusinessLayer::ProjectInformationModel::showSceneNumbersOnRightForScreenplayChanged,
            _view, &Ui::ProjectParametersView::setShowSceneNumbersOnRight);
        connect(model,
                &BusinessLayer::ProjectInformationModel::showDialoguesNumbersForScreenplayChanged,
                _view, &Ui::ProjectParametersView::setShowDialoguesNumbers);
        connect(model,
                &BusinessLayer::ProjectInformationModel::chronometerOptionsForScreenplayChanged,
                _view, &Ui::ProjectParametersView::setChronometerOptions);
        //
        connect(_view, &Ui::ProjectParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::ProjectInformationModel::setOverrideCommonSettingsForScreenplay);
        connect(_view, &Ui::ProjectParametersView::screenplayTemplateChanged, model,
                &BusinessLayer::ProjectInformationModel::setTemplateIdForScreenplay);
        connect(_view, &Ui::ProjectParametersView::showSceneNumbersChanged, model,
                &BusinessLayer::ProjectInformationModel::setShowSceneNumbersForScreenplay);
        connect(_view, &Ui::ProjectParametersView::showSceneNumbersOnLeftChanged, model,
                &BusinessLayer::ProjectInformationModel::setShowSceneNumbersOnLeftForScreenplay);
        connect(_view, &Ui::ProjectParametersView::showSceneNumbersOnRightChanged, model,
                &BusinessLayer::ProjectInformationModel::setShowSceneNumbersOnRightForScreenplay);
        connect(_view, &Ui::ProjectParametersView::showDialoguesNumbersChanged, model,
                &BusinessLayer::ProjectInformationModel::setShowDialoguesNumbersForScreenplay);
        connect(_view, &Ui::ProjectParametersView::chronometerOptionsChanged, model,
                &BusinessLayer::ProjectInformationModel::setChronometerOptionsForScreenplay);
    }
}


// ****


ProjectParametersManager::ProjectParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

ProjectParametersManager::~ProjectParametersManager() = default;

Ui::IDocumentView* ProjectParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* ProjectParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ProjectParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ProjectParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ProjectParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ProjectParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ProjectParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
