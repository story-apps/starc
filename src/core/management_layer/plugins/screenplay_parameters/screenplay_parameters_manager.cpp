#include "screenplay_parameters_manager.h"

#include "screenplay_parameters_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <ui/widgets/task_bar/task_bar.h>

#include <QApplication>
#include <QFileDialog>
#include <QTimeLine>


namespace ManagementLayer {

class ScreenplayParametersManager::Implementation
{
public:
    explicit Implementation(ScreenplayParametersManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayParametersView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ScreenplayParametersView* _view);


    ScreenplayParametersManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayParametersView* view = nullptr;
    Ui::ScreenplayParametersView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ScreenplayParametersView> view;
        QPointer<BusinessLayer::ScreenplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

ScreenplayParametersManager::Implementation::Implementation(ScreenplayParametersManager* _q)
    : q(_q)
{
}

Ui::ScreenplayParametersView* ScreenplayParametersManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ScreenplayParametersView;
    setModelForView(_model, view);

    connect(view, &Ui::ScreenplayParametersView::isScenesNumberingLockedChanged, q,
            [this](bool _locked) {
                const auto taskId = QUuid::createUuid().toString();
                TaskBar::addTask(taskId);
                TaskBar::setTaskTitle(
                    taskId, _locked ? tr("Scene numbers locked") : tr("Scene numbers unlocked"));
                auto timeline = new QTimeLine(3600, q);
                timeline->setEasingCurve(QEasingCurve::OutQuad);
                connect(timeline, &QTimeLine::valueChanged, q,
                        [taskId](qreal _value) { TaskBar::setTaskProgress(taskId, _value * 100); });
                connect(timeline, &QTimeLine::finished, q,
                        [taskId] { TaskBar::finishTask(taskId); });
                connect(timeline, &QTimeLine::finished, timeline, &QTimeLine::deleteLater);
                timeline->start();
            });

    return view;
}

void ScreenplayParametersManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplayParametersView* _view)
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
    auto model = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(_model);

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
        _view->setScenesNumbersTemplate(model->scenesNumbersTemplate());
        _view->setScenesNumberingStartAt(model->scenesNumberingStartAt());
        _view->setScenesNumbersLocked(model->isSceneNumbersLocked());
        _view->setOverrideCommonSettings(model->overrideCommonSettings());
        _view->setScreenplayTemplate(model->templateId());
        _view->setShowSceneNumbers(model->showSceneNumbers());
        _view->setShowSceneNumbersOnLeft(model->showSceneNumbersOnLeft());
        _view->setShowSceneNumbersOnRight(model->showSceneNumbersOnRight());
        _view->setShowDialoguesNumbers(model->showDialoguesNumbers());

        connect(model, &BusinessLayer::ScreenplayInformationModel::headerChanged, _view,
                &Ui::ScreenplayParametersView::setHeader);
        connect(model, &BusinessLayer::ScreenplayInformationModel::printHeaderOnTitlePageChanged,
                _view, &Ui::ScreenplayParametersView::setPrintHeaderOnTitlePage);
        connect(model, &BusinessLayer::ScreenplayInformationModel::footerChanged, _view,
                &Ui::ScreenplayParametersView::setFooter);
        connect(model, &BusinessLayer::ScreenplayInformationModel::printFooterOnTitlePageChanged,
                _view, &Ui::ScreenplayParametersView::setPrintFooterOnTitlePage);
        connect(model, &BusinessLayer::ScreenplayInformationModel::scenesNumbersTemplateChanged,
                _view, &Ui::ScreenplayParametersView::setScenesNumbersTemplate);
        connect(model, &BusinessLayer::ScreenplayInformationModel::scenesNumberingStartAtChanged,
                _view, &Ui::ScreenplayParametersView::setScenesNumberingStartAt);
        connect(model, &BusinessLayer::ScreenplayInformationModel::isSceneNumbersLockedChanged,
                _view, &Ui::ScreenplayParametersView::setScenesNumbersLocked);
        connect(model, &BusinessLayer::ScreenplayInformationModel::overrideCommonSettingsChanged,
                _view, &Ui::ScreenplayParametersView::setOverrideCommonSettings);
        connect(model, &BusinessLayer::ScreenplayInformationModel::templateIdChanged, _view,
                &Ui::ScreenplayParametersView::setScreenplayTemplate);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showSceneNumbersChanged, _view,
                &Ui::ScreenplayParametersView::setShowSceneNumbers);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showSceneNumbersOnLeftChanged,
                _view, &Ui::ScreenplayParametersView::setShowSceneNumbersOnLeft);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showSceneNumbersOnRightChanged,
                _view, &Ui::ScreenplayParametersView::setShowSceneNumbersOnRight);
        connect(model, &BusinessLayer::ScreenplayInformationModel::showDialoguesNumbersChanged,
                _view, &Ui::ScreenplayParametersView::setShowDialoguesNumbers);
        //
        connect(_view, &Ui::ScreenplayParametersView::headerChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setHeader);
        connect(_view, &Ui::ScreenplayParametersView::printHeaderOnTitlePageChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setPrintHeaderOnTitlePage);
        connect(_view, &Ui::ScreenplayParametersView::footerChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setFooter);
        connect(_view, &Ui::ScreenplayParametersView::printFooterOnTitlePageChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setPrintFooterOnTitlePage);
        connect(_view, &Ui::ScreenplayParametersView::scenesNumbersTemplateChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScenesNumbersTemplate);
        connect(_view, &Ui::ScreenplayParametersView::scenesNumberingStartAtChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScenesNumberingStartAt);
        connect(_view, &Ui::ScreenplayParametersView::isScenesNumberingLockedChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScenesNumbersLocked);
        connect(_view, &Ui::ScreenplayParametersView::overrideCommonSettingsChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setOverrideCommonSettings);
        connect(_view, &Ui::ScreenplayParametersView::screenplayTemplateChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setTemplateId);
        connect(_view, &Ui::ScreenplayParametersView::showSceneNumbersChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowSceneNumbers);
        connect(_view, &Ui::ScreenplayParametersView::showSceneNumbersOnLeftChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowSceneNumbersOnLeft);
        connect(_view, &Ui::ScreenplayParametersView::showSceneNumbersOnRightChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowSceneNumbersOnRight);
        connect(_view, &Ui::ScreenplayParametersView::showDialoguesNumbersChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setShowDialoguesNumbers);
    }
}


// ****


ScreenplayParametersManager::ScreenplayParametersManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

ScreenplayParametersManager::~ScreenplayParametersManager() = default;

Ui::IDocumentView* ScreenplayParametersManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayParametersManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ScreenplayParametersManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayParametersManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayParametersManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ScreenplayParametersManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ScreenplayParametersManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
