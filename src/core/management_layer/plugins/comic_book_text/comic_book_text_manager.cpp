#include "comic_book_text_manager.h"

#include "comic_book_text_view.h"

#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "comic-book-text";
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
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void ComicBookTextManager::Implementation::saveModelSettings()
{
    setSettingsValue(cursorPositionFor(model->document()), view->cursorPosition());
}


// ****


ComicBookTextManager::ComicBookTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ComicBookTextView::currentModelIndexChanged, this,
            &ComicBookTextManager::currentModelIndexChanged);
    auto showBookmarkDialog = [this](Ui::BookmarkDialog::DialogType _type) {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto dialog = new Ui::BookmarkDialog(d->view->topLevelWidget());
        dialog->setDialogType(_type);
        if (_type == Ui::BookmarkDialog::DialogType::Edit) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            dialog->setBookmarkName(textItem->bookmark()->name);
            dialog->setBookmarkColor(textItem->bookmark()->color);
        }
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
    };
    connect(d->view, &Ui::ComicBookTextView::addBookmarkRequested, this, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(d->view, &Ui::ComicBookTextView::editBookmarkRequested, this,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(d->view, &Ui::ComicBookTextView::createBookmarkRequested, this,
            [this](const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(d->view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::ComicBookTextView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::ComicBookTextView::removeBookmarkRequested, this, [this] {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        d->model->updateItem(textItem);
    });
}

ComicBookTextManager::~ComicBookTextManager() = default;

QObject* ComicBookTextManager::asQObject()
{
    return this;
}

void ComicBookTextManager::setModel(BusinessLayer::AbstractModel* _model)
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
    }
}

Ui::IDocumentView* ComicBookTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* ComicBookTextManager::createView()
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
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
                  SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);

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
