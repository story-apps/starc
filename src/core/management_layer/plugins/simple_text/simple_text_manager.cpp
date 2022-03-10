#include "simple_text_manager.h"

#include "simple_text_view.h"

#include <business_layer/model/simple_text/simple_text_model.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "simple-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class SimpleTextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::SimpleTextView* createView();

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
    QPointer<BusinessLayer::SimpleTextModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::SimpleTextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::SimpleTextView*> allViews;
};

SimpleTextManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::SimpleTextView* SimpleTextManager::Implementation::createView()
{
    allViews.append(new Ui::SimpleTextView);
    return allViews.last();
}

void SimpleTextManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void SimpleTextManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void SimpleTextManager::Implementation::loadModelSettings()
{
    using namespace DataStorageLayer;
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void SimpleTextManager::Implementation::saveModelSettings()
{
    setSettingsValue(cursorPositionFor(model->document()), view->cursorPosition());
}


// ****


SimpleTextManager::SimpleTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::SimpleTextView::currentModelIndexChanged, this,
            &SimpleTextManager::currentModelIndexChanged);
}

SimpleTextManager::~SimpleTextManager() = default;

QObject* SimpleTextManager::asQObject()
{
    return this;
}


void SimpleTextManager::setModel(BusinessLayer::AbstractModel* _model)
{
    if (d->model == _model) {
        return;
    }

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
    d->model = qobject_cast<BusinessLayer::SimpleTextModel*>(_model);
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

Ui::IDocumentView* SimpleTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* SimpleTextManager::createView()
{
    return d->createView();
}

void SimpleTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void SimpleTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)), this,
                  SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);

    //
    // Ставим в очередь событие нотификацию о смене текущей главы,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QMetaObject::invokeMethod(
            this, [this] { emit currentModelIndexChanged(d->view->currentModelIndex()); },
            Qt::QueuedConnection);
    }
}

void SimpleTextManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

void SimpleTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
