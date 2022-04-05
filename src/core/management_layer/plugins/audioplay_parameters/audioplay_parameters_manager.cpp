#include "audioplay_parameters_manager.h"

#include "audioplay_parameters_view.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class AudioplayParametersManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::AudioplayParametersView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::AudioplayInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::AudioplayParametersView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::AudioplayParametersView*> allViews;
};

AudioplayParametersManager::Implementation::Implementation()
{
    view = createView();
}

Ui::AudioplayParametersView* AudioplayParametersManager::Implementation::createView()
{
    allViews.append(new Ui::AudioplayParametersView);
    return allViews.last();
}


// ****


AudioplayParametersManager::AudioplayParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

AudioplayParametersManager::~AudioplayParametersManager() = default;

void AudioplayParametersManager::setModel(BusinessLayer::AbstractModel* _model)
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
        d->view->setHeader(d->model->header());
        d->view->setPrintHeaderOnTitlePage(d->model->printHeaderOnTitlePage());
        d->view->setFooter(d->model->footer());
        d->view->setPrintFooterOnTitlePage(d->model->printFooterOnTitlePage());
        d->view->setOverrideCommonSettings(d->model->overrideCommonSettings());
        d->view->setAudioplayTemplate(d->model->templateId());
        d->view->setShowBlockNumbers(d->model->showBlockNumbers());
        d->view->setContinueBlockNumbers(d->model->continueBlockNumbers());

        connect(d->model, &BusinessLayer::AudioplayInformationModel::headerChanged, d->view,
                &Ui::AudioplayParametersView::setHeader);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::printHeaderOnTitlePageChanged,
                d->view, &Ui::AudioplayParametersView::setPrintHeaderOnTitlePage);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::footerChanged, d->view,
                &Ui::AudioplayParametersView::setFooter);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::printFooterOnTitlePageChanged,
                d->view, &Ui::AudioplayParametersView::setPrintFooterOnTitlePage);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::overrideCommonSettingsChanged,
                d->view, &Ui::AudioplayParametersView::setOverrideCommonSettings);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::templateIdChanged, d->view,
                &Ui::AudioplayParametersView::setAudioplayTemplate);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::showBlockNumbersChanged,
                d->view, &Ui::AudioplayParametersView::setShowBlockNumbers);
        connect(d->model, &BusinessLayer::AudioplayInformationModel::continueBlockNumbersChanged,
                d->view, &Ui::AudioplayParametersView::setContinueBlockNumbers);
        //
        connect(d->view, &Ui::AudioplayParametersView::headerChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setHeader);
        connect(d->view, &Ui::AudioplayParametersView::printHeaderOnTitlePageChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setPrintHeaderOnTitlePage);
        connect(d->view, &Ui::AudioplayParametersView::footerChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setFooter);
        connect(d->view, &Ui::AudioplayParametersView::printFooterOnTitlePageChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setPrintFooterOnTitlePage);
        connect(d->view, &Ui::AudioplayParametersView::overrideCommonSettingsChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setOverrideCommonSettings);
        connect(d->view, &Ui::AudioplayParametersView::audioplayTemplateChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setTemplateId);
        connect(d->view, &Ui::AudioplayParametersView::showBlockNumbersChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setShowBlockNumbers);
        connect(d->view, &Ui::AudioplayParametersView::continueBlockNumbersChanged, d->model,
                &BusinessLayer::AudioplayInformationModel::setContinueBlockNumbers);
    }
}

Ui::IDocumentView* AudioplayParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* AudioplayParametersManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer
