#include "audioplay_parameters_manager.h"

#include "audioplay_parameters_view.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class AudioplayParametersManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::AudioplayParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::AudioplayParametersView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::AudioplayParametersView* view = nullptr;
    Ui::AudioplayParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::AudioplayParametersView> view;
        QPointer<BusinessLayer::AudioplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::AudioplayParametersView* AudioplayParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::AudioplayParametersView;
    setModelForView(_model, view);
    return view;
}

void AudioplayParametersManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::AudioplayParametersView* _view)
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
        _view->setHeader(model->header());
        _view->setPrintHeaderOnTitlePage(model->printHeaderOnTitlePage());
        _view->setFooter(model->footer());
        _view->setPrintFooterOnTitlePage(model->printFooterOnTitlePage());
        _view->setOverrideCommonSettings(model->overrideCommonSettings());
        _view->setAudioplayTemplate(model->templateId());
        _view->setShowBlockNumbers(model->showBlockNumbers());
        _view->setContinueBlockNumbers(model->continueBlockNumbers());

        connect(model, &BusinessLayer::AudioplayInformationModel::headerChanged, _view,
                &Ui::AudioplayParametersView::setHeader);
        connect(model, &BusinessLayer::AudioplayInformationModel::printHeaderOnTitlePageChanged,
                _view, &Ui::AudioplayParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::AudioplayInformationModel::footerChanged, _view,
                &Ui::AudioplayParametersView::setFooter);
        connect(model, &BusinessLayer::AudioplayInformationModel::printFooterOnTitlePageChanged,
                _view, &Ui::AudioplayParametersView::setPrintFooterOnTitlePage);
        connect(model, &BusinessLayer::AudioplayInformationModel::overrideCommonSettingsChanged,
                _view, &Ui::AudioplayParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::AudioplayInformationModel::templateIdChanged, _view,
                &Ui::AudioplayParametersView::setAudioplayTemplate);
        connect(model, &BusinessLayer::AudioplayInformationModel::showBlockNumbersChanged, _view,
                &Ui::AudioplayParametersView::setShowBlockNumbers);
        connect(model, &BusinessLayer::AudioplayInformationModel::continueBlockNumbersChanged,
                _view, &Ui::AudioplayParametersView::setContinueBlockNumbers);
        //
        connect(_view, &Ui::AudioplayParametersView::headerChanged, model,
                &BusinessLayer::AudioplayInformationModel::setHeader);
        connect(_view, &Ui::AudioplayParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::AudioplayInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::AudioplayParametersView::footerChanged, model,
                &BusinessLayer::AudioplayInformationModel::setFooter);
        connect(_view, &Ui::AudioplayParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::AudioplayInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::AudioplayParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::AudioplayInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::AudioplayParametersView::audioplayTemplateChanged, model,
                &BusinessLayer::AudioplayInformationModel::setTemplateId);
        connect(_view, &Ui::AudioplayParametersView::showBlockNumbersChanged, model,
                &BusinessLayer::AudioplayInformationModel::setShowBlockNumbers);
        connect(_view, &Ui::AudioplayParametersView::continueBlockNumbersChanged, model,
                &BusinessLayer::AudioplayInformationModel::setContinueBlockNumbers);
    }
}


// ****


AudioplayParametersManager::AudioplayParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

AudioplayParametersManager::~AudioplayParametersManager() = default;

Ui::IDocumentView* AudioplayParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* AudioplayParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* AudioplayParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* AudioplayParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* AudioplayParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void AudioplayParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void AudioplayParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
