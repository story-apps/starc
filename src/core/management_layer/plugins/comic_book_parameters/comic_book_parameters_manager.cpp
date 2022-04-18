#include "comic_book_parameters_manager.h"

#include "comic_book_parameters_view.h"

#include <business_layer/model/comic_book/comic_book_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ComicBookParametersManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ComicBookParametersView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::ComicBookInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ComicBookParametersView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ComicBookParametersView*> allViews;
};

ComicBookParametersManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ComicBookParametersView* ComicBookParametersManager::Implementation::createView()
{
    allViews.append(new Ui::ComicBookParametersView);
    return allViews.last();
}


// ****


ComicBookParametersManager::ComicBookParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ComicBookParametersManager::~ComicBookParametersManager() = default;

void ComicBookParametersManager::setModel(BusinessLayer::AbstractModel* _model)
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
        d->view->setHeader(d->model->header());
        d->view->setPrintHeaderOnTitlePage(d->model->printHeaderOnTitlePage());
        d->view->setFooter(d->model->footer());
        d->view->setPrintFooterOnTitlePage(d->model->printFooterOnTitlePage());
        d->view->setOverrideCommonSettings(d->model->overrideCommonSettings());
        d->view->setComicBookTemplate(d->model->templateId());

        connect(d->model, &BusinessLayer::ComicBookInformationModel::headerChanged, d->view,
                &Ui::ComicBookParametersView::setHeader);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::printHeaderOnTitlePageChanged,
                d->view, &Ui::ComicBookParametersView::setPrintHeaderOnTitlePage);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::footerChanged, d->view,
                &Ui::ComicBookParametersView::setFooter);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::printFooterOnTitlePageChanged,
                d->view, &Ui::ComicBookParametersView::setPrintFooterOnTitlePage);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::overrideCommonSettingsChanged,
                d->view, &Ui::ComicBookParametersView::setOverrideCommonSettings);
        connect(d->model, &BusinessLayer::ComicBookInformationModel::templateIdChanged, d->view,
                &Ui::ComicBookParametersView::setComicBookTemplate);
        //
        connect(d->view, &Ui::ComicBookParametersView::headerChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setHeader);
        connect(d->view, &Ui::ComicBookParametersView::printHeaderOnTitlePageChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setPrintHeaderOnTitlePage);
        connect(d->view, &Ui::ComicBookParametersView::footerChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setFooter);
        connect(d->view, &Ui::ComicBookParametersView::printFooterOnTitlePageChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setPrintFooterOnTitlePage);
        connect(d->view, &Ui::ComicBookParametersView::overrideCommonSettingsChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setOverrideCommonSettings);
        connect(d->view, &Ui::ComicBookParametersView::comicBookTemplateChanged, d->model,
                &BusinessLayer::ComicBookInformationModel::setTemplateId);
    }
}

Ui::IDocumentView* ComicBookParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* ComicBookParametersManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
