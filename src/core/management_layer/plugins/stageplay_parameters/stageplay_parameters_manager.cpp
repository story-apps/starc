#include "stageplay_parameters_manager.h"

#include "stageplay_parameters_view.h"

#include <business_layer/model/stageplay/stageplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class StageplayParametersManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::StageplayParametersView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::StageplayInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::StageplayParametersView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::StageplayParametersView*> allViews;
};

StageplayParametersManager::Implementation::Implementation()
{
    view = createView();
}

Ui::StageplayParametersView* StageplayParametersManager::Implementation::createView()
{
    allViews.append(new Ui::StageplayParametersView);
    return allViews.last();
}


// ****


StageplayParametersManager::StageplayParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

StageplayParametersManager::~StageplayParametersManager() = default;

void StageplayParametersManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        d->view->disconnect(d->model);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::StageplayInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setHeader(d->model->header());
        d->view->setPrintHeaderOnTitlePage(d->model->printHeaderOnTitlePage());
        d->view->setFooter(d->model->footer());
        d->view->setPrintFooterOnTitlePage(d->model->printFooterOnTitlePage());
        d->view->setOverrideCommonSettings(d->model->overrideCommonSettings());
        d->view->setStageplayTemplate(d->model->templateId());
        d->view->setShowBlockNumbers(d->model->showBlockNumbers());
        d->view->setContinueBlockNumbers(d->model->continueBlockNumbers());

        connect(d->model, &BusinessLayer::StageplayInformationModel::headerChanged, d->view,
                &Ui::StageplayParametersView::setHeader);
        connect(d->model, &BusinessLayer::StageplayInformationModel::printHeaderOnTitlePageChanged,
                d->view, &Ui::StageplayParametersView::setPrintHeaderOnTitlePage);
        connect(d->model, &BusinessLayer::StageplayInformationModel::footerChanged, d->view,
                &Ui::StageplayParametersView::setFooter);
        connect(d->model, &BusinessLayer::StageplayInformationModel::printFooterOnTitlePageChanged,
                d->view, &Ui::StageplayParametersView::setPrintFooterOnTitlePage);
        connect(d->model, &BusinessLayer::StageplayInformationModel::overrideCommonSettingsChanged,
                d->view, &Ui::StageplayParametersView::setOverrideCommonSettings);
        connect(d->model, &BusinessLayer::StageplayInformationModel::templateIdChanged, d->view,
                &Ui::StageplayParametersView::setStageplayTemplate);
        connect(d->model, &BusinessLayer::StageplayInformationModel::showBlockNumbersChanged,
                d->view, &Ui::StageplayParametersView::setShowBlockNumbers);
        connect(d->model, &BusinessLayer::StageplayInformationModel::continueBlockNumbersChanged,
                d->view, &Ui::StageplayParametersView::setContinueBlockNumbers);
        //
        connect(d->view, &Ui::StageplayParametersView::headerChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setHeader);
        connect(d->view, &Ui::StageplayParametersView::printHeaderOnTitlePageChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setPrintHeaderOnTitlePage);
        connect(d->view, &Ui::StageplayParametersView::footerChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setFooter);
        connect(d->view, &Ui::StageplayParametersView::printFooterOnTitlePageChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setPrintFooterOnTitlePage);
        connect(d->view, &Ui::StageplayParametersView::overrideCommonSettingsChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setOverrideCommonSettings);
        connect(d->view, &Ui::StageplayParametersView::stageplayTemplateChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setTemplateId);
        connect(d->view, &Ui::StageplayParametersView::showBlockNumbersChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setShowBlockNumbers);
        connect(d->view, &Ui::StageplayParametersView::continueBlockNumbersChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setContinueBlockNumbers);
    }
}

Ui::IDocumentView* StageplayParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* StageplayParametersManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
