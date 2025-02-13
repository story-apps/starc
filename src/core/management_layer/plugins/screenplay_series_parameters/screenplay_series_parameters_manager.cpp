#include "screenplay_series_parameters_manager.h"

#include "screenplay_series_parameters_view.h"

#include <business_layer/model/screenplay/series/screenplay_series_information_model.h>


namespace ManagementLayer {

class ScreenplaySeriesParametersManager::Implementation
{
public:
    explicit Implementation(ScreenplaySeriesParametersManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::ScreenplaySeriesParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model,
                         Ui::ScreenplaySeriesParametersView* _view);


    ScreenplaySeriesParametersManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplaySeriesParametersView* view = nullptr;
    Ui::ScreenplaySeriesParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ScreenplaySeriesParametersView> view;
        QPointer<BusinessLayer::ScreenplaySeriesInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

ScreenplaySeriesParametersManager::Implementation::Implementation(
    ScreenplaySeriesParametersManager* _q)
    : q(_q)
{
}

Ui::ScreenplaySeriesParametersView* ScreenplaySeriesParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ScreenplaySeriesParametersView;
    setModelForView(_model, view);

    return view;
}

void ScreenplaySeriesParametersManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplaySeriesParametersView* _view)
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
    auto model = qobject_cast<BusinessLayer::ScreenplaySeriesInformationModel*>(_model);

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
        _view->setScreenplayTemplate(model->templateId());
        _view->setShowSceneNumbers(model->showSceneNumbers());
        _view->setShowSceneNumbersOnLeft(model->showSceneNumbersOnLeft());
        _view->setShowSceneNumbersOnRight(model->showSceneNumbersOnRight());
        _view->setShowDialoguesNumbers(model->showDialoguesNumbers());

        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::headerChanged, _view,
                &Ui::ScreenplaySeriesParametersView::setHeader);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::printHeaderOnTitlePageChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::footerChanged, _view,
                &Ui::ScreenplaySeriesParametersView::setFooter);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::printFooterOnTitlePageChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setPrintFooterOnTitlePage);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::overrideCommonSettingsChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::templateIdChanged, _view,
                &Ui::ScreenplaySeriesParametersView::setScreenplayTemplate);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::showSceneNumbersChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setShowSceneNumbers);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::showSceneNumbersOnLeftChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setShowSceneNumbersOnLeft);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::showSceneNumbersOnRightChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setShowSceneNumbersOnRight);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::showDialoguesNumbersChanged,
                _view, &Ui::ScreenplaySeriesParametersView::setShowDialoguesNumbers);
        //
        connect(_view, &Ui::ScreenplaySeriesParametersView::headerChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setHeader);
        connect(_view, &Ui::ScreenplaySeriesParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::ScreenplaySeriesParametersView::footerChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setFooter);
        connect(_view, &Ui::ScreenplaySeriesParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::ScreenplaySeriesParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::ScreenplaySeriesParametersView::screenplayTemplateChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setTemplateId);
        connect(_view, &Ui::ScreenplaySeriesParametersView::showSceneNumbersChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setShowSceneNumbers);
        connect(_view, &Ui::ScreenplaySeriesParametersView::showSceneNumbersOnLeftChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setShowSceneNumbersOnLeft);
        connect(_view, &Ui::ScreenplaySeriesParametersView::showSceneNumbersOnRightChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setShowSceneNumbersOnRight);
        connect(_view, &Ui::ScreenplaySeriesParametersView::showDialoguesNumbersChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setShowDialoguesNumbers);
    }
}


// ****


ScreenplaySeriesParametersManager::ScreenplaySeriesParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

ScreenplaySeriesParametersManager::~ScreenplaySeriesParametersManager() = default;

Ui::IDocumentView* ScreenplaySeriesParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplaySeriesParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ScreenplaySeriesParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ScreenplaySeriesParametersManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ScreenplaySeriesParametersManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ScreenplaySeriesParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ScreenplaySeriesParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
