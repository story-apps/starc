#include "audioplay_information_manager.h"

#include "audioplay_information_view.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class AudioplayInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::AudioplayInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::AudioplayInformationView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::AudioplayInformationView* view = nullptr;
    Ui::AudioplayInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        Ui::AudioplayInformationView* view = nullptr;
        QPointer<BusinessLayer::AudioplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::AudioplayInformationView* AudioplayInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::AudioplayInformationView;
    setModelForView(_model, view);
    return view;
}

void AudioplayInformationManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::AudioplayInformationView* _view)
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
    auto model = qobject_cast<BusinessLayer::AudioplayInformationModel*>(_model);

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
        _view->setAudioplayTextVisible(model->audioplayTextVisible());
        _view->setAudioplayStatisticsVisible(model->audioplayStatisticsVisible());

        connect(model, &BusinessLayer::AudioplayInformationModel::nameChanged, _view,
                &Ui::AudioplayInformationView::setName);
        connect(model, &BusinessLayer::AudioplayInformationModel::taglineChanged, _view,
                &Ui::AudioplayInformationView::setTagline);
        connect(model, &BusinessLayer::AudioplayInformationModel::loglineChanged, _view,
                &Ui::AudioplayInformationView::setLogline);
        connect(model, &BusinessLayer::AudioplayInformationModel::titlePageVisibleChanged, _view,
                &Ui::AudioplayInformationView::setTitlePageVisible);
        connect(model, &BusinessLayer::AudioplayInformationModel::synopsisVisibleChanged, _view,
                &Ui::AudioplayInformationView::setSynopsisVisible);
        connect(model, &BusinessLayer::AudioplayInformationModel::audioplayTextVisibleChanged,
                _view, &Ui::AudioplayInformationView::setAudioplayTextVisible);
        connect(model, &BusinessLayer::AudioplayInformationModel::audioplayStatisticsVisibleChanged,
                _view, &Ui::AudioplayInformationView::setAudioplayStatisticsVisible);
        //
        connect(_view, &Ui::AudioplayInformationView::nameChanged, model,
                &BusinessLayer::AudioplayInformationModel::setName);
        connect(_view, &Ui::AudioplayInformationView::taglineChanged, model,
                &BusinessLayer::AudioplayInformationModel::setTagline);
        connect(_view, &Ui::AudioplayInformationView::loglineChanged, model,
                &BusinessLayer::AudioplayInformationModel::setLogline);
        connect(_view, &Ui::AudioplayInformationView::titlePageVisibleChanged, model,
                &BusinessLayer::AudioplayInformationModel::setTitlePageVisible);
        connect(_view, &Ui::AudioplayInformationView::synopsisVisibleChanged, model,
                &BusinessLayer::AudioplayInformationModel::setSynopsisVisible);
        connect(_view, &Ui::AudioplayInformationView::audioplayTextVisibleChanged, model,
                &BusinessLayer::AudioplayInformationModel::setAudioplayTextVisible);
        connect(_view, &Ui::AudioplayInformationView::audioplayStatisticsVisibleChanged, model,
                &BusinessLayer::AudioplayInformationModel::setAudioplayStatisticsVisible);
    }
}


// ****


AudioplayInformationManager::AudioplayInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

AudioplayInformationManager::~AudioplayInformationManager() = default;

Ui::IDocumentView* AudioplayInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* AudioplayInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* AudioplayInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* AudioplayInformationManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* AudioplayInformationManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void AudioplayInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        d->setModelForView(nullptr, viewAndModel.view);
    }
}

} // namespace ManagementLayer
