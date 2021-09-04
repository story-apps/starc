#include "comic_book_text_manager.h"

#include "comic_book_text_view.h"

#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QApplication>
#include <QFileDialog>
#include <QTimer>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "screenplay-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class ComicBookTextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ComicBookTextView* createView();

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
    QPointer<BusinessLayer::ComicBookTextModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ComicBookTextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ComicBookTextView*> allViews;
};

ComicBookTextManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::ComicBookTextView* ComicBookTextManager::Implementation::createView()
{
    allViews.append(new Ui::ComicBookTextView);
    return allViews.last();
}

void ComicBookTextManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void ComicBookTextManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void ComicBookTextManager::Implementation::loadModelSettings()
{
    using namespace DataStorageLayer;
    const auto cursorPosition = StorageFacade::settingsStorage()
                                    ->value(cursorPositionFor(model->document()),
                                            SettingsStorage::SettingsPlace::Application, 0)
                                    .toInt();
    view->setCursorPosition(cursorPosition);
}

void ComicBookTextManager::Implementation::saveModelSettings()
{
    using namespace DataStorageLayer;
    StorageFacade::settingsStorage()->setValue(cursorPositionFor(model->document()),
                                               view->cursorPosition(),
                                               SettingsStorage::SettingsPlace::Application);
}


// ****


ComicBookTextManager::ComicBookTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ComicBookTextView::currentModelIndexChanged, this,
            &ComicBookTextManager::currentModelIndexChanged);
}

ComicBookTextManager::~ComicBookTextManager() = default;

QObject* ComicBookTextManager::asQObject()
{
    return this;
}

void ComicBookTextManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ComicBookTextModel*>(_model);
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

        //        connect(d->model, &BusinessLayer::ComicBookTextModel::nameChanged,
        //                d->view, &Ui::ComicBookTextView::setName);
        //        connect(d->model, &BusinessLayer::ComicBookTextModel::textChanged,
        //                d->view, &Ui::ComicBookTextView::setText);
        //        //
        //        connect(d->view, &Ui::ComicBookTextView::nameChanged,
        //                d->model, &BusinessLayer::ComicBookTextModel::setName);
        //        connect(d->view, &Ui::ComicBookTextView::textChanged,
        //                d->model, &BusinessLayer::ComicBookTextModel::setText);
    }
}

QWidget* ComicBookTextManager::view()
{
    return d->view;
}

QWidget* ComicBookTextManager::createView()
{
    return d->createView();
}

void ComicBookTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void ComicBookTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)), this,
                  SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);

    //
    // Ставим в очередь событие нотификацию о смене текущей сцены,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QTimer::singleShot(0, this,
                           [this] { emit currentModelIndexChanged(d->view->currentModelIndex()); });
    }
}

void ComicBookTextManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

void ComicBookTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
