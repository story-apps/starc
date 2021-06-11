#include "screenplay_text_structure_manager.h"

#include "business_layer/screenplay_text_structure_model.h"
#include "ui/screenplay_text_structure_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>


namespace ManagementLayer {

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
     * @brief Индекс модели, который необходимо выделить
     * @note Используется в случаях, когда в навигаторе установлена не та модель, что отображается
     *       в редакторе, когда будет установлена нужная модель, в навигаторе будет выделен элемент
     *       с данным индексом
     */
    QModelIndex modelIndexToSelect;

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
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
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
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::ScreenplayTextStructureModel(d->view);
        d->view->setModel(d->structureModel);
    }

    //
    // Помещаем модель с данными в прокси
    //
    d->structureModel->setSourceModel(d->model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setTitle(d->model->informationModel()->name());
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::nameChanged, d->view,
                &Ui::ScreenplayTextStructureView::setTitle);
    }

    //
    // Если элемент к выделению уже задан, выберем его в структуре
    //
    if (d->modelIndexToSelect.isValid()) {
        setCurrentModelIndex(d->modelIndexToSelect);
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

void ScreenplayTextStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);
    d->view->reconfigure();
}

void ScreenplayTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)), this,
            SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);
}

void ScreenplayTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    if (d->model != _index.model()) {
        d->modelIndexToSelect = _index;
        return;
    }

    QSignalBlocker signalBlocker(this);

    //
    // Из редактора сценария мы получаем индексы текстовых элементов, они хранятся внутри
    // сцен или папок, которые как раз и отображаются в навигаторе
    //
    d->view->setCurrentModelIndex(d->structureModel->mapFromSource(_index.parent()));
    d->modelIndexToSelect = {};
}

} // namespace ManagementLayer
