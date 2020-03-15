#include "screenplay_text_manager.h"

#include "ui/screenplay_text_view.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer
{

class ScreenplayTextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTextView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::ScreenplayTextModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTextView*> allViews;
};

ScreenplayTextManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayTextView* ScreenplayTextManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayTextView);
    return allViews.last();
}


// ****


ScreenplayTextManager::ScreenplayTextManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayTextView::currentModelIndexChanged,
            this, &ScreenplayTextManager::currentModelIndexChanged);
}

ScreenplayTextManager::~ScreenplayTextManager() = default;

QObject* ScreenplayTextManager::asQObject()
{
    return this;
}

void ScreenplayTextManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->view->setModel(d->model);

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

QWidget* ScreenplayTextManager::view()
{
    return d->view;
}

QWidget* ScreenplayTextManager::createView()
{
    return d->createView();
}

void ScreenplayTextManager::reconfigure()
{
    d->view->reconfigure();
}

void ScreenplayTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)),
            this, SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);
}

void ScreenplayTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
