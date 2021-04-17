#include "simpletext_manager.h"

#include "simpletext_view.h"

#include <business_layer/model/text/text_model.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>

#include <QApplication>
#include <QFileDialog>
#include <QTimer>


namespace ManagementLayer
{

namespace  {
const QString kSettingsKey = "simpletext";
QString cursorPositionFor(Domain::DocumentObject* _item) {
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
}

class TextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::TextView* createView();

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
    Ui::TextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::TextView*> allViews;
};

TextManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::TextView* TextManager::Implementation::createView()
{
    allViews.append(new Ui::TextView);
    return allViews.last();
}

void TextManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void TextManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void TextManager::Implementation::loadModelSettings()
{
    using namespace DataStorageLayer;
    const auto cursorPosition
            = StorageFacade::settingsStorage()->value(
                  cursorPositionFor(model->document()), SettingsStorage::SettingsPlace::Application, 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void TextManager::Implementation::saveModelSettings()
{
    using namespace DataStorageLayer;
    StorageFacade::settingsStorage()->setValue(
        cursorPositionFor(model->document()), view->cursorPosition(),
        SettingsStorage::SettingsPlace::Application);
}


// ****


TextManager::TextManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
    connect(d->view, &Ui::TextView::currentModelIndexChanged,
         this, &TextManager::currentModelIndexChanged);
}

TextManager::~TextManager() = default;

QObject* TextManager::asQObject()
{
    return this;
}


void TextManager::setModel(BusinessLayer::AbstractModel* _model)
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
        d->view->setName(d->model->name());

        //
        // ... настраиваем соединения
        //
        connect(d->model, &BusinessLayer::TextModel::nameChanged,
                d->view, &Ui::TextView::setName);
        //
        connect(d->view, &Ui::TextView::nameChanged,
                d->model, &BusinessLayer::TextModel::setName);
    }
}

QWidget* TextManager::view()
{
    return d->view;
}

QWidget* TextManager::createView()
{
    return d->createView();
}

void TextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void TextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
            = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)),
                      this, SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);

    //
    // Ставим в очередь событие нотификацию о смене текущей главы,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QTimer::singleShot(0, this, [this] {
            emit currentModelIndexChanged(d->view->currentModelIndex());
        });
    }
}

void TextManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

void TextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
