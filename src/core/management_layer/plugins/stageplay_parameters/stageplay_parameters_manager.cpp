#include "stageplay_parameters_manager.h"

#include "stageplay_parameters_view.h"

#include <business_layer/model/stageplay/stageplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class StageplayParametersManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::StageplayParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::StageplayParametersView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::StageplayParametersView* view = nullptr;
    Ui::StageplayParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::StageplayParametersView> view;
        QPointer<BusinessLayer::StageplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::StageplayParametersView* StageplayParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::StageplayParametersView;
    setModelForView(_model, view);
    return view;
}

void StageplayParametersManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::StageplayParametersView* _view)
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
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::StageplayInformationModel*>(_model);

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
        _view->setHeader(model->header());
        _view->setPrintHeaderOnTitlePage(model->printHeaderOnTitlePage());
        _view->setFooter(model->footer());
        _view->setPrintFooterOnTitlePage(model->printFooterOnTitlePage());
        _view->setOverrideCommonSettings(model->overrideCommonSettings());
        _view->setStageplayTemplate(model->templateId());

        connect(model, &BusinessLayer::StageplayInformationModel::headerChanged, _view,
                &Ui::StageplayParametersView::setHeader);
        connect(model, &BusinessLayer::StageplayInformationModel::printHeaderOnTitlePageChanged,
                _view, &Ui::StageplayParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::StageplayInformationModel::footerChanged, _view,
                &Ui::StageplayParametersView::setFooter);
        connect(model, &BusinessLayer::StageplayInformationModel::printFooterOnTitlePageChanged,
                _view, &Ui::StageplayParametersView::setPrintFooterOnTitlePage);
        connect(model, &BusinessLayer::StageplayInformationModel::overrideCommonSettingsChanged,
                _view, &Ui::StageplayParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::StageplayInformationModel::templateIdChanged, _view,
                &Ui::StageplayParametersView::setStageplayTemplate);
        //
        connect(_view, &Ui::StageplayParametersView::headerChanged, model,
                &BusinessLayer::StageplayInformationModel::setHeader);
        connect(_view, &Ui::StageplayParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::StageplayInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::StageplayParametersView::footerChanged, model,
                &BusinessLayer::StageplayInformationModel::setFooter);
        connect(_view, &Ui::StageplayParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::StageplayInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::StageplayParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::StageplayInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::StageplayParametersView::stageplayTemplateChanged, model,
                &BusinessLayer::StageplayInformationModel::setTemplateId);
    }
}


// ****


StageplayParametersManager::StageplayParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

StageplayParametersManager::~StageplayParametersManager() = default;

Ui::IDocumentView* StageplayParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* StageplayParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* StageplayParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* StageplayParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* StageplayParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void StageplayParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void StageplayParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
