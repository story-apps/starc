#include "comic_book_information_manager.h"

#include "comic_book_information_view.h"

#include <business_layer/model/comic_book/comic_book_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ComicBookInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::ComicBookInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ComicBookInformationView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::ComicBookInformationView* view = nullptr;
    Ui::ComicBookInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ComicBookInformationView> view;
        QPointer<BusinessLayer::ComicBookInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::ComicBookInformationView* ComicBookInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ComicBookInformationView;
    setModelForView(_model, view);
    return view;
}

void ComicBookInformationManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ComicBookInformationView* _view)
{
    constexpr int invalidIndex = -1;
    int viewIndex = invalidIndex;
    for (int index = 0; index < allViews.size(); ++index) {
        if (allViews[index].view == _view) {
            if (allViews[index].model == _model) {
                return;
            }

            viewIndex = index;
            break;
        }
    }

    //
    // Разрываем соединения со старой моделью
    //
    if (viewIndex != invalidIndex && allViews[viewIndex].model != nullptr) {
        _view->disconnect(allViews[viewIndex].model);
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::ComicBookInformationModel*>(_model);

    //
    // Обновляем связь представления с моделью
    //
    if (viewIndex != invalidIndex) {
        allViews[viewIndex].model = model;
    }
    //
    // Или сохраняем связь представления с моделью
    //
    else {
        allViews.append({ _view, model });
    }

    //
    // Настраиваем соединения с новой моделью
    //
    if (model != nullptr) {
        _view->setName(model->name());
        _view->setTagline(model->tagline());
        _view->setLogline(model->logline());
        _view->setTitlePageVisible(model->titlePageVisible());
        _view->setSynopsisVisible(model->synopsisVisible());
        _view->setComicBookTextVisible(model->comicBookTextVisible());
        _view->setComicBookStatisticsVisible(model->comicBookStatisticsVisible());

        connect(model, &BusinessLayer::ComicBookInformationModel::nameChanged, _view,
                &Ui::ComicBookInformationView::setName);
        connect(model, &BusinessLayer::ComicBookInformationModel::taglineChanged, _view,
                &Ui::ComicBookInformationView::setTagline);
        connect(model, &BusinessLayer::ComicBookInformationModel::loglineChanged, _view,
                &Ui::ComicBookInformationView::setLogline);
        connect(model, &BusinessLayer::ComicBookInformationModel::titlePageVisibleChanged, _view,
                &Ui::ComicBookInformationView::setTitlePageVisible);
        connect(model, &BusinessLayer::ComicBookInformationModel::synopsisVisibleChanged, _view,
                &Ui::ComicBookInformationView::setSynopsisVisible);
        connect(model, &BusinessLayer::ComicBookInformationModel::comicBookTextVisibleChanged,
                _view, &Ui::ComicBookInformationView::setComicBookTextVisible);
        connect(model, &BusinessLayer::ComicBookInformationModel::comicBookStatisticsVisibleChanged,
                _view, &Ui::ComicBookInformationView::setComicBookStatisticsVisible);
        //
        connect(_view, &Ui::ComicBookInformationView::nameChanged, model,
                &BusinessLayer::ComicBookInformationModel::setName);
        connect(_view, &Ui::ComicBookInformationView::taglineChanged, model,
                &BusinessLayer::ComicBookInformationModel::setTagline);
        connect(_view, &Ui::ComicBookInformationView::loglineChanged, model,
                &BusinessLayer::ComicBookInformationModel::setLogline);
        connect(_view, &Ui::ComicBookInformationView::titlePageVisibleChanged, model,
                &BusinessLayer::ComicBookInformationModel::setTitlePageVisible);
        connect(_view, &Ui::ComicBookInformationView::synopsisVisibleChanged, model,
                &BusinessLayer::ComicBookInformationModel::setSynopsisVisible);
        connect(_view, &Ui::ComicBookInformationView::comicBookTextVisibleChanged, model,
                &BusinessLayer::ComicBookInformationModel::setComicBookTextVisible);
        connect(_view, &Ui::ComicBookInformationView::comicBookStatisticsVisibleChanged, model,
                &BusinessLayer::ComicBookInformationModel::setComicBookStatisticsVisible);
    }
}


// ****


ComicBookInformationManager::ComicBookInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ComicBookInformationManager::~ComicBookInformationManager() = default;

Ui::IDocumentView* ComicBookInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* ComicBookInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ComicBookInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ComicBookInformationManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ComicBookInformationManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ComicBookInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ComicBookInformationManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
