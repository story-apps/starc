#include "title_page_manager.h"

#include "title_page_view.h"

#include <business_layer/model/simple_text/simple_text_model.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QApplication>
#include <QFileDialog>
#include <QTimer>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "simple-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    if (_item == nullptr) {
        return {};
    }

    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class TitlePageManager::Implementation
{
public:
    explicit Implementation(TitlePageManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::TitlePageView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::TitlePageView* _view);

    /**
     * @brief Получить модель связанную с заданным представлением
     */
    QPointer<BusinessLayer::SimpleTextModel> modelForView(Ui::TitlePageView* _view) const;

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadModelAndViewSettings(BusinessLayer::AbstractModel* _model, Ui::TitlePageView* _view);
    void saveModelAndViewSettings(BusinessLayer::AbstractModel* _model, Ui::TitlePageView* _view);


    TitlePageManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::TitlePageView* view = nullptr;
    Ui::TitlePageView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::TitlePageView> view;
        QPointer<BusinessLayer::SimpleTextModel> model;
    };
    QVector<ViewAndModel> allViews;
};

TitlePageManager::Implementation::Implementation(TitlePageManager* _q)
    : q(_q)
{
}

Ui::TitlePageView* TitlePageManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::TitlePageView;
    setModelForView(_model, view);
    return view;
}

void TitlePageManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                       Ui::TitlePageView* _view)
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
    // Если модель была задана
    //
    if (viewIndex != invalidIndex && allViews[viewIndex].model != nullptr) {
        //
        // ... сохраняем параметры
        //
        saveModelAndViewSettings(allViews[viewIndex].model, _view);
        //
        // ... разрываем соединения
        //
        _view->disconnect(allViews[viewIndex].model);
        allViews[viewIndex].model->disconnect(_view);
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::SimpleTextModel*>(_model);
    _view->setModel(model);

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
    // Если новая модель задана
    //
    if (model != nullptr) {
        //
        // ... загрузим параметры
        //
        loadModelAndViewSettings(model, _view);
        //
        // ... настраиваем соединения
        //
    }
}

QPointer<BusinessLayer::SimpleTextModel> TitlePageManager::Implementation::modelForView(
    Ui::TitlePageView* _view) const
{
    for (auto& viewAndModel : allViews) {
        if (viewAndModel.view == _view) {
            return viewAndModel.model;
        }
    }
    return {};
}

void TitlePageManager::Implementation::loadModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::TitlePageView* _view)
{
    const auto cursorPosition = settingsValue(cursorPositionFor(_model->document()), 0).toInt();
    _view->setCursorPosition(cursorPosition);

    _view->loadViewSettings();
}

void TitlePageManager::Implementation::saveModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::TitlePageView* _view)
{
    setSettingsValue(cursorPositionFor(_model->document()), _view->cursorPosition());

    _view->saveViewSettings();
}


// ****


TitlePageManager::TitlePageManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

TitlePageManager::~TitlePageManager() = default;

Ui::IDocumentView* TitlePageManager::view()
{
    return d->view;
}

Ui::IDocumentView* TitlePageManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* TitlePageManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* TitlePageManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* TitlePageManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void TitlePageManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void TitlePageManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    for (const auto& view : std::as_const(d->allViews)) {
        if (!view.view.isNull()) {
            view.view->reconfigure(_changedSettingsKeys);
        }
    }
}

void TitlePageManager::saveSettings()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.model.isNull() || viewAndModel.view.isNull()) {
            continue;
        }

        d->saveModelAndViewSettings(viewAndModel.model, viewAndModel.view);
    }
}

void TitlePageManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer
