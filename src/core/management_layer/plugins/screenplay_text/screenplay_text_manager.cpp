#include "screenplay_text_manager.h"

#include "screenplay_text_view.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "screenplay-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

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
    QPointer<BusinessLayer::ScreenplayTextModel> model;

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
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void ScreenplayTextManager::Implementation::saveModelSettings()
{
    setSettingsValue(cursorPositionFor(model->document()), view->cursorPosition());
}


// ****


ScreenplayTextManager::ScreenplayTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayTextView::currentModelIndexChanged, this,
            &ScreenplayTextManager::currentModelIndexChanged);
    connect(d->view, &Ui::ScreenplayTextView::addBookmarkRequested, this, [this] {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto dialog = new Ui::BookmarkDialog(d->view->topLevelWidget());
        connect(dialog, &Ui::BookmarkDialog::savePressed, this, [this, item, dialog] {
            auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            textItem->setBookmark({ dialog->bookmarkColor(), dialog->bookmarkName() });
            d->model->updateItem(textItem);

            dialog->hideDialog();
        });
        connect(dialog, &Ui::BookmarkDialog::disappeared, dialog, &Ui::BookmarkDialog::deleteLater);

        //
        // Отображаем диалог
        //
        dialog->showDialog();
    });
    connect(d->view, &Ui::ScreenplayTextView::removeBookmarkRequested, this, [this] {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        d->model->updateItem(textItem);
    });
}

ScreenplayTextManager::~ScreenplayTextManager() = default;

QObject* ScreenplayTextManager::asQObject()
{
    return this;
}

void ScreenplayTextManager::setModel(BusinessLayer::AbstractModel* _model)
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

Ui::IDocumentView* ScreenplayTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayTextManager::createView()
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
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
                  SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
    connect(_manager->asQObject(), SIGNAL(pasteBeatNameToEditorRequested(QString)), this,
            SLOT(pasteBeatNameToEditor(QString)), Qt::UniqueConnection);

    //
    // Ставим в очередь событие нотификацию о смене текущей сцены,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QMetaObject::invokeMethod(
            this, [this] { emit currentModelIndexChanged(d->view->currentModelIndex()); },
            Qt::QueuedConnection);
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
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

void ScreenplayTextManager::pasteBeatNameToEditor(const QString& _name)
{
    d->view->insertText(_name);
}

} // namespace ManagementLayer
