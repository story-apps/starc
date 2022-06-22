#include "recycle_bin_manager.h"

#include "recycle_bin_view.h"

#include <business_layer/model/recycle_bin/recycle_bin_model.h>


namespace ManagementLayer {

class RecycleBinManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::RecycleBinView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::RecycleBinView* _view);


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::RecycleBinModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::RecycleBinView* view = nullptr;
    Ui::RecycleBinView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        Ui::RecycleBinView* view = nullptr;
        QPointer<BusinessLayer::RecycleBinModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::RecycleBinView* RecycleBinManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::RecycleBinView;
    setModelForView(_model, view);
    return view;
}

void RecycleBinManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                        Ui::RecycleBinView* _view)
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
    auto model = qobject_cast<BusinessLayer::RecycleBinModel*>(_model);

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
        _view->setDocumentsToRemoveSize(model->documentsToRemoveSize());

        connect(_view, &Ui::RecycleBinView::emptyRecycleBinPressed, model,
                &BusinessLayer::RecycleBinModel::emptyRecycleBinRequested);
        connect(model, &BusinessLayer::RecycleBinModel::documentsToRemoveSizeChanged, _view,
                &Ui::RecycleBinView::setDocumentsToRemoveSize);
    }
}


// ****


RecycleBinManager::RecycleBinManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

RecycleBinManager::~RecycleBinManager() = default;

Ui::IDocumentView* RecycleBinManager::view()
{
    return d->view;
}

Ui::IDocumentView* RecycleBinManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* RecycleBinManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* RecycleBinManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* RecycleBinManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void RecycleBinManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        d->setModelForView(nullptr, viewAndModel.view);
    }
}

} // namespace ManagementLayer
