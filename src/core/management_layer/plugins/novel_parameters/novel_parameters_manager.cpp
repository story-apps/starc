#include "novel_parameters_manager.h"

#include "novel_parameters_view.h"

#include <business_layer/model/novel/novel_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class NovelParametersManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::NovelParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::NovelParametersView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::NovelParametersView* view = nullptr;
    Ui::NovelParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::NovelParametersView> view;
        QPointer<BusinessLayer::NovelInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::NovelParametersView* NovelParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::NovelParametersView;
    setModelForView(_model, view);
    return view;
}

void NovelParametersManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                             Ui::NovelParametersView* _view)
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
    auto model = qobject_cast<BusinessLayer::NovelInformationModel*>(_model);

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
        _view->setNovelTemplate(model->templateId());

        connect(model, &BusinessLayer::NovelInformationModel::headerChanged, _view,
                &Ui::NovelParametersView::setHeader);
        connect(model, &BusinessLayer::NovelInformationModel::printHeaderOnTitlePageChanged, _view,
                &Ui::NovelParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::NovelInformationModel::footerChanged, _view,
                &Ui::NovelParametersView::setFooter);
        connect(model, &BusinessLayer::NovelInformationModel::printFooterOnTitlePageChanged, _view,
                &Ui::NovelParametersView::setPrintFooterOnTitlePage);
        connect(model, &BusinessLayer::NovelInformationModel::overrideCommonSettingsChanged, _view,
                &Ui::NovelParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::NovelInformationModel::templateIdChanged, _view,
                &Ui::NovelParametersView::setNovelTemplate);
        //
        connect(_view, &Ui::NovelParametersView::headerChanged, model,
                &BusinessLayer::NovelInformationModel::setHeader);
        connect(_view, &Ui::NovelParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::NovelInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::NovelParametersView::footerChanged, model,
                &BusinessLayer::NovelInformationModel::setFooter);
        connect(_view, &Ui::NovelParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::NovelInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::NovelParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::NovelInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::NovelParametersView::novelTemplateChanged, model,
                &BusinessLayer::NovelInformationModel::setTemplateId);
    }
}


// ****


NovelParametersManager::NovelParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

NovelParametersManager::~NovelParametersManager() = default;

Ui::IDocumentView* NovelParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* NovelParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* NovelParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* NovelParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* NovelParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void NovelParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void NovelParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
