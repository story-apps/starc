#include "screenplay_text_structure_manager.h"

#include "ui/screenplay_text_structure_view.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer
{

class ScreenplayTextStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTextStructureView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::ScreenplayTextModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTextStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTextStructureView*> allViews;
};

ScreenplayTextStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayTextStructureView* ScreenplayTextStructureManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayTextStructureView);
    return allViews.last();
}


// ****


ScreenplayTextStructureManager::ScreenplayTextStructureManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

ScreenplayTextStructureManager::~ScreenplayTextStructureManager() = default;

void ScreenplayTextStructureManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ScreenplayTextModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setModel(d->model);
//        d->view->setName(d->model->name());
//        d->view->setText(d->model->text());

//        connect(d->model, &BusinessLayer::ScreenplayTextModel::nameChanged,
//                d->view, &Ui::ScreenplayTextView::setName);
//        connect(d->model, &BusinessLayer::ScreenplayTextModel::textChanged,
//                d->view, &Ui::ScreenplayTextView::setText);
//        //
//        connect(d->view, &Ui::ScreenplayTextView::nameChanged,
//                d->model, &BusinessLayer::ScreenplayTextModel::setName);
//        connect(d->view, &Ui::ScreenplayTextView::textChanged,
//                d->model, &BusinessLayer::ScreenplayTextModel::setText);
    }
}

QWidget* ScreenplayTextStructureManager::view()
{
    return d->view;
}

QWidget* ScreenplayTextStructureManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
