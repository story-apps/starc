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
        d->view->setHeader(d->model->header());
        d->view->setFooter(d->model->footer());
        d->view->setScenesNumbersPrefix(d->model->scenesNumbersPrefix());
        d->view->setScenesNumbersingStartAt(d->model->scenesNumberingStartAt());

        connect(d->model, &BusinessLayer::ScreenplayInformationModel::nameChanged,
                d->view, &Ui::ScreenplayInformationView::setName);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::headerChanged,
                d->view, &Ui::ScreenplayInformationView::setHeader);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::footerChanged,
                d->view, &Ui::ScreenplayInformationView::setFooter);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::scenesNumbersPrefixChanged,
                d->view, &Ui::ScreenplayInformationView::setScenesNumbersPrefix);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::scenesNumberingStartAtChanged,
                d->view, &Ui::ScreenplayInformationView::setScenesNumbersingStartAt);
        //
        connect(d->view, &Ui::ScreenplayInformationView::nameChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setName);
        connect(d->view, &Ui::ScreenplayInformationView::headerChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setHeader);
        connect(d->view, &Ui::ScreenplayInformationView::footerChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setFooter);
        connect(d->view, &Ui::ScreenplayInformationView::scenesNumbersPrefixChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setScenesNumbersPrefix);
        connect(d->view, &Ui::ScreenplayInformationView::scenesNumberingStartAtChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setScenesNumberingStartAt);
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
