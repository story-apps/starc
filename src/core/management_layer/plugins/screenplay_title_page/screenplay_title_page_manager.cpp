#include "screenplay_title_page_manager.h"

#include "screenplay_title_page_view.h"

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

class ScreenplayTitlePageManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTitlePageView* createView();

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
    BusinessLayer::TextModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTitlePageView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTitlePageView*> allViews;
};

ScreenplayTitlePageManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::ScreenplayTitlePageView* ScreenplayTitlePageManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayTitlePageView);
    return allViews.last();
}

void ScreenplayTitlePageManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void ScreenplayTitlePageManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void ScreenplayTitlePageManager::Implementation::loadModelSettings()
{
    using namespace DataStorageLayer;
    const auto cursorPosition = StorageFacade::settingsStorage()
                                    ->value(cursorPositionFor(model->document()),
                                            SettingsStorage::SettingsPlace::Application, 0)
                                    .toInt();
    view->setCursorPosition(cursorPosition);
}

void ScreenplayTitlePageManager::Implementation::saveModelSettings()
{
    using namespace DataStorageLayer;
    StorageFacade::settingsStorage()->setValue(cursorPositionFor(model->document()),
                                               view->cursorPosition(),
                                               SettingsStorage::SettingsPlace::Application);
}


// ****


ScreenplayTitlePageManager::ScreenplayTitlePageManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ScreenplayTitlePageManager::~ScreenplayTitlePageManager() = default;

void ScreenplayTitlePageManager::setModel(BusinessLayer::AbstractModel* _model)
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

QWidget* ScreenplayTitlePageManager::view()
{
    return d->view;
}

QWidget* ScreenplayTitlePageManager::createView()
{
    return d->createView();
}

void ScreenplayTitlePageManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void ScreenplayTitlePageManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

} // namespace ManagementLayer
