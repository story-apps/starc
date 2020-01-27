#include "text_manager.h"

#include "text_view.h"

#include <business_layer/model/text/text_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer
{

class TextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::TextView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::TextModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::TextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::TextView*> allViews;
};

TextManager::Implementation::Implementation()
{
    view = createView();
}

Ui::TextView* TextManager::Implementation::createView()
{
    allViews.append(new Ui::TextView);
    return allViews.last();
}


// ****


TextManager::TextManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

TextManager::~TextManager() = default;

void TextManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::TextModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setText(d->model->text());

        connect(d->model, &BusinessLayer::TextModel::nameChanged,
                d->view, &Ui::TextView::setName);
        connect(d->model, &BusinessLayer::TextModel::textChanged,
                d->view, &Ui::TextView::setText);
        //
        connect(d->view, &Ui::TextView::nameChanged,
                d->model, &BusinessLayer::TextModel::setName);
        connect(d->view, &Ui::TextView::textChanged,
                d->model, &BusinessLayer::TextModel::setText);
    }
}

QWidget* TextManager::view()
{
    return d->view;
}

QWidget* TextManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
