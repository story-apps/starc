#include "audioplay_information_manager.h"

#include "audioplay_information_view.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class AudioplayInformationManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::AudioplayInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::AudioplayInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::AudioplayInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::AudioplayInformationView*> allViews;
};

AudioplayInformationManager::Implementation::Implementation()
{
    view = createView();
}

Ui::AudioplayInformationView* AudioplayInformationManager::Implementation::createView()
{
    allViews.append(new Ui::AudioplayInformationView);
    return allViews.last();
}


// ****


AudioplayInformationManager::AudioplayInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

AudioplayInformationManager::~AudioplayInformationManager() = default;

void AudioplayInformationManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::AudioplayInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setTagline(d->model->tagline());
        d->view->setLogline(d->model->logline());
        d->view->setTitlePageVisible(d->model->titlePageVisible());
        d->view->setSynopsisVisible(d->model->synopsisVisible());
        d->view->setAudioplayTextVisible(d->model->audioplayTextVisible());
        d->view->setAudioplayStatisticsVisible(d->model->audioplayStatisticsVisible());

        connect(d->model, &BusinessLayer::AudioplayInformationModel::nameChanged, d->view,
                &Ui::AudioplayInformationView::setName);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::taglineChanged, d->view,
                &Ui::AudioplayInformationView::setTagline);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::loglineChanged, d->view,
                &Ui::AudioplayInformationView::setLogline);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::titlePageVisibleChanged,
                d->view, &Ui::AudioplayInformationView::setTitlePageVisible);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::synopsisVisibleChanged,
                d->view, &Ui::AudioplayInformationView::setSynopsisVisible);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::audioplayTextVisibleChanged,
                d->view, &Ui::AudioplayInformationView::setAudioplayTextVisible);
        connect(d->model,
                &BusinessLayer::AudioplayInformationModel::audioplayStatisticsVisibleChanged,
                d->view, &Ui::AudioplayInformationView::setAudioplayStatisticsVisible);
        //
        connect(d->view, &Ui::AudioplayInformationView::nameChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setName);
        connect(d->view, &Ui::AudioplayInformationView::taglineChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setTagline);
        connect(d->view, &Ui::AudioplayInformationView::loglineChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setLogline);
        connect(d->view, &Ui::AudioplayInformationView::titlePageVisibleChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setTitlePageVisible);
        connect(d->view, &Ui::AudioplayInformationView::synopsisVisibleChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setSynopsisVisible);
        connect(d->view, &Ui::AudioplayInformationView::audioplayTextVisibleChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setAudioplayTextVisible);
        connect(d->view, &Ui::AudioplayInformationView::audioplayStatisticsVisibleChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setAudioplayStatisticsVisible);
    }
}

Ui::IDocumentView* AudioplayInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* AudioplayInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
