#include "screenplay_parameters_manager.h"

#include "screenplay_parameters_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer
{

class ScreenplayParametersManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayParametersView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::ScreenplayInformationModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayParametersView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayParametersView*> allViews;
};

ScreenplayParametersManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayParametersView* ScreenplayParametersManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayParametersView);
    return allViews.last();
}


// ****


ScreenplayParametersManager::ScreenplayParametersManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

ScreenplayParametersManager::~ScreenplayParametersManager() = default;

void ScreenplayParametersManager::setModel(BusinessLayer::AbstractModel* _model)
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
        d->view->setHeader(d->model->header());
        d->view->setPrintHeaderOnTitlePage(d->model->printHeaderOnTitlePage());
        d->view->setFooter(d->model->footer());
        d->view->setPrintFooterOnTitlePage(d->model->printFooterOnTitlePage());
        d->view->setScenesNumbersPrefix(d->model->scenesNumbersPrefix());
        d->view->setScenesNumbersingStartAt(d->model->scenesNumberingStartAt());

        connect(d->model, &BusinessLayer::ScreenplayInformationModel::headerChanged,
                d->view, &Ui::ScreenplayParametersView::setHeader);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::printHeaderOnTitlePageChanged,
                d->view, &Ui::ScreenplayParametersView::setPrintHeaderOnTitlePage);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::footerChanged,
                d->view, &Ui::ScreenplayParametersView::setFooter);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::printFooterOnTitlePageChanged,
                d->view, &Ui::ScreenplayParametersView::setPrintFooterOnTitlePage);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::scenesNumbersPrefixChanged,
                d->view, &Ui::ScreenplayParametersView::setScenesNumbersPrefix);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::scenesNumberingStartAtChanged,
                d->view, &Ui::ScreenplayParametersView::setScenesNumbersingStartAt);
        //
        connect(d->view, &Ui::ScreenplayParametersView::headerChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setHeader);
        connect(d->view, &Ui::ScreenplayParametersView::printHeaderOnTitlePageChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setPrintHeaderOnTitlePage);
        connect(d->view, &Ui::ScreenplayParametersView::footerChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setFooter);
        connect(d->view, &Ui::ScreenplayParametersView::printFooterOnTitlePageChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setPrintFooterOnTitlePage);
        connect(d->view, &Ui::ScreenplayParametersView::scenesNumbersPrefixChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setScenesNumbersPrefix);
        connect(d->view, &Ui::ScreenplayParametersView::scenesNumberingStartAtChanged,
                d->model, &BusinessLayer::ScreenplayInformationModel::setScenesNumberingStartAt);
    }
}

QWidget* ScreenplayParametersManager::view()
{
    return d->view;
}

QWidget* ScreenplayParametersManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
