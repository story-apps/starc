#include "audioplay_statistics_structure_manager.h"

#include "audioplay_statistics_structure_view.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/audioplay_statistics_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>


namespace ManagementLayer {

class AudioplayStatisticsStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::AudioplayStatisticsStructureView* createView();


    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::AudioplayStatisticsModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::AudioplayStatisticsStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::AudioplayStatisticsStructureView*> allViews;
};

AudioplayStatisticsStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::AudioplayStatisticsStructureView* AudioplayStatisticsStructureManager::Implementation::
    createView()
{
    allViews.append(new Ui::AudioplayStatisticsStructureView);
    return allViews.last();
}


// ****


AudioplayStatisticsStructureManager::AudioplayStatisticsStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::AudioplayStatisticsStructureView::currentReportIndexChanged, this,
            &AudioplayStatisticsStructureManager::currentReportIndexChanged);
    connect(d->view, &Ui::AudioplayStatisticsStructureView::currentPlotIndexChanged, this,
            &AudioplayStatisticsStructureManager::currentPlotIndexChanged);
}

AudioplayStatisticsStructureManager::~AudioplayStatisticsStructureManager() = default;

QObject* AudioplayStatisticsStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* AudioplayStatisticsStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* AudioplayStatisticsStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* AudioplayStatisticsStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* AudioplayStatisticsStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* AudioplayStatisticsStructureManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void AudioplayStatisticsStructureManager::resetModels()
{
    setModel(nullptr);
}

void AudioplayStatisticsStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr && d->model->textModel() != nullptr
        && d->model->textModel()->informationModel() != nullptr) {
        disconnect(d->model->textModel()->informationModel(),
                   &BusinessLayer::AudioplayInformationModel::nameChanged, d->view, nullptr);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::AudioplayStatisticsModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr && d->model->textModel() != nullptr
        && d->model->textModel()->informationModel() != nullptr) {
        auto updateTitle = [this] {
            d->view->setTitle(QString("%1 | %2").arg(
                tr("Statistics"), d->model->textModel()->informationModel()->name()));
        };
        updateTitle();
        connect(d->model->textModel()->informationModel(),
                &BusinessLayer::AudioplayInformationModel::nameChanged, d->view, updateTitle);
    }
}

} // namespace ManagementLayer
