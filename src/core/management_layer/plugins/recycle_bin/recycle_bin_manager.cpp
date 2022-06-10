#include "recycle_bin_manager.h"

#include "recycle_bin_view.h"

#include <business_layer/model/recycle_bin/recycle_bin_model.h>


namespace ManagementLayer {

class RecycleBinManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::RecycleBinView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::RecycleBinModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::RecycleBinView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::RecycleBinView*> allViews;
};

RecycleBinManager::Implementation::Implementation()
{
    view = createView();
}

Ui::RecycleBinView* RecycleBinManager::Implementation::createView()
{
    allViews.append(new Ui::RecycleBinView);
    return allViews.last();
}


// ****


RecycleBinManager::RecycleBinManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

RecycleBinManager::~RecycleBinManager() = default;

void RecycleBinManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::RecycleBinModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setDocumentsToRemoveSize(d->model->documentsToRemoveSize());

        connect(d->view, &Ui::RecycleBinView::emptyRecycleBinPressed, d->model,
                &BusinessLayer::RecycleBinModel::emptyRecycleBinRequested);
        connect(d->model, &BusinessLayer::RecycleBinModel::documentsToRemoveSizeChanged, d->view,
                &Ui::RecycleBinView::setDocumentsToRemoveSize);
    }
}

Ui::IDocumentView* RecycleBinManager::view()
{
    return d->view;
}

Ui::IDocumentView* RecycleBinManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
