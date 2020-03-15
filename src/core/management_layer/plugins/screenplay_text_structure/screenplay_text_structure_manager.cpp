#include "screenplay_text_structure_manager.h"

#include "business_layer/screenplay_text_structure_model.h"
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
     * @brief Текущая модель сценария
     */
    BusinessLayer::ScreenplayTextModel* model = nullptr;

    /**
     * @brief Модель отображения структуры сценария
     */
    BusinessLayer::ScreenplayTextStructureModel* structureModel = nullptr;

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
    connect(d->view, &Ui::ScreenplayTextStructureView::currentModelIndexChanged, this,
            [this] (const QModelIndex& _index) {
        emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
    });
}

ScreenplayTextStructureManager::~ScreenplayTextStructureManager() = default;

QObject* ScreenplayTextStructureManager::asQObject()
{
    return this;
}

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
    // Помещаем модель в прокси
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::ScreenplayTextStructureModel(this);
        d->view->setModel(d->structureModel);
    }
    d->structureModel->setSourceModel(d->model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
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

void ScreenplayTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)),
            this, SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);
}

void ScreenplayTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    QSignalBlocker signalBlocker(this);

    //
    // Из редактора сценария мы получаем индексы текстовых элементов, они хранятся внутри
    // сцен или папок, которые как раз и отображаются в навигаторе
    //
    d->view->setCurrentModelIndex(d->structureModel->mapFromSource(_index.parent()));
}

} // namespace ManagementLayer
