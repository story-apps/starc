#include "novel_information_manager.h"

#include "novel_information_view.h"

#include <business_layer/model/novel/novel_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class NovelInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::NovelInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::NovelInformationView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::NovelInformationView* view = nullptr;
    Ui::NovelInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::NovelInformationView> view;
        QPointer<BusinessLayer::NovelInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::NovelInformationView* NovelInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::NovelInformationView;
    setModelForView(_model, view);
    return view;
}

void NovelInformationManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                              Ui::NovelInformationView* _view)
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
        _view->setName(model->name());
        _view->setTagline(model->tagline());
        _view->setLogline(model->logline());
        _view->setTitlePageVisible(model->titlePageVisible());
        _view->setSynopsisVisible(model->synopsisVisible());
        _view->setOutlineVisible(model->outlineVisible());
        _view->setNovelTextVisible(model->novelTextVisible());
        _view->setNovelStatisticsVisible(model->novelStatisticsVisible());

        connect(model, &BusinessLayer::NovelInformationModel::nameChanged, _view,
                &Ui::NovelInformationView::setName);
        connect(model, &BusinessLayer::NovelInformationModel::taglineChanged, _view,
                &Ui::NovelInformationView::setTagline);
        connect(model, &BusinessLayer::NovelInformationModel::loglineChanged, _view,
                &Ui::NovelInformationView::setLogline);
        connect(model, &BusinessLayer::NovelInformationModel::titlePageVisibleChanged, _view,
                &Ui::NovelInformationView::setTitlePageVisible);
        connect(model, &BusinessLayer::NovelInformationModel::synopsisVisibleChanged, _view,
                &Ui::NovelInformationView::setSynopsisVisible);
        connect(model, &BusinessLayer::NovelInformationModel::outlineVisibleChanged, _view,
                &Ui::NovelInformationView::setOutlineVisible);
        connect(model, &BusinessLayer::NovelInformationModel::novelTextVisibleChanged, _view,
                &Ui::NovelInformationView::setNovelTextVisible);
        connect(model, &BusinessLayer::NovelInformationModel::novelStatisticsVisibleChanged, _view,
                &Ui::NovelInformationView::setNovelStatisticsVisible);
        //
        connect(_view, &Ui::NovelInformationView::nameChanged, model,
                &BusinessLayer::NovelInformationModel::setName);
        connect(_view, &Ui::NovelInformationView::taglineChanged, model,
                &BusinessLayer::NovelInformationModel::setTagline);
        connect(_view, &Ui::NovelInformationView::loglineChanged, model,
                &BusinessLayer::NovelInformationModel::setLogline);
        connect(_view, &Ui::NovelInformationView::titlePageVisibleChanged, model,
                &BusinessLayer::NovelInformationModel::setTitlePageVisible);
        connect(_view, &Ui::NovelInformationView::synopsisVisibleChanged, model,
                &BusinessLayer::NovelInformationModel::setSynopsisVisible);
        connect(_view, &Ui::NovelInformationView::outlineVisibleChanged, model,
                &BusinessLayer::NovelInformationModel::setOutlineVisible);
        connect(_view, &Ui::NovelInformationView::novelTextVisibleChanged, model,
                &BusinessLayer::NovelInformationModel::setNovelTextVisible);
        connect(_view, &Ui::NovelInformationView::novelStatisticsVisibleChanged, model,
                &BusinessLayer::NovelInformationModel::setNovelStatisticsVisible);
    }
}


// ****


NovelInformationManager::NovelInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

NovelInformationManager::~NovelInformationManager() = default;

Ui::IDocumentView* NovelInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* NovelInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* NovelInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* NovelInformationManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* NovelInformationManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void NovelInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void NovelInformationManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
