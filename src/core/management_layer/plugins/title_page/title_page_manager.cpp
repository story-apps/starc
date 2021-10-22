#include "title_page_manager.h"

#include "title_page_view.h"

#include <business_layer/model/text/text_model.h>
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
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class TitlePageManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::TitlePageView* createView();

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadViewSettings();
    void saveViewSettings();

    /**
     * @brief Работа с параметрами отображения текущей модели
     */
    void loadModelSettings();
    void saveModelSettings();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::TextModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::TitlePageView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::TitlePageView*> allViews;
};

TitlePageManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::TitlePageView* TitlePageManager::Implementation::createView()
{
    allViews.append(new Ui::TitlePageView);
    return allViews.last();
}

void TitlePageManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void TitlePageManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void TitlePageManager::Implementation::loadModelSettings()
{
    using namespace DataStorageLayer;
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void TitlePageManager::Implementation::saveModelSettings()
{
    using namespace DataStorageLayer;
    StorageFacade::settingsStorage()->setValue(cursorPositionFor(model->document()),
                                               view->cursorPosition(),
                                               SettingsStorage::SettingsPlace::Application);
}


// ****


TitlePageManager::TitlePageManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

TitlePageManager::~TitlePageManager() = default;

void TitlePageManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Если модель была задана
    //
    if (d->model != nullptr) {
        //
        // ... сохраняем её параметры
        //
        d->saveModelSettings();
        //
        // ... разрываем соединения
        //
        d->view->disconnect(d->model);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::TextModel*>(_model);
    d->view->setModel(d->model);

    //
    // Если новая модель задана
    //
    if (d->model != nullptr) {
        //
        // ... загрузим параметры
        //
        d->loadModelSettings();
    }
}

QWidget* TitlePageManager::view()
{
    return d->view;
}

QWidget* TitlePageManager::createView()
{
    return d->createView();
}

void TitlePageManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void TitlePageManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

} // namespace ManagementLayer
