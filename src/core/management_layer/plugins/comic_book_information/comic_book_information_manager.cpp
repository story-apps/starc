#include "comic_book_information_manager.h"

#include "comic_book_information_view.h"

#include <business_layer/model/comic_book/comic_book_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ComicBookInformationManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ComicBookInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::ComicBookInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ComicBookInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ComicBookInformationView*> allViews;
};

ComicBookInformationManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ComicBookInformationView* ComicBookInformationManager::Implementation::createView()
{
    allViews.append(new Ui::ComicBookInformationView);
    return allViews.last();
}


// ****


ComicBookInformationManager::ComicBookInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ComicBookInformationManager::~ComicBookInformationManager() = default;

void ComicBookInformationManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ComicBookInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setTagline(d->model->tagline());
        d->view->setLogline(d->model->logline());
        d->view->setTitlePageVisible(d->model->titlePageVisible());
        d->view->setSynopsisVisible(d->model->synopsisVisible());
        d->view->setComicBookTextVisible(d->model->comicBookTextVisible());
        d->view->setComicBookStatisticsVisible(d->model->comicBookStatisticsVisible());

        connect(d->model, &BusinessLayer::ComicBookInformationModel::nameChanged, d->view,
                &Ui::ComicBookInformationView::setName);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::taglineChanged, d->view,
                &Ui::ComicBookInformationView::setTagline);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::loglineChanged, d->view,
                &Ui::ComicBookInformationView::setLogline);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::titlePageVisibleChanged,
                d->view, &Ui::ComicBookInformationView::setTitlePageVisible);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::synopsisVisibleChanged,
                d->view, &Ui::ComicBookInformationView::setSynopsisVisible);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::comicBookTextVisibleChanged,
                d->view, &Ui::ComicBookInformationView::setComicBookTextVisible);
        connect(d->model,
                &BusinessLayer::ComicBookInformationModel::comicBookStatisticsVisibleChanged,
                d->view, &Ui::ComicBookInformationView::setComicBookStatisticsVisible);
        //
        connect(d->view, &Ui::ComicBookInformationView::nameChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setName);
        connect(d->view, &Ui::ComicBookInformationView::taglineChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setTagline);
        connect(d->view, &Ui::ComicBookInformationView::loglineChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setLogline);
        connect(d->view, &Ui::ComicBookInformationView::titlePageVisibleChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setTitlePageVisible);
        connect(d->view, &Ui::ComicBookInformationView::synopsisVisibleChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setSynopsisVisible);
        connect(d->view, &Ui::ComicBookInformationView::comicBookTextVisibleChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setComicBookTextVisible);
        connect(d->view, &Ui::ComicBookInformationView::comicBookStatisticsVisibleChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setComicBookStatisticsVisible);
    }
}

Ui::IDocumentView* ComicBookInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* ComicBookInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
