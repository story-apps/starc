#include "screenplay_information_manager.h"

#include "screenplay_information_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer
{

class ScreenplayInformationManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::ScreenplayInformationModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayInformationView*> allViews;
};

ScreenplayInformationManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayInformationView* ScreenplayInformationManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayInformationView);
    return allViews.last();
}


// ****


ScreenplayInformationManager::ScreenplayInformationManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

ScreenplayInformationManager::~ScreenplayInformationManager() = default;

void ScreenplayInformationManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setLogline(d->model->logline());

        connect(d->model, &BusinessLayer::ScreenplayInformationModel::nameChanged,
                d->view, &Ui::ScreenplayInformationView::setName);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::loglineChanged,
                d->view, &Ui::ScreenplayInformationView::setLogline);
        //
        connect(d->view, &Ui::ScreenplayInformationView::nameChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setName);
        connect(d->view, &Ui::ScreenplayInformationView::loglineChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setLogline);
    }
}

QWidget* ScreenplayInformationManager::view()
{
    return d->view;
}

QWidget* ScreenplayInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
