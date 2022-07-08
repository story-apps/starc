#include "screenplay_parameters_manager.h"

#include "screenplay_parameters_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ScreenplayParametersManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::ScreenplayParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ScreenplayParametersView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayParametersView* view = nullptr;
    Ui::ScreenplayParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ScreenplayParametersView> view;
        QPointer<BusinessLayer::ScreenplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::ScreenplayParametersView* ScreenplayParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ScreenplayParametersView;
    setModelForView(_model, view);
    return view;
}

void ScreenplayParametersManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplayParametersView* _view)
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
    auto model = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(_model);

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
        _view->setScenesNumbersPrefix(model->scenesNumbersPrefix());
        _view->setScenesNumbersingStartAt(model->scenesNumberingStartAt());
        _view->setOverrideCommonSettings(model->overrideCommonSettings());
        _view->setScreenplayTemplate(model->templateId());
        _view->setShowSceneNumbers(model->showSceneNumbers());
        _view->setShowSceneNumbersOnLeft(model->showSceneNumbersOnLeft());
        _view->setShowSceneNumbersOnRight(model->showSceneNumbersOnRight());
        _view->setShowDialoguesNumbers(model->showDialoguesNumbers());

        connect(model, &BusinessLayer::ScreenplayInformationModel::headerChanged, _view,
                &Ui::ScreenplayParametersView::setHeader);
        connect(model, &BusinessLayer::ScreenplayInformationModel::printHeaderOnTitlePageChanged,
                _view, &Ui::ScreenplayParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::ScreenplayInformationModel::footerChanged, _view,
                &Ui::ScreenplayParametersView::setFooter);
        connect(model, &BusinessLayer::ScreenplayInformationModel::printFooterOnTitlePageChanged,
                _view, &Ui::ScreenplayParametersView::setPrintFooterOnTitlePage);
        connect(model, &BusinessLayer::ScreenplayInformationModel::scenesNumbersPrefixChanged,
                _view, &Ui::ScreenplayParametersView::setScenesNumbersPrefix);
        connect(model, &BusinessLayer::ScreenplayInformationModel::scenesNumberingStartAtChanged,
                _view, &Ui::ScreenplayParametersView::setScenesNumbersingStartAt);
        connect(model, &BusinessLayer::ScreenplayInformationModel::overrideCommonSettingsChanged,
                _view, &Ui::ScreenplayParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::ScreenplayInformationModel::templateIdChanged, _view,
                &Ui::ScreenplayParametersView::setScreenplayTemplate);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showSceneNumbersChanged, _view,
                &Ui::ScreenplayParametersView::setShowSceneNumbers);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showSceneNumbersOnLeftChanged,
                _view, &Ui::ScreenplayParametersView::setShowSceneNumbersOnLeft);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showSceneNumbersOnRightChanged,
                _view, &Ui::ScreenplayParametersView::setShowSceneNumbersOnRight);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showDialoguesNumbersChanged,
                _view, &Ui::ScreenplayParametersView::setShowDialoguesNumbers);
        //
        connect(_view, &Ui::ScreenplayParametersView::headerChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setHeader);
        connect(_view, &Ui::ScreenplayParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::ScreenplayParametersView::footerChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setFooter);
        connect(_view, &Ui::ScreenplayParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::ScreenplayParametersView::scenesNumbersPrefixChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScenesNumbersPrefix);
        connect(_view, &Ui::ScreenplayParametersView::scenesNumberingStartAtChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScenesNumberingStartAt);
        connect(_view, &Ui::ScreenplayParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::ScreenplayParametersView::screenplayTemplateChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setTemplateId);
        connect(_view, &Ui::ScreenplayParametersView::showSceneNumbersChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowSceneNumbers);
        connect(_view, &Ui::ScreenplayParametersView::showSceneNumbersOnLeftChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowSceneNumbersOnLeft);
        connect(_view, &Ui::ScreenplayParametersView::showSceneNumbersOnRightChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowSceneNumbersOnRight);
        connect(_view, &Ui::ScreenplayParametersView::showDialoguesNumbersChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowDialoguesNumbers);
    }
}


// ****


ScreenplayParametersManager::ScreenplayParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ScreenplayParametersManager::~ScreenplayParametersManager() = default;

Ui::IDocumentView* ScreenplayParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ScreenplayParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ScreenplayParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ScreenplayParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
