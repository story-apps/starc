#include "screenplay_text_manager.h"

#include "screenplay_text_view.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>

#include <QApplication>
#include <QFileDialog>
#include <QTimer>


namespace ManagementLayer
{

namespace  {
    const QString kSettingsKey = "screenplay-text";
    QString cursorPositionFor(Domain::DocumentObject* _item) {
        return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
    }
}

class ScreenplayTextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTextView* createView();

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
    BusinessLayer::ScreenplayTextModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTextView*> allViews;
};

ScreenplayTextManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::ScreenplayTextView* ScreenplayTextManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayTextView);
    return allViews.last();
}

void ScreenplayTextManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void ScreenplayTextManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void ScreenplayTextManager::Implementation::loadModelSettings()
{
    using namespace DataStorageLayer;
    const auto cursorPosition
            = StorageFacade::settingsStorage()->value(
                  cursorPositionFor(model->document()), SettingsStorage::SettingsPlace::Application, 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void ScreenplayTextManager::Implementation::saveModelSettings()
{
    using namespace DataStorageLayer;
    StorageFacade::settingsStorage()->setValue(
        cursorPositionFor(model->document()), view->cursorPosition(),
        SettingsStorage::SettingsPlace::Application);
}


// ****


ScreenplayTextManager::ScreenplayTextManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayTextView::currentModelIndexChanged,
            this, &ScreenplayTextManager::currentModelIndexChanged);
}

ScreenplayTextManager::~ScreenplayTextManager() = default;

QObject* ScreenplayTextManager::asQObject()
{
    return this;
}

void ScreenplayTextManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ScreenplayTextModel*>(_model);
    d->view->setModel(d->model);

    //
    // Если новая модель задана
    //
    if (d->model != nullptr) {
        //
        // ... загрузим параметры
        //
        d->loadModelSettings();
        //
        // ... настраиваем соединения
        //
//        d->view->setName(d->model->name());
//        d->view->setText(d->model->text());

//        connect(d->model, &BusinessLayer::ScreenplayTextModel::nameChanged,
//                d->view, &Ui::ScreenplayTextView::setName);
//        connect(d->model, &BusinessLayer::ScreenplayTextModel::textChanged,
//                d->view, &Ui::ScreenplayTextView::setText);
//        //
//        connect(d->view, &Ui::ScreenplayTextView::nameChanged,
//                d->model, &BusinessLayer::ScreenplayTextModel::setName);
//        connect(d->view, &Ui::ScreenplayTextView::textChanged,
//                d->model, &BusinessLayer::ScreenplayTextModel::setText);
    }
}

QWidget* ScreenplayTextManager::view()
{
    return d->view;
}

QWidget* ScreenplayTextManager::createView()
{
    return d->createView();
}

void ScreenplayTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void ScreenplayTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
            = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)),
                      this, SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);

    //
    // Ставим в очередь событие нотификацию о смене текущей сцены,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QTimer::singleShot(0, this, [this] {
            emit currentModelIndexChanged(d->view->currentModelIndex());
        });
    }
}

void ScreenplayTextManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

void ScreenplayTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    if (d->view->currentModelIndex().parent() == _index.parent()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
