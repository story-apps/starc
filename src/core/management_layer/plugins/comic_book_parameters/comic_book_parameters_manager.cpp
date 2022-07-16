#include "comic_book_parameters_manager.h"

#include "comic_book_parameters_view.h"

#include <business_layer/model/comic_book/comic_book_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ComicBookParametersManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::ComicBookParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ComicBookParametersView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::ComicBookParametersView* view = nullptr;
    Ui::ComicBookParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ComicBookParametersView> view;
        QPointer<BusinessLayer::ComicBookInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::ComicBookParametersView* ComicBookParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ComicBookParametersView;
    setModelForView(_model, view);
    return view;
}

void ComicBookParametersManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ComicBookParametersView* _view)
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
        allViews[viewIndex].model->disconnect(_view);
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
        _view->setHeader(model->header());
        _view->setPrintHeaderOnTitlePage(model->printHeaderOnTitlePage());
        _view->setFooter(model->footer());
        _view->setPrintFooterOnTitlePage(model->printFooterOnTitlePage());
        _view->setOverrideCommonSettings(model->overrideCommonSettings());
        _view->setComicBookTemplate(model->templateId());

        connect(model, &BusinessLayer::ComicBookInformationModel::headerChanged, _view,
                &Ui::ComicBookParametersView::setHeader);
        connect(model, &BusinessLayer::ComicBookInformationModel::printHeaderOnTitlePageChanged,
                _view, &Ui::ComicBookParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::ComicBookInformationModel::footerChanged, _view,
                &Ui::ComicBookParametersView::setFooter);
        connect(model, &BusinessLayer::ComicBookInformationModel::printFooterOnTitlePageChanged,
                _view, &Ui::ComicBookParametersView::setPrintFooterOnTitlePage);
        connect(model, &BusinessLayer::ComicBookInformationModel::overrideCommonSettingsChanged,
                _view, &Ui::ComicBookParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::ComicBookInformationModel::templateIdChanged, _view,
                &Ui::ComicBookParametersView::setComicBookTemplate);
        //
        connect(_view, &Ui::ComicBookParametersView::headerChanged, model,
                &BusinessLayer::ComicBookInformationModel::setHeader);
        connect(_view, &Ui::ComicBookParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::ComicBookInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::ComicBookParametersView::footerChanged, model,
                &BusinessLayer::ComicBookInformationModel::setFooter);
        connect(_view, &Ui::ComicBookParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::ComicBookInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::ComicBookParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::ComicBookInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::ComicBookParametersView::comicBookTemplateChanged, model,
                &BusinessLayer::ComicBookInformationModel::setTemplateId);
    }
}


// ****


ComicBookParametersManager::ComicBookParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ComicBookParametersManager::~ComicBookParametersManager() = default;

Ui::IDocumentView* ComicBookParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* ComicBookParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ComicBookParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ComicBookParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ComicBookParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ComicBookParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ComicBookParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
